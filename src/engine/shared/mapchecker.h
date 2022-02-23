/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_MAPCHECKER_H
#define ENGINE_SHARED_MAPCHECKER_H

#include <engine/mapchecker.h>

#include "memheap.h"

class CMapChecker : public IMapChecker
{
	enum
	{
		MAX_MAP_LENGTH = 8,
	};

	struct CWhitelistEntry
	{
		char m_aMapName[MAX_MAP_LENGTH];
		SHA256_DIGEST m_MapSha256;
		unsigned m_MapCrc;
		unsigned m_MapSize;
		CWhitelistEntry *m_pNext;
	};

	class CHeap m_Whitelist;
	CWhitelistEntry *m_pFirst;

	bool m_ClearListBeforeAdding; // whether to clear the existing list before adding a new map list

	void Init();
	void SetDefaults();

public:
	CMapChecker();
	void AddMaplist(const struct CMapVersion *pMaplist, int Num);
	bool IsMapValid(const char *pMapName, const SHA256_DIGEST *pMapSha256, unsigned MapCrc, unsigned MapSize);
	bool ReadAndValidateMap(const char *pFilename, int StorageType);

	int NumStandardMaps();
	const char *GetStandardMapName(int Index);
	bool IsStandardMap(const char *pMapName);
};

#endif
