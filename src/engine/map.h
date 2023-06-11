/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_MAP_H
#define ENGINE_MAP_H

#include <base/hash.h>

#include "kernel.h"
#include "storage.h"

class IMap : public IInterface
{
	MACRO_INTERFACE("map", 0)
public:
	virtual void *GetData(int Index) = 0;
	virtual void *GetDataSwapped(int Index) = 0;
	virtual bool GetDataString(int Index, char *pBuffer, int BufferSize) = 0;
	virtual int GetDataSize(int Index) const = 0;
	virtual void UnloadData(int Index) = 0;
	virtual void *GetItem(int Index, int *pType = 0, int *pID = 0, int *pSize = 0) const = 0;
	virtual int GetItemSize(int Index) const = 0;
	virtual void GetType(int Type, int *pStart, int *pNum) const = 0;
	virtual void *FindItem(int Type, int ID, int *pIndex = 0, int *pSize = 0) const = 0;
	virtual int NumItems() const = 0;
	virtual int NumData() const = 0;
};


class IEngineMap : public IMap
{
	MACRO_INTERFACE("enginemap", 0)
public:
	virtual bool Load(const char *pMapName, class IStorage *pStorage = 0, int StorageType = IStorage::TYPE_ALL) = 0;
	virtual bool IsLoaded() const = 0;
	virtual const char *GetError() const = 0;
	virtual void Unload() = 0;
	virtual SHA256_DIGEST Sha256() const = 0;
	virtual unsigned Crc() const = 0;
	virtual unsigned FileSize() const = 0;
};

extern IEngineMap *CreateEngineMap();

#endif
