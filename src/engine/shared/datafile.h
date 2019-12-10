/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_DATAFILE_H
#define ENGINE_SHARED_DATAFILE_H

#include <base/hash.h>

// raw datafile access
class CDataFileReader
{
	struct CDatafile *m_pDataFile;
	void *GetDataImpl(int Index, int Swap);
public:
	CDataFileReader() : m_pDataFile(0) {}
	~CDataFileReader() { Close(); }

	bool IsOpen() const { return m_pDataFile != 0; }

	bool Open(class IStorage *pStorage, const char *pFilename, int StorageType);
	bool Close();

	void *GetData(int Index);
	void *GetDataSwapped(int Index); // makes sure that the data is 32bit LE ints when saved
	int GetDataSize(int Index) const;
	void ReplaceData(int Index, char *pData);
	void UnloadData(int Index);
	void *GetItem(int Index, int *pType, int *pID);
	int GetItemSize(int Index) const;
	void GetType(int Type, int *pStart, int *pNum);
	void *FindItem(int Type, int ID);
	int NumItems() const;
	int NumData() const;
	void Unload();

	SHA256_DIGEST Sha256() const;
	unsigned Crc() const;

	static bool CheckSha256(IOHANDLE Handle, const void *pSha256);
};

// write access
class CDataFileWriter
{
	struct CDataInfo
	{
		int m_UncompressedSize;
		int m_CompressedSize;
		void *m_pCompressedData;
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

	struct CItemTypeInfo
	{
		int m_Num;
		int m_First;
		int m_Last;
	};

	enum
	{
		MAX_ITEM_TYPES=0xffff,
		MAX_ITEMS=1024,
		MAX_DATAS=1024,
	};

	IOHANDLE m_File;
	int m_NumItems;
	int m_NumDatas;
	int m_NumItemTypes;
	CItemTypeInfo *m_pItemTypes;
	CItemInfo *m_pItems;
	CDataInfo *m_pDatas;

public:
	CDataFileWriter();
	~CDataFileWriter();
	bool Open(class IStorage *pStorage, const char *Filename);
	int AddData(int Size, void *pData);
	int AddDataSwapped(int Size, void *pData);
	int AddItem(int Type, int ID, int Size, void *pData);
	int Finish();
};


#endif
