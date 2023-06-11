/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <zlib.h>

#include <base/hash_ctxt.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/storage.h>

#include "datafile.h"

static const bool DEBUG = false;

enum
{
	MAX_ITEM_TYPES = 0xFFFF,
	MAX_ITEM_IDS = 0xFFFF,
	MAX_ITEMS = 1024,
	MAX_DATAS = 1024,
};

struct CDatafileItemType
{
	int m_Type;
	int m_Start;
	int m_Num;
};

struct CDatafileItem
{
	unsigned m_TypeAndID; // packed with masks MAX_ITEM_TYPES and MAX_ITEM_IDS
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

	int SizeOffset() const
	{
		// The size of these fields is not included in m_Size and m_Swaplen
		return sizeof(m_aID) + sizeof(m_Version) + sizeof(m_Size) + sizeof(m_Swaplen);
	}
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
	unsigned m_FileSize;
	SHA256_DIGEST m_Sha256;
	unsigned m_Crc;
	CDatafileInfo m_Info;
	CDatafileHeader m_Header;
	int m_DataStartOffset;
	char **m_ppDataPtrs;
	int *m_pDataSizes;
	char *m_pData;

	int GetFileItemSize(int Index) const
	{
		if(Index < 0 || Index >= m_Header.m_NumItems)
			return 0;

		if(Index == m_Header.m_NumItems - 1)
			return m_Header.m_ItemSize - m_Info.m_pItemOffsets[Index];

		return m_Info.m_pItemOffsets[Index + 1] - m_Info.m_pItemOffsets[Index];
	}

	int GetItemSize(int Index) const
	{
		const int FileItemSize = GetFileItemSize(Index);
		if(FileItemSize == 0)
			return 0;

		return FileItemSize - sizeof(CDatafileItem);
	}
};

bool CDataFileReader::Open(class IStorage *pStorage, const char *pFilename, int StorageType)
{
	dbg_assert(!m_pDataFile, "file already open");
	dbg_msg("datafile", "loading. filename='%s'", pFilename);

	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!File)
	{
		str_format(m_aError, sizeof(m_aError), "could not open '%s' for reading", pFilename);
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	// determine size and hashes of the file and store them
	int64 FileSize = 0;
	SHA256_CTX Sha256Ctx;
	sha256_init(&Sha256Ctx);
	unsigned Crc = crc32(0L, 0x0, 0);
	{
		unsigned char aBuffer[64 * 1024];
		while(true)
		{
			const unsigned Bytes = io_read(File, aBuffer, sizeof(aBuffer));
			if(Bytes == 0)
				break;
			FileSize += Bytes;
			sha256_update(&Sha256Ctx, aBuffer, Bytes);
			Crc = crc32(Crc, aBuffer, Bytes);
		}

		io_seek(File, 0, IOSEEK_START);
	}

	// read header
	CDatafileHeader Header;
	if(FileSize < (int)sizeof(Header) || io_read(File, &Header, sizeof(Header)) != sizeof(Header))
	{
		io_close(File);
		str_copy(m_aError, "could not read file header", sizeof(m_aError));
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	// TODO: change this header
	if(Header.m_aID[0] != 'A' || Header.m_aID[1] != 'T' || Header.m_aID[2] != 'A' || Header.m_aID[3] != 'D')
	{
		if(Header.m_aID[0] != 'D' || Header.m_aID[1] != 'A' || Header.m_aID[2] != 'T' || Header.m_aID[3] != 'A')
		{
			io_close(File);
			str_format(m_aError, sizeof(m_aError), "wrong signature. %x %x %x %x", Header.m_aID[0], Header.m_aID[1], Header.m_aID[2], Header.m_aID[3]);
			dbg_msg("datafile", "%s", m_aError);
			return false;
		}
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
#endif

	if(Header.m_Version != 3 && Header.m_Version != 4)
	{
		io_close(File);
		str_format(m_aError, sizeof(m_aError), "wrong version. version=%x", Header.m_Version);
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	// validate file information
	if(Header.m_NumItemTypes < 0 || Header.m_NumItemTypes > MAX_ITEM_TYPES + 1
		|| Header.m_NumItems < 0 || Header.m_NumItems > MAX_ITEMS
		|| Header.m_NumRawData < 0 || Header.m_NumRawData > MAX_DATAS
		|| Header.m_ItemSize < 0 || Header.m_ItemSize % sizeof(int) != 0
		|| Header.m_DataSize < 0)
	{
		io_close(File);
		str_copy(m_aError, "unable to load file, invalid header information", sizeof(m_aError));
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	// calculate and validate sizes
	int64 Size = 0;
	int SizeFix = 0;
	Size += Header.m_NumItemTypes * sizeof(CDatafileItemType);
	Size += (Header.m_NumItems + Header.m_NumRawData) * sizeof(int);
	if(Header.m_Version >= 4) // v4 has uncompressed data sizes aswell
	{
		// The size of the uncompressed data sizes was not included in
		// Header.m_Size and Header.m_Swaplen of version 4 maps prior
		// to commit 3dd1ea0d8f6cb442ac41bd223279f41d1ed1b2bb.
		SizeFix = Header.m_NumRawData * sizeof(int);
		Size += SizeFix;
	}
	Size += Header.m_ItemSize;

	bool ValidSize = true;
	if((int64)Header.m_Size + Header.SizeOffset() != FileSize)
	{
		if(SizeFix && (int64)Header.m_Size + SizeFix + Header.SizeOffset() == FileSize)
			Header.m_Size += SizeFix;
		else
			ValidSize = false;
	}

	if((int64)Header.m_Swaplen + Header.SizeOffset() != FileSize - Header.m_DataSize)
	{
		if(SizeFix && (int64)Header.m_Swaplen + SizeFix + Header.SizeOffset() == FileSize - Header.m_DataSize)
			Header.m_Swaplen += SizeFix;
		else
			ValidSize = false;
	}

	ValidSize &= Header.m_Swaplen == Header.m_Size - Header.m_DataSize;
	ValidSize &= Header.m_Swaplen % sizeof(int) == 0;
	ValidSize &= (int64)Header.m_DataSize + (int)sizeof(Header) + Size == FileSize;
	if(!ValidSize)
	{
		io_close(File);
		str_copy(m_aError, "unable to load file, invalid size information or truncated file", sizeof(m_aError));
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	int64 AllocSize = Size;
	AllocSize += sizeof(CDatafile); // add space for info structure
	AllocSize += Header.m_NumRawData * (sizeof(void *) + sizeof(int)); // add space for data pointers and sizes

	if(AllocSize > (int64(1) << 31)) // 2 GB
	{
		io_close(File);
		str_copy(m_aError, "unable to load file, too large", sizeof(m_aError));
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	CDatafile *pTmpDataFile = static_cast<CDatafile *>(mem_alloc(AllocSize));
	pTmpDataFile->m_Header = Header;
	pTmpDataFile->m_DataStartOffset = sizeof(CDatafileHeader) + Size;
	pTmpDataFile->m_ppDataPtrs = (char **)(pTmpDataFile + 1);
	pTmpDataFile->m_pDataSizes = (int *)(pTmpDataFile->m_ppDataPtrs + Header.m_NumRawData);
	pTmpDataFile->m_pData = (char *)(pTmpDataFile->m_pDataSizes + Header.m_NumRawData);
	pTmpDataFile->m_File = File;
	pTmpDataFile->m_FileSize = FileSize;
	pTmpDataFile->m_Sha256 = sha256_finish(&Sha256Ctx);
	pTmpDataFile->m_Crc = Crc;

	// clear the data pointers and sizes
	mem_zero(pTmpDataFile->m_ppDataPtrs, Header.m_NumRawData * sizeof(void *));
	mem_zero(pTmpDataFile->m_pDataSizes, Header.m_NumRawData * sizeof(int));

	// read types, offsets, sizes and item data
	const unsigned ReadSize = io_read(pTmpDataFile->m_File, pTmpDataFile->m_pData, Size);
	if(ReadSize != Size)
	{
		io_close(pTmpDataFile->m_File);
		mem_free(pTmpDataFile);
		str_format(m_aError, sizeof(m_aError), "truncation error, could not read all data (wanted=%d, got=%d)", unsigned(Size), ReadSize);
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(pTmpDataFile->m_pData, sizeof(int), Header.m_Swaplen / sizeof(int));
#endif

	if(DEBUG)
	{
		dbg_msg("datafile", "allocsize=%d", unsigned(AllocSize));
		dbg_msg("datafile", "readsize=%d", ReadSize);
		dbg_msg("datafile", "swaplen=%d", Header.m_Swaplen);
		dbg_msg("datafile", "item_size=%d", Header.m_ItemSize);
	}

	pTmpDataFile->m_Info.m_pItemTypes = (CDatafileItemType *)pTmpDataFile->m_pData;
	pTmpDataFile->m_Info.m_pItemOffsets = (int *)&pTmpDataFile->m_Info.m_pItemTypes[pTmpDataFile->m_Header.m_NumItemTypes];
	pTmpDataFile->m_Info.m_pDataOffsets = (int *)&pTmpDataFile->m_Info.m_pItemOffsets[pTmpDataFile->m_Header.m_NumItems];
	if(Header.m_Version >= 4)
	{
		pTmpDataFile->m_Info.m_pDataSizes = (int *)&pTmpDataFile->m_Info.m_pDataOffsets[pTmpDataFile->m_Header.m_NumRawData];
		pTmpDataFile->m_Info.m_pItemStart = (char *)&pTmpDataFile->m_Info.m_pDataSizes[pTmpDataFile->m_Header.m_NumRawData];
	}
	else
	{
		pTmpDataFile->m_Info.m_pDataSizes = 0x0;
		pTmpDataFile->m_Info.m_pItemStart = (char *)&pTmpDataFile->m_Info.m_pDataOffsets[pTmpDataFile->m_Header.m_NumRawData];
	}
	pTmpDataFile->m_Info.m_pDataStart = pTmpDataFile->m_Info.m_pItemStart + pTmpDataFile->m_Header.m_ItemSize;

	// validate info
	bool Valid = true;
	do
	{
		// validate item types
		int64 CountedItems = 0;
		for(int Index = 0; Index < pTmpDataFile->m_Header.m_NumItemTypes && Valid; Index++)
		{
			const CDatafileItemType *pItemType = &pTmpDataFile->m_Info.m_pItemTypes[Index];
			Valid &= pItemType->m_Type >= 0 && pItemType->m_Type <= MAX_ITEM_TYPES;
			Valid &= pItemType->m_Start >= 0 && pItemType->m_Num > 0;
			Valid &= pItemType->m_Start == CountedItems;
			CountedItems += pItemType->m_Num;
		}
		Valid &= CountedItems == pTmpDataFile->m_Header.m_NumItems;
		if(!Valid)
			break;

		// validate item offsets
		int PrevItemOffset = -1;
		for(int Index = 0; Index < pTmpDataFile->m_Header.m_NumItems && Valid; Index++)
		{
			const int Offset = pTmpDataFile->m_Info.m_pItemOffsets[Index];
			Valid &= Offset > PrevItemOffset;
			Valid &= Offset < pTmpDataFile->m_Header.m_ItemSize;
			Valid &= int64(pTmpDataFile->m_Info.m_pItemStart + Offset) % sizeof(int) == 0;
			PrevItemOffset = Offset;
		}
		if(!Valid)
			break;

		// validate item sizes
		int64 TotalItemSize = 0;
		for(int Index = 0; Index < pTmpDataFile->m_Header.m_NumItems && Valid; Index++)
		{
			const int FileItemSize = pTmpDataFile->GetFileItemSize(Index);
			if(FileItemSize < (int)sizeof(CDatafileItem))
				Valid = false;
			else
			{
				const CDatafileItem *pItem = reinterpret_cast<CDatafileItem *>(pTmpDataFile->m_Info.m_pItemStart + pTmpDataFile->m_Info.m_pItemOffsets[Index]);
				Valid &= pItem->m_Size >= 0;
				Valid &= pItem->m_Size == pTmpDataFile->GetItemSize(Index);
				TotalItemSize += FileItemSize;
			}
		}
		Valid &= TotalItemSize == pTmpDataFile->m_Header.m_ItemSize;
		if(!Valid)
			break;

		// validate data offsets
		int PrevDataOffset = -1;
		for(int Index = 0; Index < pTmpDataFile->m_Header.m_NumRawData && Valid; Index++)
		{
			const int Offset = pTmpDataFile->m_Info.m_pDataOffsets[Index];
			Valid &= Offset > PrevDataOffset;
			Valid &= Offset < pTmpDataFile->m_Header.m_DataSize;
			PrevDataOffset = Offset;
		}
		if(!Valid)
			break;

		// validate data sizes
		if(pTmpDataFile->m_Info.m_pDataSizes)
		{
			for(int Index = 0; Index < pTmpDataFile->m_Header.m_NumRawData && Valid; Index++)
			{
				Valid &= pTmpDataFile->m_Info.m_pDataSizes[Index] > 0;
			}
		}
	} while(false);

	if(!Valid)
	{
		io_close(pTmpDataFile->m_File);
		mem_free(pTmpDataFile);
		str_copy(m_aError, "unable to load file, invalid file information", sizeof(m_aError));
		dbg_msg("datafile", "%s", m_aError);
		return false;
	}

	// activate new data file
	Close();
	m_pDataFile = pTmpDataFile;
	m_aError[0] = '\0';

	dbg_msg("datafile", "loading done. datafile='%s'", pFilename);

	return true;
}

int CDataFileReader::NumData() const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->m_Header.m_NumRawData;
}

int CDataFileReader::GetFileDataSize(int Index) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	if(Index == m_pDataFile->m_Header.m_NumRawData - 1)
		return m_pDataFile->m_Header.m_DataSize - m_pDataFile->m_Info.m_pDataOffsets[Index];

	return m_pDataFile->m_Info.m_pDataOffsets[Index + 1] - m_pDataFile->m_Info.m_pDataOffsets[Index];
}

int CDataFileReader::GetDataSize(int Index) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	if(Index < 0 || Index >= m_pDataFile->m_Header.m_NumRawData)
		return 0;

	if(!m_pDataFile->m_ppDataPtrs[Index])
	{
		if(m_pDataFile->m_Info.m_pDataSizes)
		{
			return m_pDataFile->m_Info.m_pDataSizes[Index];
		}
		else
		{
			return GetFileDataSize(Index);
		}
	}
	const int Size = m_pDataFile->m_pDataSizes[Index];
	if(Size < 0)
		return 0; // summarize all errors as zero size
	return Size;
}

void *CDataFileReader::GetDataImpl(int Index, bool Swap)
{
	dbg_assert((bool)m_pDataFile, "file not open");

	if(Index < 0 || Index >= m_pDataFile->m_Header.m_NumRawData)
		return 0;

	// load it if needed
	if(!m_pDataFile->m_ppDataPtrs[Index])
	{
		// don't try to load again if it previously failed
		if(m_pDataFile->m_pDataSizes[Index] < 0)
			return 0;

		// fetch the data size
		const int DataSize = GetFileDataSize(Index);
#if defined(CONF_ARCH_ENDIAN_BIG)
		unsigned int SwapSize = DataSize;
#endif

		if(m_pDataFile->m_Info.m_pDataSizes)
		{
			// v4 has compressed data
			const unsigned OriginalUncompressedSize = m_pDataFile->m_Info.m_pDataSizes[Index];
			unsigned long UncompressedSize = OriginalUncompressedSize;
			dbg_msg("datafile", "loading data index=%d size=%d uncompressed=%lu", Index, DataSize, UncompressedSize);

			// read the compressed data
			void *pCompressedData = mem_alloc(DataSize);
			if(io_seek(m_pDataFile->m_File, m_pDataFile->m_DataStartOffset + m_pDataFile->m_Info.m_pDataOffsets[Index], IOSEEK_START)
				|| io_read(m_pDataFile->m_File, pCompressedData, DataSize) != (unsigned)DataSize)
			{
				dbg_msg("datafile", "truncation error, could not read all data");
				mem_free(pCompressedData);
				m_pDataFile->m_ppDataPtrs[Index] = 0x0;
				m_pDataFile->m_pDataSizes[Index] = -1;
				return 0;
			}

			// decompress the data
			m_pDataFile->m_ppDataPtrs[Index] = static_cast<char *>(mem_alloc(UncompressedSize));
			m_pDataFile->m_pDataSizes[Index] = UncompressedSize;
			const int Result = uncompress((Bytef *)m_pDataFile->m_ppDataPtrs[Index], &UncompressedSize, (Bytef *)pCompressedData, DataSize);
			mem_free(pCompressedData);
			if(Result != Z_OK || UncompressedSize != OriginalUncompressedSize)
			{
				dbg_msg("datafile", "uncompress error %d, size %lu", Result, UncompressedSize);
				mem_free(m_pDataFile->m_ppDataPtrs[Index]);
				m_pDataFile->m_ppDataPtrs[Index] = 0x0;
				m_pDataFile->m_pDataSizes[Index] = -1;
				return 0;
			}

#if defined(CONF_ARCH_ENDIAN_BIG)
			SwapSize = UncompressedSize;
#endif
		}
		else
		{
			// load the data
			dbg_msg("datafile", "loading data index=%d size=%d", Index, DataSize);
			m_pDataFile->m_ppDataPtrs[Index] = static_cast<char *>(mem_alloc(DataSize));
			m_pDataFile->m_pDataSizes[Index] = DataSize;
			if(io_seek(m_pDataFile->m_File, m_pDataFile->m_DataStartOffset + m_pDataFile->m_Info.m_pDataOffsets[Index], IOSEEK_START)
				|| io_read(m_pDataFile->m_File, m_pDataFile->m_ppDataPtrs[Index], DataSize) != (unsigned)DataSize)
			{
				dbg_msg("datafile", "truncation error, could not read all data");
				mem_free(m_pDataFile->m_ppDataPtrs[Index]);
				m_pDataFile->m_ppDataPtrs[Index] = 0x0;
				m_pDataFile->m_pDataSizes[Index] = -1;
				return 0;
			}
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
	return GetDataImpl(Index, false);
}

void *CDataFileReader::GetDataSwapped(int Index)
{
	return GetDataImpl(Index, true);
}

bool CDataFileReader::GetDataString(int Index, char *pBuffer, int BufferSize)
{
	dbg_assert(BufferSize > 0, "incorrect size");
	dbg_assert((bool)pBuffer, "no buffer");

	const int DataSize = GetDataSize(Index);
	const char *pData = static_cast<char *>(GetData(Index));
	if(!DataSize || !pData)
	{
		pBuffer[0] = '\0';
		UnloadData(Index);
		return false;
	}
	str_copy(pBuffer, pData, minimum(DataSize, BufferSize));
	UnloadData(Index);
	return true;
}

void CDataFileReader::ReplaceData(int Index, char *pData, int Size)
{
	dbg_assert((bool)m_pDataFile, "file not open");
	dbg_assert(Size > 0, "incorrect size");
	dbg_assert((bool)pData, "no data");

	if(Index < 0 || Index >= m_pDataFile->m_Header.m_NumRawData)
		return;

	mem_free(m_pDataFile->m_ppDataPtrs[Index]);
	m_pDataFile->m_ppDataPtrs[Index] = pData;
	m_pDataFile->m_pDataSizes[Index] = Size;
}

void CDataFileReader::UnloadData(int Index)
{
	dbg_assert((bool)m_pDataFile, "file not open");

	if(Index < 0 || Index >= m_pDataFile->m_Header.m_NumRawData)
		return;

	mem_free(m_pDataFile->m_ppDataPtrs[Index]);
	m_pDataFile->m_ppDataPtrs[Index] = 0x0;
	m_pDataFile->m_pDataSizes[Index] = 0;
}

int CDataFileReader::GetFileItemSize(int Index) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->GetFileItemSize(Index);
}

int CDataFileReader::GetItemSize(int Index) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->GetItemSize(Index);
}

void *CDataFileReader::GetItem(int Index, int *pType, int *pID, int *pSize) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	if(Index < 0 || Index >= m_pDataFile->m_Header.m_NumItems)
	{
		if(pType)
			*pType = 0;
		if(pID)
			*pID = 0;
		if(pSize)
			*pSize = 0;

		return 0;
	}

	CDatafileItem *pItem = reinterpret_cast<CDatafileItem *>(m_pDataFile->m_Info.m_pItemStart + m_pDataFile->m_Info.m_pItemOffsets[Index]);
	if(pType)
		*pType = (pItem->m_TypeAndID >> 16u) & MAX_ITEM_TYPES;
	if(pID)
		*pID = pItem->m_TypeAndID & MAX_ITEM_IDS;
	if(pSize)
		*pSize = pItem->m_Size;

	return (void *)(pItem + 1);
}

void CDataFileReader::GetType(int Type, int *pStart, int *pNum) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	for(int i = 0; i < m_pDataFile->m_Header.m_NumItemTypes; i++)
	{
		if(m_pDataFile->m_Info.m_pItemTypes[i].m_Type == Type)
		{
			*pStart = m_pDataFile->m_Info.m_pItemTypes[i].m_Start;
			*pNum = m_pDataFile->m_Info.m_pItemTypes[i].m_Num;
			return;
		}
	}

	*pStart = 0;
	*pNum = 0;
}

void *CDataFileReader::FindItem(int Type, int ID, int *pIndex, int *pSize) const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	int Start, Num;
	GetType(Type, &Start, &Num);
	for(int i = 0; i < Num; i++)
	{
		int ItemID, Size;
		void *pItem = GetItem(Start + i, 0, &ItemID, &Size);
		if(ID == ItemID)
		{
			if(pIndex)
				*pIndex = Start + i;
			if(pSize)
				*pSize = Size;
			return pItem;
		}
	}

	if(pIndex)
		*pIndex = -1;
	if(pSize)
		*pSize = 0;
	return 0;
}

int CDataFileReader::NumItems() const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->m_Header.m_NumItems;
}

bool CDataFileReader::Close()
{
	if(!m_pDataFile)
		return true;

	// free the data that is loaded
	for(int i = 0; i < m_pDataFile->m_Header.m_NumRawData; i++)
		UnloadData(i);

	io_close(m_pDataFile->m_File);
	mem_free(m_pDataFile);
	m_pDataFile = 0;
	return true;
}

SHA256_DIGEST CDataFileReader::Sha256() const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->m_Sha256;
}

unsigned CDataFileReader::Crc() const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->m_Crc;
}

unsigned CDataFileReader::FileSize() const
{
	dbg_assert((bool)m_pDataFile, "file not open");

	return m_pDataFile->m_FileSize;
}

bool CDataFileReader::CheckSha256(IOHANDLE Handle, const void *pSha256)
{
	// read the hash of the file
	SHA256_CTX Sha256Ctx;
	sha256_init(&Sha256Ctx);
	unsigned char aBuffer[64 * 1024];

	while(true)
	{
		const unsigned Bytes = io_read(Handle, aBuffer, sizeof(aBuffer));
		if(Bytes == 0)
			break;
		sha256_update(&Sha256Ctx, aBuffer, Bytes);
	}

	io_seek(Handle, 0, IOSEEK_START);
	SHA256_DIGEST Sha256 = sha256_finish(&Sha256Ctx);

	return !sha256_comp(*(const SHA256_DIGEST *)pSha256, Sha256);
}


struct CItemTypeInfo
{
	int m_Num;
	int m_First;
	int m_Last;
};

struct CItemInfo
{
	int m_Type;
	int m_ID;
	int m_Size;
	int m_Next;
	int m_Prev;
	void *m_pData;
};

struct CDataInfo
{
	int m_UncompressedSize;
	int m_CompressedSize;
	void *m_pCompressedData;
};

CDataFileWriter::CDataFileWriter()
{
	m_File = 0;
	m_pItemTypes = static_cast<CItemTypeInfo *>(mem_alloc(sizeof(CItemTypeInfo) * (MAX_ITEM_TYPES + 1)));
	m_pItems = static_cast<CItemInfo *>(mem_alloc(sizeof(CItemInfo) * MAX_ITEMS));
	m_pDatas = static_cast<CDataInfo *>(mem_alloc(sizeof(CDataInfo) * MAX_DATAS));
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
	dbg_assert(!m_File, "file already open");

	m_File = pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!m_File)
		return false;

	m_NumItems = 0;
	m_NumDatas = 0;
	m_NumItemTypes = 0;
	mem_zero(m_pItemTypes, sizeof(CItemTypeInfo) * (MAX_ITEM_TYPES + 1));

	for(int i = 0; i <= MAX_ITEM_TYPES; i++)
	{
		m_pItemTypes[i].m_First = -1;
		m_pItemTypes[i].m_Last = -1;
	}

	return true;
}

int CDataFileWriter::AddItem(int Type, int ID, int Size, const void *pData)
{
	dbg_assert((bool)m_File, "file not open");
	dbg_assert(m_NumItems < MAX_ITEMS, "too many items");
	dbg_assert(Type >= 0 && Type <= MAX_ITEM_TYPES, "incorrect type");
	dbg_assert(ID >= 0 && ID <= MAX_ITEM_IDS, "incorrect id");
	dbg_assert(Size >= 0, "incorrect size");
	dbg_assert(Size % sizeof(int) == 0, "incorrect boundary");
	dbg_assert(Size == 0 || pData, "no data");

	m_pItems[m_NumItems].m_Type = Type;
	m_pItems[m_NumItems].m_ID = ID;
	m_pItems[m_NumItems].m_Size = Size;

	// copy data
	if(Size > 0)
	{
		m_pItems[m_NumItems].m_pData = mem_alloc(Size);
		mem_copy(m_pItems[m_NumItems].m_pData, pData, Size);
	}
	else
		m_pItems[m_NumItems].m_pData = 0x0;

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
	return m_NumItems - 1;
}

int CDataFileWriter::AddData(int Size, const void *pData)
{
	dbg_assert((bool)m_File, "file not open");
	dbg_assert(Size > 0, "incorrect size");
	dbg_assert(m_NumDatas < MAX_DATAS, "too much data");
	dbg_assert((bool)pData, "no data");

	CDataInfo *pInfo = &m_pDatas[m_NumDatas];
	unsigned long CompressedSize = minimum(compressBound(Size), 1UL << 30);
	void *pCompData = mem_alloc(CompressedSize); // temporary buffer that we use during compression

	const int Result = compress((Bytef *)pCompData, &CompressedSize, (Bytef *)pData, Size);
	if(Result != Z_OK)
	{
		dbg_msg("datafile", "compress error %d", Result);
		mem_free(pCompData);
		return -1;
	}

	pInfo->m_UncompressedSize = Size;
	pInfo->m_CompressedSize = (int)CompressedSize;
	pInfo->m_pCompressedData = mem_alloc(pInfo->m_CompressedSize);
	mem_copy(pInfo->m_pCompressedData, pCompData, pInfo->m_CompressedSize);
	mem_free(pCompData);

	m_NumDatas++;
	return m_NumDatas - 1;
}

int CDataFileWriter::AddDataSwapped(int Size, const void *pData)
{
	dbg_assert((bool)m_File, "file not open");
	dbg_assert(Size > 0, "incorrect size");
	dbg_assert(Size % sizeof(int) == 0, "incorrect boundary");
	dbg_assert((bool)pData, "no data");

#if defined(CONF_ARCH_ENDIAN_BIG)
	void *pSwapped = mem_alloc(Size); // temporary buffer that we use during compression
	mem_copy(pSwapped, pData, Size);
	swap_endian(pSwapped, sizeof(int), Size/sizeof(int));
	const int Index = AddData(Size, pSwapped);
	mem_free(pSwapped);
	return Index;
#else
	return AddData(Size, pData);
#endif
}


bool CDataFileWriter::Finish()
{
	dbg_assert((bool)m_File, "file not open");

	// we should now write this file!
	if(DEBUG)
		dbg_msg("datafile", "writing");

	// calculate sizes
	int ItemSize = 0;
	for(int i = 0; i < m_NumItems; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "item=%d size=%d (%d)", i, m_pItems[i].m_Size, (int)(m_pItems[i].m_Size + sizeof(CDatafileItem)));
		ItemSize += m_pItems[i].m_Size + sizeof(CDatafileItem);
	}

	int DataSize = 0;
	for(int i = 0; i < m_NumDatas; i++)
		DataSize += m_pDatas[i].m_CompressedSize;

	// calculate the complete size
	const int TypesSize = m_NumItemTypes * sizeof(CDatafileItemType);
	const int HeaderSize = sizeof(CDatafileHeader);
	const int OffsetSize = (m_NumItems + 2 * m_NumDatas) * sizeof(int); // ItemOffsets, DataOffsets, DataUncompressedSizes
	const int SwapSize = HeaderSize + TypesSize + OffsetSize + ItemSize;
	const int FileSize = SwapSize + DataSize;

	if(DEBUG)
		dbg_msg("datafile", "m_NumItemTypes=%d TypesSize=%d ItemSize=%d DataSize=%d", m_NumItemTypes, TypesSize, ItemSize, DataSize);

	// construct Header
	{
		CDatafileHeader Header;
		Header.m_aID[0] = 'D';
		Header.m_aID[1] = 'A';
		Header.m_aID[2] = 'T';
		Header.m_aID[3] = 'A';
		Header.m_Version = 4;
		Header.m_Size = FileSize - Header.SizeOffset();
		Header.m_Swaplen = SwapSize - Header.SizeOffset();
		Header.m_NumItemTypes = m_NumItemTypes;
		Header.m_NumItems = m_NumItems;
		Header.m_NumRawData = m_NumDatas;
		Header.m_ItemSize = ItemSize;
		Header.m_DataSize = DataSize;

		// write Header
		if(DEBUG)
			dbg_msg("datafile", "HeaderSize=%d", (int)sizeof(Header));

#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
#endif

		io_write(m_File, &Header, sizeof(Header));
	}

	// write types
	for(int i = 0, Count = 0; i <= MAX_ITEM_TYPES; i++)
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
	for(int Type = 0, Offset = 0; Type <= MAX_ITEM_TYPES; Type++)
	{
		if(m_pItemTypes[Type].m_Num)
		{
			// write all m_pItems in of this type
			for(int k = m_pItemTypes[Type].m_First; k != -1; k = m_pItems[k].m_Next)
			{
				if(DEBUG)
					dbg_msg("datafile", "writing item offset num=%d offset=%d", k, Offset);
				int Temp = Offset;
#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(&Temp, sizeof(int), sizeof(Temp)/sizeof(int));
#endif
				io_write(m_File, &Temp, sizeof(Temp));
				Offset += m_pItems[k].m_Size + sizeof(CDatafileItem);
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
	for(int Type = 0; Type <= MAX_ITEM_TYPES; Type++)
	{
		if(m_pItemTypes[Type].m_Num)
		{
			// write all m_pItems in of this type
			for(int k = m_pItemTypes[Type].m_First; k != -1; k = m_pItems[k].m_Next)
			{
				CDatafileItem Item;
				Item.m_TypeAndID = (unsigned(Type) << 16u) | unsigned(m_pItems[k].m_ID);
				Item.m_Size = m_pItems[k].m_Size;
				if(DEBUG)
					dbg_msg("datafile", "writing item type=%x idx=%d id=%d size=%d", Type, k, m_pItems[k].m_ID, m_pItems[k].m_Size);

#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(&Item, sizeof(int), sizeof(Item)/sizeof(int));
				if(m_pItems[k].m_pData)
					swap_endian(m_pItems[k].m_pData, sizeof(int), m_pItems[k].m_Size/sizeof(int));
#endif
				io_write(m_File, &Item, sizeof(Item));
				if(m_pItems[k].m_pData)
					io_write(m_File, m_pItems[k].m_pData, m_pItems[k].m_Size);
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
	return true;
}
