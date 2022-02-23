/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_MAPCHECKER_H
#define ENGINE_MAPCHECKER_H

#include <base/hash.h>

#include "kernel.h"

class IMapChecker : public IInterface
{
	MACRO_INTERFACE("mapchecker", 0)
public:
	virtual void AddMaplist(const struct CMapVersion *pMaplist, int Num) = 0;
	virtual bool IsMapValid(const char *pMapName, const SHA256_DIGEST *pMapSha256, unsigned MapCrc, unsigned MapSize) = 0;
	virtual bool ReadAndValidateMap(const char *pFilename, int StorageType) = 0;

	virtual int NumStandardMaps() = 0;
	virtual const char *GetStandardMapName(int Index) = 0;
	virtual bool IsStandardMap(const char *pMapName) = 0;
};

extern IMapChecker *CreateMapChecker();

#endif
