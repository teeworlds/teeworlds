/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "gameworld.h"
#include "entity.h"
#include "gamecontext.h"

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;
	
	m_Paused = false;
	m_ResetRequested = false;
	m_pFirstEntity = 0x0;
	for(int i = 0; i < NUM_ENT_TYPES; i++)
		m_apFirstEntityTypes[i] = 0;
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	while(m_pFirstEntity)
		delete m_pFirstEntity;
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return m_apFirstEntityTypes[Type];
}


int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type)
{
	int Num = 0;
	for(CEntity *pEnt = (Type<0) ? m_pFirstEntity : m_apFirstEntityTypes[Type];
		pEnt; pEnt = (Type<0) ? pEnt->m_pNextEntity : pEnt->m_pNextTypeEntity)
	{
		if(distance(pEnt->m_Pos, Pos) < Radius+pEnt->m_ProximityRadius)
		{
			ppEnts[Num] = pEnt;
			Num++;
			if(Num == Max)
				break;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
	CEntity *pCur = m_pFirstEntity;
	while(pCur)
	{
		dbg_assert(pCur != pEnt, "err");
		pCur = pCur->m_pNextEntity;
	}

	// insert it
	if(m_pFirstEntity)
		m_pFirstEntity->m_pPrevEntity = pEnt;
	pEnt->m_pNextEntity = m_pFirstEntity;
	pEnt->m_pPrevEntity = 0x0;
	m_pFirstEntity = pEnt;

	// into typelist aswell
	if(m_apFirstEntityTypes[pEnt->m_Objtype])
		m_apFirstEntityTypes[pEnt->m_Objtype]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_Objtype];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_Objtype] = pEnt;
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if(!pEnt->m_pNextEntity && !pEnt->m_pPrevEntity && m_pFirstEntity != pEnt)
		return;

	// remove
	if(pEnt->m_pPrevEntity)
		pEnt->m_pPrevEntity->m_pNextEntity = pEnt->m_pNextEntity;
	else
		m_pFirstEntity = pEnt->m_pNextEntity;
	if(pEnt->m_pNextEntity)
		pEnt->m_pNextEntity->m_pPrevEntity = pEnt->m_pPrevEntity;

	if(pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_Objtype] = pEnt->m_pNextTypeEntity;
	if(pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if(m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextEntity;

	pEnt->m_pNextEntity = 0;
	pEnt->m_pPrevEntity = 0;
	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for(CEntity *pEnt = m_pFirstEntity; pEnt; )
	{
		m_pNextTraverseEntity = pEnt->m_pNextEntity;
		pEnt->Snap(SnappingClient);
		pEnt = m_pNextTraverseEntity;
	}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(CEntity *pEnt = m_pFirstEntity; pEnt; )
	{
		m_pNextTraverseEntity = pEnt->m_pNextEntity;
		pEnt->Reset();
		pEnt = m_pNextTraverseEntity;
	}
	RemoveEntities();

	GameServer()->m_pController->PostReset();
	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	CEntity *pEnt = m_pFirstEntity;
	while(pEnt)
	{
		m_pNextTraverseEntity = pEnt->m_pNextEntity;
		if(pEnt->m_MarkedForDestroy)
		{
			RemoveEntity(pEnt);
			pEnt->Destroy();
		}
		pEnt = m_pNextTraverseEntity;
	}
}

void CGameWorld::Tick()
{
	if(m_ResetRequested)
		Reset();

	if(!m_Paused)
	{
		if(GameServer()->m_pController->IsForceBalanced())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for(CEntity *pEnt = m_pFirstEntity; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextEntity;
			pEnt->Tick();
			pEnt = m_pNextTraverseEntity;
		}
		
		for(CEntity *pEnt = m_pFirstEntity; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextEntity;
			pEnt->TickDefered();
			pEnt = m_pNextTraverseEntity;
		}
	}

	RemoveEntities();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	vec2 LineDir = normalize(Pos1-Pos0);
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(NETOBJTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;
			
		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}
	
	return pClosest;
}


CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CEntity *pNotThis)
{
	// Find other players
	float ClosestRange = Radius*2;
	CCharacter *pClosest = 0;
		
	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(NETOBJTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;
			
		float Len = distance(Pos, p->m_Pos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			if(Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}
	
	return pClosest;
}
