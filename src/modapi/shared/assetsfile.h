/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef MODAPI_ENGINE_ASSETSFILE_H
#define MODAPI_ENGINE_ASSETSFILE_H

#include <modapi/graphics.h>

#include <engine/kernel.h>

class IModAPI_AssetsFile : public IInterface
{
	MACRO_INTERFACE("assetsfile", 0)
public:
	virtual void *GetData(int Index) = 0;
	virtual void *GetDataSwapped(int Index) = 0;
	virtual void UnloadData(int Index) = 0;
	virtual void *GetItem(int Index, int *Type, int *pID) = 0;
	virtual void GetType(int Type, int *pStart, int *pNum) = 0;
	virtual void *FindItem(int Type, int ID) = 0;
	virtual int NumItems() = 0;
};


class IModAPI_AssetsFileEngine : public IModAPI_AssetsFile
{
	MACRO_INTERFACE("assetsfileengine", 0)
public:
	virtual bool Load(const char *pAssetsFileName, class IStorage *pStorage=0) = 0;
	virtual bool IsLoaded() = 0;
	virtual void Unload() = 0;
	virtual unsigned Crc() = 0;
};

extern IModAPI_AssetsFileEngine *CreateAssetsFileEngine();

#endif
