#include "entity.h"

#include <engine/server.h>

CModAPI_EntitySnapshot07::CModAPI_EntitySnapshot07(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int ProximityRadius, int NbIdPerSnapshot) :
	CEntity(pGameWorld, ObjType, Pos, ProximityRadius)
{
	m_NbIdPerSnapshot = NbIdPerSnapshot;
	m_IDList.set_size(2*m_NbIdPerSnapshot);
	
	for(int i=0; i<m_NbIdPerSnapshot; i++)
	{
		m_IDList[i] = Server()->SnapNewID07();
		m_IDList[NbIdPerSnapshot+i] = Server()->SnapNewID07ModAPI();
	}
}

CModAPI_EntitySnapshot07::~CModAPI_EntitySnapshot07()
{
	for(int i=0; i<m_NbIdPerSnapshot; i++)
	{
		Server()->SnapFreeID07(m_IDList[i]);
		Server()->SnapFreeID07ModAPI(m_IDList[m_NbIdPerSnapshot+i]);
	}
}

void CModAPI_EntitySnapshot07::Snap(int SnappingClient, int FirstID)
{
	
}

void CModAPI_EntitySnapshot07::Snap07(int SnappingClient)
{
	Snap(SnappingClient, 0);
}

void CModAPI_EntitySnapshot07::Snap07ModAPI(int SnappingClient)
{
	Snap(SnappingClient, m_NbIdPerSnapshot);
}

int CModAPI_EntitySnapshot07::GetID(int ID)
{
	return m_IDList[ID];
}
