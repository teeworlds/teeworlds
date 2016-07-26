#include <base/system.h>
#include <modapi/shared/assetsfile.h>
#include <engine/storage.h>
#include <engine/shared/datafile.h>

class CModAPI_AssetsFile : public IModAPI_AssetsFileEngine
{
	CDataFileReader m_DataFile;
	
public:
	CModAPI_AssetsFile() {}

	virtual void *GetData(int Index) { return m_DataFile.GetData(Index); }
	virtual void *GetDataSwapped(int Index) { return m_DataFile.GetDataSwapped(Index); }
	virtual void UnloadData(int Index) { m_DataFile.UnloadData(Index); }
	virtual void *GetItem(int Index, int *pType, int *pID) { return m_DataFile.GetItem(Index, pType, pID); }
	virtual void GetType(int Type, int *pStart, int *pNum) { m_DataFile.GetType(Type, pStart, pNum); }
	virtual void *FindItem(int Type, int ID) { return m_DataFile.FindItem(Type, ID); }
	virtual int NumItems() { return m_DataFile.NumItems(); }

	virtual void Unload()
	{
		m_DataFile.Close();
	}

	virtual bool Load(const char *pModName, IStorage *pStorage)
	{
		if(!pStorage)
			pStorage = Kernel()->RequestInterface<IStorage>();
		if(!pStorage)
			return false;
		if(!m_DataFile.Open(pStorage, pModName, IStorage::TYPE_ALL))
			return false;
		
		return true;
	}

	virtual bool IsLoaded()
	{
		return m_DataFile.IsOpen();
	}

	virtual unsigned Crc()
	{
		return m_DataFile.Crc();
	}
};

extern IModAPI_AssetsFileEngine *CreateAssetsFileEngine() { return new CModAPI_AssetsFile; }
