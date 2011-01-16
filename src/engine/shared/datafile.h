/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_DATAFILE_H
#define ENGINE_SHARED_DATAFILE_H

// raw datafile access
class CDataFileReader
{
	class CDatafile *m_pDataFile;
	void *GetDataImpl(int Index, int Swap);
public:
	CDataFileReader() : m_pDataFile(0) {}
	~CDataFileReader() { Close(); }
	
	bool IsOpen() const { return m_pDataFile != 0; }
	
	bool Open(class IStorage *pStorage, const char *pFilename, int StorageType);
	bool Close();
	
	void *GetData(int Index);
	void *GetDataSwapped(int Index); // makes sure that the data is 32bit LE ints when saved
	int GetDataSize(int Index);
	void UnloadData(int Index);
	void *GetItem(int Index, int *pType, int *pId);
	int GetItemSize(int Index);
	void GetType(int Type, int *pStart, int *pNum);
	void *FindItem(int Type, int Id);
	int NumItems();
	int NumData();
	void Unload();
	
	unsigned Crc();
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
		int m_Id;
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
	
	IOHANDLE m_File;
	int m_NumItems;
	int m_NumDatas;
	int m_NumItemTypes;
	CItemTypeInfo m_aItemTypes[0xffff];
	CItemInfo m_aItems[1024];
	CDataInfo m_aDatas[1024];	
	
public:
	CDataFileWriter() : m_File(0) {}
	bool Open(class IStorage *pStorage, const char *Filename);
	int AddData(int Size, void *pData);
	int AddDataSwapped(int Size, void *pData);
	int AddItem(int Type, int Id, int Size, void *pData);
	int Finish();
};


#endif
