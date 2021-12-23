/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_DATAFILE_H
#define ENGINE_SHARED_DATAFILE_H

#include <base/hash.h>
#include <base/system.h>

// raw datafile access
class CDataFileReader
{
	struct CDatafile *m_pDataFile;
	char m_aError[128];

	void *GetDataImpl(int Index, bool Swap);
	int GetFileDataSize(int Index) const;
	int GetFileItemSize(int Index) const;

public:
	CDataFileReader() : m_pDataFile(0) { m_aError[0] = '\0'; }
	~CDataFileReader() { Close(); }

	bool Open(class IStorage *pStorage, const char *pFilename, int StorageType);
	bool Close();

	bool IsOpen() const { return m_pDataFile != 0; }
	const char *GetError() const { return m_aError; }

	void *GetData(int Index);
	void *GetDataSwapped(int Index); // makes sure that the data is 32bit LE ints when saved
	bool GetDataString(int Index, char *pBuffer, int BufferSize);
	int GetDataSize(int Index) const;
	void ReplaceData(int Index, char *pData, int Size);
	void UnloadData(int Index);
	void *GetItem(int Index, int *pType = 0, int *pID = 0, int *pSize = 0) const;
	int GetItemSize(int Index) const;
	void GetType(int Type, int *pStart, int *pNum) const;
	void *FindItem(int Type, int ID, int *pIndex = 0, int *pSize = 0) const;
	int NumItems() const;
	int NumData() const;

	SHA256_DIGEST Sha256() const;
	unsigned Crc() const;
	unsigned FileSize() const;

	static bool CheckSha256(IOHANDLE Handle, const void *pSha256);
};

// write access
class CDataFileWriter
{
	IOHANDLE m_File;
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumDatas;
	struct CItemTypeInfo *m_pItemTypes;
	struct CItemInfo *m_pItems;
	struct CDataInfo *m_pDatas;

public:
	CDataFileWriter();
	~CDataFileWriter();

	bool Open(class IStorage *pStorage, const char *Filename);
	bool Finish();

	// These function return the index of the added data/item, or -1 on error.
	int AddData(int Size, const void *pData);
	int AddDataSwapped(int Size, const void *pData);
	int AddItem(int Type, int ID, int Size, const void *pData);
};


#endif
