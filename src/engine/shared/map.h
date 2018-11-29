/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/map.h>
#include "datafile.h"

class CMap : public IEngineMap
{
	CDataFileReader m_DataFile;
public:
	CMap() {}

	virtual void *GetData(int Index);
	virtual void *GetDataSwapped(int Index);
	virtual void UnloadData(int Index);
	virtual void *GetItem(int Index, int *pType, int *pID);
	virtual void GetType(int Type, int *pStart, int *pNum);
	virtual void *FindItem(int Type, int ID);
	virtual int NumItems();

	virtual void Unload();
	virtual bool Load(const char *pMapName, IStorage *pStorage);
	virtual bool IsLoaded();

	virtual unsigned Crc();
};
