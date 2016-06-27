#include "entity.h"

#include <engine/server.h>

CModAPI_Entity::CModAPI_Entity(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int NbIdPerSnapshot, int ProximityRadius) :
	CEntity(pGameWorld, ObjType, Pos, ProximityRadius)
{
	m_NbIdPerSnapshot = NbIdPerSnapshot;
	m_IDList.set_size(3*m_NbIdPerSnapshot);
	
	for(int i=0; i<m_NbIdPerSnapshot; i++)
	{
		m_IDList[i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW06);
		m_IDList[NbIdPerSnapshot+i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW07);
		m_IDList[NbIdPerSnapshot*2+i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW07MODAPI);
	}
}

CModAPI_Entity::~CModAPI_Entity()
{
	for(int i=0; i<m_NbIdPerSnapshot; i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW06, m_IDList[i]);
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07, m_IDList[i]);
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07MODAPI, m_IDList[m_NbIdPerSnapshot+i]);
	}
}

void CModAPI_Entity::Snap07ModAPI(int Snapshot, int SnappingClient)
{
	Snap07(Snapshot, SnappingClient);
}

int CModAPI_Entity::GetID(int Snapshot, int ID)
{
	int TrueID = ID;
	if(Snapshot == MODAPI_SNAPSHOT_TW07MODAPI) TrueID += m_NbIdPerSnapshot;
	return m_IDList[TrueID];
}

CModAPI_Entity_SnapshotModAPI::CModAPI_Entity_SnapshotModAPI(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int NbId06, int NbId07, int NbId07ModAPI, int ProximityRadius) :
	CEntity(pGameWorld, ObjType, Pos, ProximityRadius)
{
	m_IDList06.set_size(NbId06);
	for(int i=0; i<m_IDList06.size(); i++)
	{
		m_IDList06[i] = Server()->SnapNewID(MODAPI_SNAPSHOT_TW06);
	}
	
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

CModAPI_Entity_SnapshotModAPI::~CModAPI_Entity_SnapshotModAPI()
{
	for(int i=0; i<m_IDList06.size(); i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW06, m_IDList06[i]);
	}
	
	for(int i=0; i<m_IDList07.size(); i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07, m_IDList07[i]);
	}
	
	for(int i=0; i<m_IDList07ModAPI.size(); i++)
	{
		Server()->SnapFreeID(MODAPI_SNAPSHOT_TW07MODAPI, m_IDList07ModAPI[i]);
	}
}

int CModAPI_Entity_SnapshotModAPI::GetID(int Snapshot, int ID)
{
	if(Snapshot == MODAPI_SNAPSHOT_TW06)
		return m_IDList06[ID];
	else if(Snapshot == MODAPI_SNAPSHOT_TW07)
		return m_IDList07[ID];
	else
		return m_IDList07ModAPI[ID];
}
