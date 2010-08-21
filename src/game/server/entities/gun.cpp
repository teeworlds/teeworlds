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
CGun::CGun(CGameWorld *pGameWorld, vec2 m_Pos)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	DELAY=Server()->TickSpeed()*0.3f;
	this->m_Pos = m_Pos;
	this->eval_tick=Server()->Tick();
	
	GameWorld()->InsertEntity(this);
}


void CGun::fire()
{
	CCharacter *ents[16];
	int num = -1;
	num =  GameServer()->m_World.FindEntities(m_Pos,RANGE, (CEntity**)ents, 16, NETOBJTYPE_CHARACTER);
	int id=-1;
	int minlen=0;
	for (int i = 0; i < num; i++)
	{
		CCharacter *target = ents[i];
		int res=0;
		vec2 coltile;
		res = GameServer()->Collision()->IntersectLine(m_Pos, target->m_Pos,0,0,false);
		if (!res)
		{
			int len=length(ents[i]->m_Pos - m_Pos);
			if (minlen==0)
			{
				minlen=len;
				id=i;
			}
			else if(minlen>len)
			{
				minlen=len;
				id=i;
			}
		}
	}
	if (id!=-1)
	{
		CCharacter *target = ents[id];
		vec2 fdir = normalize(target->m_Pos - m_Pos);
		new CPlasma(&GameServer()->m_World, m_Pos,fdir);
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
		eval_tick=Server()->Tick();
		int index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y);
		if (index)
		{
			core=GameServer()->Collision()->CpSpeed(index);
		}
		m_Pos+=core;
		fire();
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
	pObj->m_StartTick = eval_tick;
}
