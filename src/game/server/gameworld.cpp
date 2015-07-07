/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "entity.h"
#include "gamecontext.h"
#include "gamecontroller.h"
#include "gameworld.h"

CGameWorld::CGameWorld()
{
	m_pGameServer = 0;
	m_pServer = 0;

	m_Paused = false;
	m_Resetting = false;
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
	m_aLists[pEnt->GetEntType()].Insert(pEnt);
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	if(!pEnt->IsMarkedForDestroy())
	{
		pEnt->MarkForDestroy();
		m_aLists[pEnt->GetEntType()].Remove(pEnt);
		m_DestroyList.Insert(pEnt);
	}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntityList::CIterator e = m_aLists[i]; e.Exists(); e.Next())
			e.Get()->Reset();
	CleanupEntities();

	GameServer()->m_pController->OnReset();
	CleanupEntities();

	m_Resetting = false;
}

void CGameWorld::CleanupEntities()
{
	// destroy objects marked for destruction
	m_DestroyList.DeleteAll();
}

void CGameWorld::Tick()
{
	if(m_Resetting)
		Reset();

	if(!m_Paused)
	{
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntityList::CIterator e = m_aLists[i]; e.Exists(); e.Next())
				e.Get()->Tick();

		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntityList::CIterator e = m_aLists[i]; e.Exists(); e.Next())
				e.Get()->TickDefered();
	}
	else if(GameServer()->m_pController->IsGamePaused())
	{
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntityList::CIterator e = m_aLists[i]; e.Exists(); e.Next())
				e.Get()->TickPaused();
	}

	CleanupEntities();
}

void CGameWorld::Snap(int SnappingClient)
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntityList::CIterator e = m_aLists[i]; e.Exists(); e.Next())
			e.Get()->Snap(SnappingClient);
}

void CGameWorld::PostSnap()
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntityList::CIterator e = m_aLists[i]; e.Exists(); e.Next())
			e.Get()->PostSnap();
}

CEntity *CGameWorld::GetFirst(int Type)
{
	dbg_assert(Type >= 0 && Type < NUM_ENTTYPES, "Invalid entity type");
	return m_aLists[Type].GetFirst();
}

int CGameWorld::FindEntities(int Type, vec2 Pos, float Radius, CEntity **ppEnts, int MaxEnts)
{
	dbg_assert(Type >= 0 && Type < NUM_ENTTYPES, "Invalid entity type");

	int Num = 0;
	for(CEntity *pEnt = GetFirst(Type); pEnt; pEnt = pEnt->GetNextEntity())
	{
		float Len = distance(Pos, pEnt->GetPos());
		if(Len < Radius+pEnt->GetProximityRadius())
		{
			if(ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if(Num == MaxEnts)
				break;
		}
	}

	return Num;
}

CEntity *CGameWorld::IntersectEntity(int Type, vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, CEntity *pNotThis)
{
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CEntity *pClosest = 0;

	for(CEntity *p = GetFirst(Type); p; p = p->GetNextEntity())
 	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->GetPos());
		float Len = distance(p->GetPos(), IntersectPos);
		if(Len < p->GetProximityRadius()+Radius)
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

CEntity *CGameWorld::ClosestEntity(int Type, vec2 Pos, float Radius, CEntity *pNotThis)
{
	float ClosestRange = Radius*2;
	CEntity *pClosest = 0;

	for(CEntity *p = GetFirst(Type); p; p = p->GetNextEntity())
 	{
		if(p == pNotThis)
			continue;

		float Len = distance(Pos, p->GetPos());
		if(Len < p->GetProximityRadius()+Radius)
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
