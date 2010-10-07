// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <base/system.h>
#include <engine/map.h>
#include <engine/storage.h>
#include "datafile.h"

class CMap : public IEngineMap
{
	CDataFileReader m_DataFile;
public:
	CMap() {}
	
	virtual void *GetData(int Index) { return m_DataFile.GetData(Index); }
	virtual void *GetDataSwapped(int Index) { return m_DataFile.GetDataSwapped(Index); }
	virtual void UnloadData(int Index) { m_DataFile.UnloadData(Index); }
	virtual void *GetItem(int Index, int *pType, int *pId) { return m_DataFile.GetItem(Index, pType, pId); }
	virtual void GetType(int Type, int *pStart, int *pNum) { m_DataFile.GetType(Type, pStart, pNum); }
	virtual void *FindItem(int Type, int Id) { return m_DataFile.FindItem(Type, Id); }
	virtual int NumItems() { return m_DataFile.NumItems(); }
	
	virtual void Unload()
	{
		m_DataFile.Close();
	}

	virtual bool Load(const char *pMapName)
	{
		IStorage *pStorage = Kernel()->RequestInterface<IStorage>();
		if(!pStorage)
			return false;
		return m_DataFile.Open(pStorage, pMapName, IStorage::TYPE_ALL);
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

extern IEngineMap *CreateEngineMap() { return new CMap; }
