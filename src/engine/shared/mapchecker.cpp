/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/storage.h>

#include <versionsrv/versionsrv.h>
#include <versionsrv/mapversions.h>

#include "mapchecker.h"

CMapChecker::CMapChecker()
{
	Init();
	SetDefaults();
}

void CMapChecker::Init()
{
	m_Whitelist.Reset();
	m_pFirst = 0;
	m_ClearListBeforeAdding = false;
}

void CMapChecker::SetDefaults()
{
	AddMaplist(s_aMapVersionList, s_NumMapVersionItems);
	m_ClearListBeforeAdding = true;
}

void CMapChecker::AddMaplist(const CMapVersion *pMaplist, int Num)
{
	if(m_ClearListBeforeAdding)
		Init();

	for(int i = 0; i < Num; ++i)
	{
		CWhitelistEntry *pEntry = (CWhitelistEntry *)m_Whitelist.Allocate(sizeof(CWhitelistEntry));
		pEntry->m_pNext = m_pFirst;
		m_pFirst = pEntry;

		str_copy(pEntry->m_aMapName, pMaplist[i].m_aName, sizeof(pEntry->m_aMapName));
		pEntry->m_MapCrc = bytes_be_to_uint(pMaplist[i].m_aCrc);
		pEntry->m_MapSize = bytes_be_to_uint(pMaplist[i].m_aSize);
		mem_copy(&pEntry->m_MapSha256, &pMaplist[i].m_aSha256, sizeof(pEntry->m_MapSha256));
	}
}

bool CMapChecker::IsMapValid(const char *pMapName, const SHA256_DIGEST *pMapSha256, unsigned MapCrc, unsigned MapSize)
{
	bool StandardMap = false;
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
	{
		if(str_comp(pCurrent->m_aMapName, pMapName) == 0)
		{
			StandardMap = true;
			if(pCurrent->m_MapCrc == MapCrc && pCurrent->m_MapSize == MapSize)
				return true;
		}
	}
	return !StandardMap;
}

bool CMapChecker::ReadAndValidateMap(const char *pFilename, int StorageType)
{
	IStorage *pStorage = Kernel()->RequestInterface<IStorage>();

	// extract map name
	char aMapName[MAX_MAP_LENGTH];
	char aMapNameExt[MAX_MAP_LENGTH+4];
	bool StandardMap = false;
	const char *pExtractedName = pFilename;
	const char *pEnd = 0;

	for(const char *pSrc = pFilename; *pSrc; ++pSrc)
	{
		if(*pSrc == '/' || *pSrc == '\\')
			pExtractedName = pSrc+1;
		else if(*pSrc == '.')
			pEnd = pSrc;
	}

	int Length = (int)(pEnd - pExtractedName);
	if(Length <= 0 || Length >= MAX_MAP_LENGTH)
		return true;
	str_truncate(aMapName, MAX_MAP_LENGTH, pExtractedName, pEnd - pExtractedName);
	str_format(aMapNameExt, sizeof(aMapNameExt), "%s.map", aMapName);

	// check for valid map
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
	{
		if(str_comp(pCurrent->m_aMapName, aMapName) == 0)
		{
			StandardMap = true;
			char aBuffer[IO_MAX_PATH_LENGTH];
			if(pStorage->FindFile(aMapNameExt, "maps", StorageType, aBuffer, sizeof(aBuffer), &pCurrent->m_MapSha256, pCurrent->m_MapCrc, pCurrent->m_MapSize))
				return true;
		}
		else if(StandardMap)
			break;
	}

	return !StandardMap;
}

int CMapChecker::NumStandardMaps()
{
	int Count = 0;
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
		Count++;
	return Count;
}

const char *CMapChecker::GetStandardMapName(int Index)
{
	int i = 0;
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext, i++)
		if(i == Index)
			return pCurrent->m_aMapName;
	return 0x0;
}

bool CMapChecker::IsStandardMap(const char *pMapName)
{
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
		if(str_comp(pCurrent->m_aMapName, pMapName) == 0)
			return true;
	return false;
}

IMapChecker *CreateMapChecker() { return new CMapChecker; }
