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
CGun::CGun(CGameWorld *pGameWorld, vec2 Pos, bool Freeze, bool Explosive)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
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
	int Num = -1;
	Num =  GameServer()->m_World.FindEntities(m_Pos,RANGE, (CEntity**)Ents, 16, NETOBJTYPE_CHARACTER);
	int Id=-1;
	int MinLen=0;
	for (int i = 0; i < Num; i++)
	{
		CCharacter *Target = Ents[i];
		int res=0;
		vec2 coltile;
		res = GameServer()->Collision()->IntersectLine(m_Pos, Target->m_Pos,0,0,false);
		if (!res)
		{
			int Len=length(Ents[i]->m_Pos - m_Pos);
			if (MinLen==0)
			{
				MinLen=Len;
				Id=i;
			}
			else if(MinLen>Len)
			{
				MinLen=Len;
				Id=i;
			}
		}
	}
	if (Id!=-1)
	{
		vec2 Fdir = normalize(Ents[Id]->m_Pos - m_Pos);
		new CPlasma(&GameServer()->m_World, m_Pos, Fdir, m_Freeze, m_Explosive);
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
		m_EvalTick=Server()->Tick();
		int Index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y);
		if (Index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(Index);
		}
		m_Pos+=m_Core;
		Fire();
	}

}

void CGun::Snap(int snapping_client)
{	
	if(NetworkClipped(snapping_client))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = m_EvalTick;
}
