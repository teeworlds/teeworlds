/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_MOD_H
#define ENGINE_MOD_H

#include <modapi/graphics.h>

#include <engine/kernel.h>

class IMod : public IInterface
{
	MACRO_INTERFACE("mod", 0)
public:
	virtual void *GetData(int Index) = 0;
	virtual void *GetDataSwapped(int Index) = 0;
	virtual void UnloadData(int Index) = 0;
	virtual void *GetItem(int Index, int *Type, int *pID) = 0;
	virtual void GetType(int Type, int *pStart, int *pNum) = 0;
	virtual void *FindItem(int Type, int ID) = 0;
	virtual int NumItems() = 0;
};


class IEngineMod : public IMod
{
	MACRO_INTERFACE("enginemod", 0)
public:
	virtual bool Load(const char *pModName, class IStorage *pStorage=0) = 0;
	virtual bool IsLoaded() = 0;
	virtual void Unload() = 0;
	virtual unsigned Crc() = 0;
};

extern IEngineMod *CreateEngineMod();

#endif
