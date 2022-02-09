/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_MAPCHECKER_H
#define ENGINE_SHARED_MAPCHECKER_H

#include <base/hash.h>

#include "memheap.h"

class CMapChecker
{
	enum
	{
		MAX_MAP_LENGTH=8,
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

	bool m_RemoveDefaultList;

	void Init();
	void SetDefaults();

public:
	CMapChecker();
	void AddMaplist(struct CMapVersion *pMaplist, int Num);
	bool IsMapValid(const char *pMapName, const SHA256_DIGEST *pMapSha256, unsigned MapCrc, unsigned MapSize);
	bool ReadAndValidateMap(class IStorage *pStorage, const char *pFilename, int StorageType);
};

#endif
