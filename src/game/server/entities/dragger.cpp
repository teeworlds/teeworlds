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

CDragger::CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW)
: CEntity(pGameWorld, NETOBJTYPE_LASER), m_Targets()
{
	m_Pos = Pos;
	m_Strength = Strength;
	m_EvalTick = Server()->Tick();
	m_NW = NW;
	
	GameWorld()->InsertEntity(this);
}

void CDragger::Move()
{
	if (m_Targets.empty())
		return;
	CCharacter *Ents[16];
	int IdInTeam[16]; 
	int LenInTeam[16];
	for (int i = 0; i < 16; i++) {
		IdInTeam[i] = -1;
		LenInTeam[i] = 0;
	}
	int Num = -1;
	Num =  GameServer()->m_World.FindEntities(m_Pos,LENGTH, (CEntity**)Ents, 16, NETOBJTYPE_CHARACTER);
	for (int i = 0; i < Num; i++)
	{
		CCharacter * Target = Ents[i];
		int Res = m_NW ? GameServer()->Collision()->IntersectNoLaserNW(m_Pos, Target->m_Pos, 0, 0) :
			GameServer()->Collision()->IntersectNoLaser(m_Pos, Target->m_Pos, 0, 0);

		if (Res==0)
		{
			int Len=length(Target->m_Pos - m_Pos);
			if (LenInTeam[Target->Team()] == 0 || LenInTeam[Target->Team()] > Len)
			{
				LenInTeam[Target->Team()] = Len;
				IdInTeam[Target->Team()] = i;
			}
		}
	}
	for (int i = 0; i < 16; i++) {
		if(IdInTeam[i] != -1) {
			CCharacter *Target = Ents[IdInTeam[i]];
			m_Targets.push_back(Target);
		}
	}
}

void CDragger::Drag()
{
	if (!m_Targets.empty())
	{
/*
		int Res = m_NW ? GameServer()->Collision()->IntersectNoLaserNW(m_Pos, m_Target->m_Pos, 0, 0) :
			GameServer()->Collision()->IntersectNoLaser(m_Pos, m_Target->m_Pos, 0, 0);
		if (Res || length(m_Pos-m_Target->m_Pos)>700)//TODO: USE CONSTANTS IDIOT
		{
			m_Targets.clear();
		}
		else {
			for(std::list<CCharacter * >::iterator i = m_Targets.begin(); i != m_Targets.end(); ++i) {
				CCharacter * Target = *i;
				if (length(m_Pos-Target->m_Pos)>28) {
					if(((m_Target->m_TileIndexL == TILE_STOPA || m_Target->m_TileFIndexL == TILE_STOPA || m_Target->m_TileIndex == TILE_STOPL || m_Target->m_TileIndexL == TILE_STOPL || m_Target->m_TileFIndex == TILE_STOPL || m_Target->m_TileFIndexL == TILE_STOPL || m_Target->m_TileIndexL == TILE_STOPH || m_Target->m_TileFIndexL == TILE_STOPH)) || ((m_Target->m_TileIndexR == TILE_STOPA || m_Target->m_TileFIndexR == TILE_STOPA || m_Target->m_TileIndex == TILE_STOPR || m_Target->m_TileIndexR == TILE_STOPR || m_Target->m_TileFIndex == TILE_STOPR || m_Target->m_TileFIndexR == TILE_STOPR || m_Target->m_TileIndexR == TILE_STOPH || m_Target->m_TileFIndexR == TILE_STOPH)))
					{
						m_Target->m_Core.m_Vel.y +=(normalize(m_Pos-m_Target->m_Pos)*m_Strength).y;
						//dbg_msg("","x");
						return;
					}
					if(((m_Target->m_TileIndexB == TILE_STOPA || m_Target->m_TileFIndexB == TILE_STOPA || m_Target->m_TileIndex == TILE_STOPB || m_Target->m_TileIndexB == TILE_STOPB || m_Target->m_TileFIndex == TILE_STOPB || m_Target->m_TileFIndexB == TILE_STOPB|| m_Target->m_TileIndexB == TILE_STOPV || m_Target->m_TileFIndexB == TILE_STOPV)) || ((m_Target->m_TileIndexT == TILE_STOPA || m_Target->m_TileFIndexT == TILE_STOPA || m_Target->m_TileIndex == TILE_STOPT || m_Target->m_TileIndexT == TILE_STOPT || m_Target->m_TileFIndex == TILE_STOPT || m_Target->m_TileFIndexT == TILE_STOPT || m_Target->m_TileIndexT == TILE_STOPV || m_Target->m_TileFIndexT == TILE_STOPV)))
					{
						m_Target->m_Core.m_Vel.x +=(normalize(m_Pos-m_Target->m_Pos)*m_Strength).x;
						//dbg_msg("y","");
						return;
					}
				//m_Target->m_Core.m_Vel = Temp;
					m_Target->m_Core.m_Vel +=(normalize(m_Pos-m_Target->m_Pos)*m_Strength);
			}
	}
}

void CDragger::Reset()
{
	m_Targets.clear();
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
	/*
	if (m_Targets.empty())
	{
		for(std::list<CCharacter * >::iterator i = m_Targets.begin(); i != m_Targets.end(); ++i) {
			if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient,(*i)->m_Pos))
				return;
		}
	}
	else
		if(NetworkClipped(SnappingClient,m_Pos))
			return;

	CCharacter * Char = GameServer()->GetPlayerChar(SnappingClient);
	if(Char && m_Target && !Char->GetPlayer()->m_ShowOthers && Char->Team() != m_Target->Team()) return;

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
	*/
}
//я тут был
//я тоже
