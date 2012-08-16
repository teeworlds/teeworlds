/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/storage.h>

#include <versionsrv/versionsrv.h>
#include <versionsrv/mapversions.h>

#include "datafile.h"
#include "memheap.h"
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
	m_RemoveDefaultList = false;
}

void CMapChecker::SetDefaults()
{
	AddMaplist(s_aMapVersionList, s_NumMapVersionItems);
	m_RemoveDefaultList = true;
}

void CMapChecker::AddMaplist(CMapVersion *pMaplist, int Num)
{
	if(m_RemoveDefaultList)
		Init();

	for(int i = 0; i < Num; ++i)
	{
		CWhitelistEntry *pEntry = (CWhitelistEntry *)m_Whitelist.Allocate(sizeof(CWhitelistEntry));
		pEntry->m_pNext = m_pFirst;
		m_pFirst = pEntry;

		str_copy(pEntry->m_aMapName, pMaplist[i].m_aName, sizeof(pEntry->m_aMapName));
		pEntry->m_MapCrc = (pMaplist[i].m_aCrc[0]<<24) | (pMaplist[i].m_aCrc[1]<<16) | (pMaplist[i].m_aCrc[2]<<8) | pMaplist[i].m_aCrc[3];
		pEntry->m_MapSize = (pMaplist[i].m_aSize[0]<<24) | (pMaplist[i].m_aSize[1]<<16) | (pMaplist[i].m_aSize[2]<<8) | pMaplist[i].m_aSize[3];
	}
}

bool CMapChecker::IsMapValid(const char *pMapName, unsigned MapCrc, unsigned MapSize)
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
	return StandardMap?false:true;
}

bool CMapChecker::ReadAndValidateMap(IStorage *pStorage, const char *pFilename, int StorageType)
{
	bool LoadedMapInfo = false;
	bool StandardMap = false;
	unsigned MapCrc = 0;
	unsigned MapSize = 0;

	// extract map name
	char aMapName[MAX_MAP_LENGTH];
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
	str_copy(aMapName, pExtractedName, min((int)MAX_MAP_LENGTH, (int)(pEnd-pExtractedName+1)));

	// check for valid map
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
	{
		if(str_comp(pCurrent->m_aMapName, aMapName) == 0)
		{
			StandardMap = true;
			if(!LoadedMapInfo)
			{
				if(!CDataFileReader::GetCrcSize(pStorage, pFilename, StorageType, &MapCrc, &MapSize))
					return true;
				LoadedMapInfo = true;
			}

			if(pCurrent->m_MapCrc == MapCrc && pCurrent->m_MapSize == MapSize)
				return true;
		}
	}
	return StandardMap?false:true;
}
