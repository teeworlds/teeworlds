/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>
#include <engine/storage.h>
#include "datafile.h"
#include <zlib.h>

static const int DEBUG=0;

struct CDatafileItemType
{
	int m_Type;
	int m_Start;
	int m_Num;
} ;

struct CDatafileItem
{
	int m_TypeAndID;
	int m_Size;
};

struct CDatafileHeader
{
	char m_aID[4];
	int m_Version;
	int m_Size;
	int m_Swaplen;
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumRawData;
	int m_ItemSize;
	int m_DataSize;
};

struct CDatafileData
{
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumRawData;
	int m_ItemSize;
	int m_DataSize;
	char m_aStart[4];
};

struct CDatafileInfo
{
	CDatafileItemType *m_pItemTypes;
	int *m_pItemOffsets;
	int *m_pDataOffsets;
	int *m_pDataSizes;

	char *m_pItemStart;
	char *m_pDataStart;
};

struct CDatafile
{
	IOHANDLE m_File;
	unsigned m_Crc;
	CDatafileInfo m_Info;
	CDatafileHeader m_Header;
	int m_DataStartOffset;
	char **m_ppDataPtrs;
	char *m_pData;
};

bool CDataFileReader::Open(class IStorage *pStorage, const char *pFilename, int StorageType)
{
	dbg_msg("datafile", "loading. filename='%s'", pFilename);

	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!File)
	{
		dbg_msg("datafile", "could not open '%s'", pFilename);
		return false;
	}


	// take the CRC of the file and store it
	unsigned Crc = 0;
	{
		enum
		{
			BUFFER_SIZE = 64*1024
		};

		unsigned char aBuffer[BUFFER_SIZE];

		while(1)
		{
			unsigned Bytes = io_read(File, aBuffer, BUFFER_SIZE);
			if(Bytes <= 0)
				break;
			Crc = crc32(Crc, aBuffer, Bytes); // ignore_convention
		}

		io_seek(File, 0, IOSEEK_START);
	}


	// TODO: change this header
	CDatafileHeader Header;
	io_read(File, &Header, sizeof(Header));
	if(Header.m_aID[0] != 'A' || Header.m_aID[1] != 'T' || Header.m_aID[2] != 'A' || Header.m_aID[3] != 'D')
	{
		if(Header.m_aID[0] != 'D' || Header.m_aID[1] != 'A' || Header.m_aID[2] != 'T' || Header.m_aID[3] != 'A')
		{
			dbg_msg("datafile", "wrong signature. %x %x %x %x", Header.m_aID[0], Header.m_aID[1], Header.m_aID[2], Header.m_aID[3]);
			io_close(File);
			return 0;
		}
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
#endif
	if(Header.m_Version != 3 && Header.m_Version != 4)
	{
		dbg_msg("datafile", "wrong version. version=%x", Header.m_Version);
		io_close(File);
		return 0;
	}

	// read in the rest except the data
	unsigned Size = 0;
	Size += Header.m_NumItemTypes*sizeof(CDatafileItemType);
	Size += (Header.m_NumItems+Header.m_NumRawData)*sizeof(int);
	if(Header.m_Version == 4)
		Size += Header.m_NumRawData*sizeof(int); // v4 has uncompressed data sizes aswell
	Size += Header.m_ItemSize;

	unsigned AllocSize = Size;
	AllocSize += sizeof(CDatafile); // add space for info structure
	AllocSize += Header.m_NumRawData*sizeof(void*); // add space for data pointers

	CDatafile *pTmpDataFile = (CDatafile*)mem_alloc(AllocSize, 1);
	pTmpDataFile->m_Header = Header;
	pTmpDataFile->m_DataStartOffset = sizeof(CDatafileHeader) + Size;
	pTmpDataFile->m_ppDataPtrs = (char**)(pTmpDataFile+1);
	pTmpDataFile->m_pData = (char *)(pTmpDataFile+1)+Header.m_NumRawData*sizeof(char *);
	pTmpDataFile->m_File = File;
	pTmpDataFile->m_Crc = Crc;

	// clear the data pointers
	mem_zero(pTmpDataFile->m_ppDataPtrs, Header.m_NumRawData*sizeof(void*));

	// read types, offsets, sizes and item data
	unsigned ReadSize = io_read(File, pTmpDataFile->m_pData, Size);
	if(ReadSize != Size)
	{
		io_close(pTmpDataFile->m_File);
		mem_free(pTmpDataFile);
		pTmpDataFile = 0;
		dbg_msg("datafile", "couldn't load the whole thing, wanted=%d got=%d", Size, ReadSize);
		return false;
	}

	Close();
	m_pDataFile = pTmpDataFile;

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(m_pDataFile->m_pData, sizeof(int), min(static_cast<unsigned>(Header.m_Swaplen), Size) / sizeof(int));
#endif

	//if(DEBUG)
	{
		dbg_msg("datafile", "allocsize=%d", AllocSize);
		dbg_msg("datafile", "readsize=%d", ReadSize);
		dbg_msg("datafile", "swaplen=%d", Header.m_Swaplen);
		dbg_msg("datafile", "item_size=%d", m_pDataFile->m_Header.m_ItemSize);
	}

	m_pDataFile->m_Info.m_pItemTypes = (CDatafileItemType *)m_pDataFile->m_pData;
	m_pDataFile->m_Info.m_pItemOffsets = (int *)&m_pDataFile->m_Info.m_pItemTypes[m_pDataFile->m_Header.m_NumItemTypes];
	m_pDataFile->m_Info.m_pDataOffsets = (int *)&m_pDataFile->m_Info.m_pItemOffsets[m_pDataFile->m_Header.m_NumItems];
	m_pDataFile->m_Info.m_pDataSizes = (int *)&m_pDataFile->m_Info.m_pDataOffsets[m_pDataFile->m_Header.m_NumRawData];

	if(Header.m_Version == 4)
		m_pDataFile->m_Info.m_pItemStart = (char *)&m_pDataFile->m_Info.m_pDataSizes[m_pDataFile->m_Header.m_NumRawData];
	else
		m_pDataFile->m_Info.m_pItemStart = (char *)&m_pDataFile->m_Info.m_pDataOffsets[m_pDataFile->m_Header.m_NumRawData];
	m_pDataFile->m_Info.m_pDataStart = m_pDataFile->m_Info.m_pItemStart + m_pDataFile->m_Header.m_ItemSize;

	dbg_msg("datafile", "loading done. datafile='%s'", pFilename);

	if(DEBUG)
	{
		/*
		for(int i = 0; i < m_pDataFile->data.num_raw_data; i++)
		{
			void *p = datafile_get_data(df, i);
			dbg_msg("datafile", "%d %d", (int)((char*)p - (char*)(&m_pDataFile->data)), size);
		}

		for(int i = 0; i < datafile_num_items(df); i++)
		{
			int type, id;
			void *data = datafile_get_item(df, i, &type, &id);
			dbg_msg("map", "\t%d: type=%x id=%x p=%p offset=%d", i, type, id, data, m_pDataFile->info.item_offsets[i]);
			int *idata = (int*)data;
			for(int k = 0; k < 3; k++)
				dbg_msg("datafile", "\t\t%d=%d (%x)", k, idata[k], idata[k]);
		}

		for(int i = 0; i < m_pDataFile->data.num_m_aItemTypes; i++)
		{
			dbg_msg("map", "\t%d: type=%x start=%d num=%d", i,
				m_pDataFile->info.m_aItemTypes[i].type,
				m_pDataFile->info.m_aItemTypes[i].start,
				m_pDataFile->info.m_aItemTypes[i].num);
			for(int k = 0; k < m_pDataFile->info.m_aItemTypes[i].num; k++)
			{
				int type, id;
				datafile_get_item(df, m_pDataFile->info.m_aItemTypes[i].start+k, &type, &id);
				if(type != m_pDataFile->info.m_aItemTypes[i].type)
					dbg_msg("map", "\tERROR");
			}
		}
		*/
	}

	return true;
}

bool CDataFileReader::GetCrcSize(class IStorage *pStorage, const char *pFilename, int StorageType, unsigned *pCrc, unsigned *pSize)
{
	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!File)
		return false;

	// get crc and size
	unsigned Crc = 0;
	unsigned Size = 0;
	unsigned char aBuffer[64*1024];
	while(1)
	{
		unsigned Bytes = io_read(File, aBuffer, sizeof(aBuffer));
		if(Bytes <= 0)
			break;
		Crc = crc32(Crc, aBuffer, Bytes); // ignore_convention
		Size += Bytes;
	}

	io_close(File);

	*pCrc = Crc;
	*pSize = Size;
	return true;
}

int CDataFileReader::NumData()
{
	if(!m_pDataFile) { return 0; }
	return m_pDataFile->m_Header.m_NumRawData;
}

// always returns the size in the file
int CDataFileReader::GetDataSize(int Index)
{
	if(!m_pDataFile) { return 0; }

	if(Index == m_pDataFile->m_Header.m_NumRawData-1)
		return m_pDataFile->m_Header.m_DataSize-m_pDataFile->m_Info.m_pDataOffsets[Index];
	return m_pDataFile->m_Info.m_pDataOffsets[Index+1]-m_pDataFile->m_Info.m_pDataOffsets[Index];
}

void *CDataFileReader::GetDataImpl(int Index, int Swap)
{
	if(!m_pDataFile) { return 0; }

	// load it if needed
	if(!m_pDataFile->m_ppDataPtrs[Index])
	{
		// fetch the data size
		int DataSize = GetDataSize(Index);
#if defined(CONF_ARCH_ENDIAN_BIG)
		int SwapSize = DataSize;
#endif

		if(m_pDataFile->m_Header.m_Version == 4)
		{
			// v4 has compressed data
			void *pTemp = (char *)mem_alloc(DataSize, 1);
			unsigned long UncompressedSize = m_pDataFile->m_Info.m_pDataSizes[Index];
			unsigned long s;

			dbg_msg("datafile", "loading data index=%d size=%d uncompressed=%d", Index, DataSize, UncompressedSize);
			m_pDataFile->m_ppDataPtrs[Index] = (char *)mem_alloc(UncompressedSize, 1);

			// read the compressed data
			io_seek(m_pDataFile->m_File, m_pDataFile->m_DataStartOffset+m_pDataFile->m_Info.m_pDataOffsets[Index], IOSEEK_START);
			io_read(m_pDataFile->m_File, pTemp, DataSize);

			// decompress the data, TODO: check for errors
			s = UncompressedSize;
			uncompress((Bytef*)m_pDataFile->m_ppDataPtrs[Index], &s, (Bytef*)pTemp, DataSize); // ignore_convention
#if defined(CONF_ARCH_ENDIAN_BIG)
			SwapSize = s;
#endif

			// clean up the temporary buffers
			mem_free(pTemp);
		}
		else
		{
			// load the data
			dbg_msg("datafile", "loading data index=%d size=%d", Index, DataSize);
			m_pDataFile->m_ppDataPtrs[Index] = (char *)mem_alloc(DataSize, 1);
			io_seek(m_pDataFile->m_File, m_pDataFile->m_DataStartOffset+m_pDataFile->m_Info.m_pDataOffsets[Index], IOSEEK_START);
			io_read(m_pDataFile->m_File, m_pDataFile->m_ppDataPtrs[Index], DataSize);
		}

#if defined(CONF_ARCH_ENDIAN_BIG)
		if(Swap && SwapSize)
			swap_endian(m_pDataFile->m_ppDataPtrs[Index], sizeof(int), SwapSize/sizeof(int));
#endif
	}

	return m_pDataFile->m_ppDataPtrs[Index];
}

void *CDataFileReader::GetData(int Index)
{
	return GetDataImpl(Index, 0);
}

void *CDataFileReader::GetDataSwapped(int Index)
{
	return GetDataImpl(Index, 1);
}

void CDataFileReader::UnloadData(int Index)
{
	if(Index < 0)
		return;

	//
	mem_free(m_pDataFile->m_ppDataPtrs[Index]);
	m_pDataFile->m_ppDataPtrs[Index] = 0x0;
}

int CDataFileReader::GetItemSize(int Index)
{
	if(!m_pDataFile) { return 0; }
	if(Index == m_pDataFile->m_Header.m_NumItems-1)
		return m_pDataFile->m_Header.m_ItemSize-m_pDataFile->m_Info.m_pItemOffsets[Index];
	return m_pDataFile->m_Info.m_pItemOffsets[Index+1]-m_pDataFile->m_Info.m_pItemOffsets[Index];
}

void *CDataFileReader::GetItem(int Index, int *pType, int *pID)
{
	if(!m_pDataFile) { if(pType) *pType = 0; if(pID) *pID = 0; return 0; }

	CDatafileItem *i = (CDatafileItem *)(m_pDataFile->m_Info.m_pItemStart+m_pDataFile->m_Info.m_pItemOffsets[Index]);
	if(pType)
		*pType = (i->m_TypeAndID>>16)&0xffff; // remove sign extention
	if(pID)
		*pID = i->m_TypeAndID&0xffff;
	return (void *)(i+1);
}

void CDataFileReader::GetType(int Type, int *pStart, int *pNum)
{
	*pStart = 0;
	*pNum = 0;

	if(!m_pDataFile)
		return;

	for(int i = 0; i < m_pDataFile->m_Header.m_NumItemTypes; i++)
	{
		if(m_pDataFile->m_Info.m_pItemTypes[i].m_Type == Type)
		{
			*pStart = m_pDataFile->m_Info.m_pItemTypes[i].m_Start;
			*pNum = m_pDataFile->m_Info.m_pItemTypes[i].m_Num;
			return;
		}
	}
}

void *CDataFileReader::FindItem(int Type, int ID)
{
	if(!m_pDataFile) return 0;

	int Start, Num;
	GetType(Type, &Start, &Num);
	for(int i = 0; i < Num; i++)
	{
		int ItemID;
		void *pItem = GetItem(Start+i,0, &ItemID);
		if(ID == ItemID)
			return pItem;
	}
	return 0;
}

int CDataFileReader::NumItems()
{
	if(!m_pDataFile) return 0;
	return m_pDataFile->m_Header.m_NumItems;
}

bool CDataFileReader::Close()
{
	if(!m_pDataFile)
		return true;

	// free the data that is loaded
	int i;
	for(i = 0; i < m_pDataFile->m_Header.m_NumRawData; i++)
		mem_free(m_pDataFile->m_ppDataPtrs[i]);

	io_close(m_pDataFile->m_File);
	mem_free(m_pDataFile);
	m_pDataFile = 0;
	return true;
}

unsigned CDataFileReader::Crc()
{
	if(!m_pDataFile) return 0xFFFFFFFF;
	return m_pDataFile->m_Crc;
}


CDataFileWriter::CDataFileWriter()
{
	m_File = 0;
	m_pItemTypes = static_cast<CItemTypeInfo *>(mem_alloc(sizeof(CItemTypeInfo) * MAX_ITEM_TYPES, 1));
	m_pItems = static_cast<CItemInfo *>(mem_alloc(sizeof(CItemInfo) * MAX_ITEMS, 1));
	m_pDatas = static_cast<CDataInfo *>(mem_alloc(sizeof(CDataInfo) * MAX_DATAS, 1));
}

CDataFileWriter::~CDataFileWriter()
{
	mem_free(m_pItemTypes);
	m_pItemTypes = 0;
	mem_free(m_pItems);
	m_pItems = 0;
	mem_free(m_pDatas);
	m_pDatas = 0;
}

bool CDataFileWriter::Open(class IStorage *pStorage, const char *pFilename)
{
	dbg_assert(!m_File, "a file already exists");
	m_File = pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!m_File)
		return false;

	m_NumItems = 0;
	m_NumDatas = 0;
	m_NumItemTypes = 0;
	mem_zero(m_pItemTypes, sizeof(CItemTypeInfo) * MAX_ITEM_TYPES);

	for(int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pItemTypes[i].m_First = -1;
		m_pItemTypes[i].m_Last = -1;
	}

	return true;
}

int CDataFileWriter::AddItem(int Type, int ID, int Size, void *pData)
{
	if(!m_File) return 0;

	dbg_assert(Type >= 0 && Type < 0xFFFF, "incorrect type");
	dbg_assert(m_NumItems < 1024, "too many items");
	dbg_assert(Size%sizeof(int) == 0, "incorrect boundary");

	m_pItems[m_NumItems].m_Type = Type;
	m_pItems[m_NumItems].m_ID = ID;
	m_pItems[m_NumItems].m_Size = Size;

	// copy data
	m_pItems[m_NumItems].m_pData = mem_alloc(Size, 1);
	mem_copy(m_pItems[m_NumItems].m_pData, pData, Size);

	if(!m_pItemTypes[Type].m_Num) // count item types
		m_NumItemTypes++;

	// link
	m_pItems[m_NumItems].m_Prev = m_pItemTypes[Type].m_Last;
	m_pItems[m_NumItems].m_Next = -1;

	if(m_pItemTypes[Type].m_Last != -1)
		m_pItems[m_pItemTypes[Type].m_Last].m_Next = m_NumItems;
	m_pItemTypes[Type].m_Last = m_NumItems;

	if(m_pItemTypes[Type].m_First == -1)
		m_pItemTypes[Type].m_First = m_NumItems;

	m_pItemTypes[Type].m_Num++;

	m_NumItems++;
	return m_NumItems-1;
}

int CDataFileWriter::AddData(int Size, void *pData)
{
	if(!m_File) return 0;

	dbg_assert(m_NumDatas < 1024, "too much data");

	CDataInfo *pInfo = &m_pDatas[m_NumDatas];
	unsigned long s = compressBound(Size);
	void *pCompData = mem_alloc(s, 1); // temporary buffer that we use during compression

	int Result = compress((Bytef*)pCompData, &s, (Bytef*)pData, Size); // ignore_convention
	if(Result != Z_OK)
	{
		dbg_msg("datafile", "compression error %d", Result);
		dbg_assert(0, "zlib error");
	}

	pInfo->m_UncompressedSize = Size;
	pInfo->m_CompressedSize = (int)s;
	pInfo->m_pCompressedData = mem_alloc(pInfo->m_CompressedSize, 1);
	mem_copy(pInfo->m_pCompressedData, pCompData, pInfo->m_CompressedSize);
	mem_free(pCompData);

	m_NumDatas++;
	return m_NumDatas-1;
}

int CDataFileWriter::AddDataSwapped(int Size, void *pData)
{
	dbg_assert(Size%sizeof(int) == 0, "incorrect boundary");

#if defined(CONF_ARCH_ENDIAN_BIG)
	void *pSwapped = mem_alloc(Size, 1); // temporary buffer that we use during compression
	mem_copy(pSwapped, pData, Size);
	swap_endian(pSwapped, sizeof(int), Size/sizeof(int));
	int Index = AddData(Size, pSwapped);
	mem_free(pSwapped);
	return Index;
#else
	return AddData(Size, pData);
#endif
}


int CDataFileWriter::Finish()
{
	if(!m_File) return 1;

	int ItemSize = 0;
	int TypesSize, HeaderSize, OffsetSize, FileSize, SwapSize;
	int DataSize = 0;
	CDatafileHeader Header;

	// we should now write this file!
	if(DEBUG)
		dbg_msg("datafile", "writing");

	// calculate sizes
	for(int i = 0; i < m_NumItems; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "item=%d size=%d (%d)", i, m_pItems[i].m_Size, m_pItems[i].m_Size+sizeof(CDatafileItem));
		ItemSize += m_pItems[i].m_Size + sizeof(CDatafileItem);
	}


	for(int i = 0; i < m_NumDatas; i++)
		DataSize += m_pDatas[i].m_CompressedSize;

	// calculate the complete size
	TypesSize = m_NumItemTypes*sizeof(CDatafileItemType);
	HeaderSize = sizeof(CDatafileHeader);
	OffsetSize = (m_NumItems + m_NumDatas + m_NumDatas) * sizeof(int); // ItemOffsets, DataOffsets, DataUncompressedSizes
	FileSize = HeaderSize + TypesSize + OffsetSize + ItemSize + DataSize;
	SwapSize = FileSize - DataSize;

	(void)SwapSize;

	if(DEBUG)
		dbg_msg("datafile", "num_m_aItemTypes=%d TypesSize=%d m_aItemsize=%d DataSize=%d", m_NumItemTypes, TypesSize, ItemSize, DataSize);

	// construct Header
	{
		Header.m_aID[0] = 'D';
		Header.m_aID[1] = 'A';
		Header.m_aID[2] = 'T';
		Header.m_aID[3] = 'A';
		Header.m_Version = 4;
		Header.m_Size = FileSize - 16;
		Header.m_Swaplen = SwapSize - 16;
		Header.m_NumItemTypes = m_NumItemTypes;
		Header.m_NumItems = m_NumItems;
		Header.m_NumRawData = m_NumDatas;
		Header.m_ItemSize = ItemSize;
		Header.m_DataSize = DataSize;

		// write Header
		if(DEBUG)
			dbg_msg("datafile", "HeaderSize=%d", sizeof(Header));
#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
#endif
		io_write(m_File, &Header, sizeof(Header));
	}

	// write types
	for(int i = 0, Count = 0; i < 0xffff; i++)
	{
		if(m_pItemTypes[i].m_Num)
		{
			// write info
			CDatafileItemType Info;
			Info.m_Type = i;
			Info.m_Start = Count;
			Info.m_Num = m_pItemTypes[i].m_Num;
			if(DEBUG)
				dbg_msg("datafile", "writing type=%x start=%d num=%d", Info.m_Type, Info.m_Start, Info.m_Num);
#if defined(CONF_ARCH_ENDIAN_BIG)
			swap_endian(&Info, sizeof(int), sizeof(CDatafileItemType)/sizeof(int));
#endif
			io_write(m_File, &Info, sizeof(Info));
			Count += m_pItemTypes[i].m_Num;
		}
	}

	// write item offsets
	for(int i = 0, Offset = 0; i < 0xffff; i++)
	{
		if(m_pItemTypes[i].m_Num)
		{
			// write all m_pItems in of this type
			int k = m_pItemTypes[i].m_First;
			while(k != -1)
			{
				if(DEBUG)
					dbg_msg("datafile", "writing item offset num=%d offset=%d", k, Offset);
				int Temp = Offset;
#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(&Temp, sizeof(int), sizeof(Temp)/sizeof(int));
#endif
				io_write(m_File, &Temp, sizeof(Temp));
				Offset += m_pItems[k].m_Size + sizeof(CDatafileItem);

				// next
				k = m_pItems[k].m_Next;
			}
		}
	}

	// write data offsets
	for(int i = 0, Offset = 0; i < m_NumDatas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data offset num=%d offset=%d", i, Offset);
		int Temp = Offset;
#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&Temp, sizeof(int), sizeof(Temp)/sizeof(int));
#endif
		io_write(m_File, &Temp, sizeof(Temp));
		Offset += m_pDatas[i].m_CompressedSize;
	}

	// write data uncompressed sizes
	for(int i = 0; i < m_NumDatas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data uncompressed size num=%d size=%d", i, m_pDatas[i].m_UncompressedSize);
		int UncompressedSize = m_pDatas[i].m_UncompressedSize;
#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&UncompressedSize, sizeof(int), sizeof(UncompressedSize)/sizeof(int));
#endif
		io_write(m_File, &UncompressedSize, sizeof(UncompressedSize));
	}

	// write m_pItems
	for(int i = 0; i < 0xffff; i++)
	{
		if(m_pItemTypes[i].m_Num)
		{
			// write all m_pItems in of this type
			int k = m_pItemTypes[i].m_First;
			while(k != -1)
			{
				CDatafileItem Item;
				Item.m_TypeAndID = (i<<16)|m_pItems[k].m_ID;
				Item.m_Size = m_pItems[k].m_Size;
				if(DEBUG)
					dbg_msg("datafile", "writing item type=%x idx=%d id=%d size=%d", i, k, m_pItems[k].m_ID, m_pItems[k].m_Size);

#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(&Item, sizeof(int), sizeof(Item)/sizeof(int));
				swap_endian(m_pItems[k].m_pData, sizeof(int), m_pItems[k].m_Size/sizeof(int));
#endif
				io_write(m_File, &Item, sizeof(Item));
				io_write(m_File, m_pItems[k].m_pData, m_pItems[k].m_Size);

				// next
				k = m_pItems[k].m_Next;
			}
		}
	}

	// write data
	for(int i = 0; i < m_NumDatas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data id=%d size=%d", i, m_pDatas[i].m_CompressedSize);
		io_write(m_File, m_pDatas[i].m_pCompressedData, m_pDatas[i].m_CompressedSize);
	}

	// free data
	for(int i = 0; i < m_NumItems; i++)
		mem_free(m_pItems[i].m_pData);
	for(int i = 0; i < m_NumDatas; ++i)
		mem_free(m_pDatas[i].m_pCompressedData);

	io_close(m_File);
	m_File = 0;

	if(DEBUG)
		dbg_msg("datafile", "done");
	return 0;
}
