#include "entity.h"

#include <engine/server.h>

CModAPI_EntitySnapshot07::CModAPI_EntitySnapshot07(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int NbIdPerSnapshot, int ProximityRadius) :
	CEntity(pGameWorld, ObjType, Pos, ProximityRadius)
{
	m_NbIdPerSnapshot = NbIdPerSnapshot;
	m_IDList.set_size(2*m_NbIdPerSnapshot);
	
	for(int i=0; i<m_NbIdPerSnapshot; i++)
	{
		m_IDList[i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW07);
		m_IDList[NbIdPerSnapshot+i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW07MODAPI);
	}
}

CModAPI_EntitySnapshot07::~CModAPI_EntitySnapshot07()
{
	for(int i=0; i<m_NbIdPerSnapshot; i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07, m_IDList[i]);
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07MODAPI, m_IDList[m_NbIdPerSnapshot+i]);
	}
}

void CModAPI_EntitySnapshot07::Snap(int Snapshot, int SnappingClient)
{
	
}

void CModAPI_EntitySnapshot07::Snap07(int SnappingClient)
{
	Snap(MODAPI_SNAPSHOT_TW07, SnappingClient);
}

void CModAPI_EntitySnapshot07::Snap07ModAPI(int SnappingClient)
{
	Snap(MODAPI_SNAPSHOT_TW07MODAPI, SnappingClient);
}

int CModAPI_EntitySnapshot07::GetID(int Snapshot, int ID)
{
	int TrueID = ID;
	if(Snapshot == MODAPI_SNAPSHOT_TW07MODAPI) TrueID += m_NbIdPerSnapshot;
	return m_IDList[TrueID];
}

CModAPI_EntityMultiSnapshot::CModAPI_EntityMultiSnapshot(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int NbId07, int NbId07ModAPI, int ProximityRadius) :
	CEntity(pGameWorld, ObjType, Pos, ProximityRadius)
{
	m_IDList07.set_size(NbId07);
	for(int i=0; i<m_IDList07.size(); i++)
	{
		m_IDList07[i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW07);
	}
	
	m_IDList07ModAPI.set_size(NbId07ModAPI);
	for(int i=0; i<m_IDList07ModAPI.size(); i++)
	{
		m_IDList07ModAPI[i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW07MODAPI);
	}
}

CModAPI_EntityMultiSnapshot::~CModAPI_EntityMultiSnapshot()
{
	for(int i=0; i<m_IDList07.size(); i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07, m_IDList07[i]);
	}
	
	for(int i=0; i<m_IDList07ModAPI.size(); i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07MODAPI, m_IDList07ModAPI[i]);
	}
}

void CModAPI_EntityMultiSnapshot::Snap07(int SnappingClient)
{
	
}

void CModAPI_EntityMultiSnapshot::Snap07ModAPI(int SnappingClient)
{
	
}

int CModAPI_EntityMultiSnapshot::GetID(int Snapshot, int ID)
{
	if(Snapshot == MODAPI_SNAPSHOT_TW07)
		return m_IDList07[ID];
	else
		return m_IDList07ModAPI[ID];
}
