/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/server.h>
#include <engine/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "gun.h"
#include "plasma.h"


const int RANGE=700;

//////////////////////////////////////////////////
// CGun
//////////////////////////////////////////////////
CGun::CGun(CGameWorld *pGameWorld, vec2 Pos, bool Freeze, bool Explosive, int Layer, int Number)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Layer = Layer;
	m_Number = Number;
	m_Delay = Server()->TickSpeed()*0.3f;
	m_Pos = Pos;
	m_EvalTick = Server()->Tick();
	m_Freeze = Freeze;
	m_Explosive = Explosive;
	
	GameWorld()->InsertEntity(this);
}


void CGun::Fire()
{
	CCharacter *Ents[16];
	int IdInTeam[16]; 
	int LenInTeam[16];
	for (int i = 0; i < 16; i++) {
		IdInTeam[i] = -1;
		LenInTeam[i] = 0;
	}
	
	int Num = -1;
	Num =  GameServer()->m_World.FindEntities(m_Pos, RANGE, (CEntity**)Ents, 16, NETOBJTYPE_CHARACTER);

	for (int i = 0; i < Num; i++)
	{
		CCharacter *Target = Ents[i];
		//now gun doesn't affect on super
		if(Target->Team() == TEAM_SUPER) continue;
		if(m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Target->Team()]) continue;
		int res = GameServer()->Collision()->IntersectLine(m_Pos, Target->m_Pos,0,0,false);
		if (!res)
		{
			int Len = length(Target->m_Pos - m_Pos);
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
			new CPlasma(&GameServer()->m_World, m_Pos, normalize(Target->m_Pos - m_Pos), m_Freeze, m_Explosive, i);
		}
	}
	
}
	
void CGun::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CGun::Tick()
{
	if (Server()->Tick()%int(Server()->TickSpeed()*0.15f)==0)
	{
		int Flags;
		m_EvalTick=Server()->Tick();
		int index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y, &Flags);
		if (index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(index, Flags);
		}
		m_Pos+=m_Core;
		Fire();
	}

}

void CGun::Snap(int SnappingClient)
{	
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter * SnapChar = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%11;
	if (SnapChar && SnapChar->m_Alive && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[SnapChar->Team()] && (!Tick)) return;
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = m_EvalTick;
}
