/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/config.h>
#include <engine/server.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "dragger.h"

//////////////////////////////////////////////////
// CDragger
//////////////////////////////////////////////////

const int LENGTH=700;

CDragger::CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW, int CatchedTeam)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Pos = Pos;
	m_Strength = Strength;
	m_EvalTick = Server()->Tick();
	m_NW = NW;
	m_CatchedTeam = CatchedTeam;
	GameWorld()->InsertEntity(this);
}

void CDragger::Move()
{
	if (m_Target)
		return;
	CCharacter *Ents[16];
	int Num = -1;
	Num =  GameServer()->m_World.FindEntities(m_Pos,LENGTH, (CEntity**)Ents, 16, NETOBJTYPE_CHARACTER);
	int Id=-1;
	int MinLen=0;
	for (int i = 0; i < Num; i++)
	{
		m_Target = Ents[i];
		if(m_Target->Team() != m_CatchedTeam) continue;
		int Res=0;
		if (!m_NW)
			Res = GameServer()->Collision()->IntersectNoLaser(m_Pos, m_Target->m_Pos, 0, 0);
		else
			Res = GameServer()->Collision()->IntersectNoLaserNW(m_Pos, m_Target->m_Pos, 0, 0);

		if (Res==0)
		{
			int Len=length(Ents[i]->m_Pos - m_Pos);
			if (MinLen==0 || MinLen>Len)
			{
				MinLen=Len;
				Id=i;
			}
		}
	}
	if (Id!=-1)
	{
		m_Target = Ents[Id];
	}
	else
	{
		m_Target=0;
	}
}

void CDragger::Drag()
{
	if (m_Target)
	{

		int Res = 0;
		if (!m_NW)
			Res = GameServer()->Collision()->IntersectNoLaser(m_Pos, m_Target->m_Pos, 0, 0);
		else
			Res = GameServer()->Collision()->IntersectNoLaserNW(m_Pos, m_Target->m_Pos, 0, 0);
		if (Res || length(m_Pos-m_Target->m_Pos)>700)
		{
			m_Target=0;
		}
		else
			if (length(m_Pos-m_Target->m_Pos)>28)
				if(!((m_Target->m_CurrentTile >= TILE_STOPL && m_Target->m_CurrentTile <= TILE_STOPT) || (m_Target->m_CurrentFTile >= TILE_STOPL && m_Target->m_CurrentFTile <= TILE_STOPT)))
					m_Target->m_Core.m_Vel+=normalize(m_Pos-m_Target->m_Pos)*m_Strength;
	}
}

void CDragger::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CDragger::Tick()
{
	if (Server()->Tick()%int(Server()->TickSpeed()*0.15f)==0)
	{
		m_EvalTick=Server()->Tick();
		int Index = GameServer()->Collision()->IsCp(m_Pos.x, m_Pos.y);
		if (Index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(Index);
		}
		m_Pos+=m_Core;
		Move();
	}
	Drag();
	return;


}

void CDragger::Snap(int SnappingClient)
{
	if (m_Target)
	{
		if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient,m_Target->m_Pos))
			return;
	}
	else
		if(NetworkClipped(SnappingClient,m_Pos))
			return;

	CCharacter * Char = GameServer()->GetPlayerChar(SnappingClient);
	if(Char && m_Target 
		&& !Char->GetPlayer()->m_ShowOthers 
		&& Char->Team() != m_Target->Team()) return;

	CNetObj_Laser *obj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));

	obj->m_X = (int)m_Pos.x;
	obj->m_Y = (int)m_Pos.y;
	if (m_Target)
	{
		obj->m_FromX = (int)m_Target->m_Pos.x;
		obj->m_FromY = (int)m_Target->m_Pos.y;
	}
	else
	{
		obj->m_FromX = (int)m_Pos.x;
		obj->m_FromY = (int)m_Pos.y;
	}


	int StartTick = m_EvalTick;
	if (StartTick < Server()->Tick() - 4)
		StartTick = Server()->Tick() - 4;
	else if (StartTick>Server()->Tick())
		StartTick = Server()->Tick();
	obj->m_StartTick = StartTick;
}

CDraggerTeam::CDraggerTeam(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW) {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_Draggers[i] = new CDragger(pGameWorld, Pos, Strength, NW, i);
	}
}

CDraggerTeam::~CDraggerTeam() {
	//for(int i = 0; i < MAX_CLIENTS; ++i) {
	//	delete m_Draggers[i];
	//}
}
//? ??? ???
//? ????
