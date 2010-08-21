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

CDragger::CDragger(CGameWorld *pGameWorld, vec2 pos, float strength, bool nw)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	this->m_Pos = pos;
	this->strength = strength;
	this->eval_tick=Server()->Tick();
	this->nw=nw;
	GameWorld()->InsertEntity(this);
}

void CDragger::move()
{
	if (target)
		return;
	CCharacter *ents[16];
	int num = -1;
	num =  GameServer()->m_World.FindEntities(m_Pos,LENGTH, (CEntity**)ents, 16, NETOBJTYPE_CHARACTER);
	int id=-1;
	int minlen=0;
	for (int i = 0; i < num; i++)
	{
		target = ents[i];
		int res=0;
		if (!nw)
			res = GameServer()->Collision()->IntersectNoLaser(m_Pos, target->m_Pos, 0, 0);
		else
			res = GameServer()->Collision()->IntersectNoLaserNW(m_Pos, target->m_Pos, 0, 0);

		if (res==0)
		{
			int len=length(ents[i]->m_Pos - m_Pos);
			if (minlen==0 || minlen>len)
			{
				minlen=len;
				id=i;
			}
		}
	}
	if (id!=-1)
	{
		target = ents[id];
	}
	else
	{
		target=0;
	}
}

void CDragger::drag()
{
	if (target)
	{

		int res = 0;
		if (!nw)
			res = GameServer()->Collision()->IntersectNoLaser(m_Pos, target->m_Pos, 0, 0);

		if (res || length(m_Pos-target->m_Pos)>700)
		{
			target=0;
		}
		else
			if (length(m_Pos-target->m_Pos)>28)
			{
				target->m_Core.m_Vel+=normalize(m_Pos-target->m_Pos)*strength;
			}
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
		eval_tick=Server()->Tick();
		int index = GameServer()->Collision()->IsCp(m_Pos.x, m_Pos.y);
		if (index)
		{
			core=GameServer()->Collision()->CpSpeed(index);
		}
		m_Pos+=core;
		move();
	}
	drag();
	return;


}

void CDragger::Snap(int snapping_client)
{
	if (target)
	{
		if(NetworkClipped(snapping_client, m_Pos) && NetworkClipped(snapping_client,target->m_Pos))
			return;
	}
	else
		if(NetworkClipped(snapping_client,m_Pos))
			return;

	CNetObj_Laser *obj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));

	obj->m_X = (int)m_Pos.x;
	obj->m_Y = (int)m_Pos.y;
	if (target)
	{
		obj->m_FromX = (int)target->m_Pos.x;
		obj->m_FromY = (int)target->m_Pos.y;
	}
	else
	{
		obj->m_FromX = (int)m_Pos.x;
		obj->m_FromY = (int)m_Pos.y;
	}


	int start_tick = eval_tick;
	if (start_tick<Server()->Tick()-4)
		start_tick=Server()->Tick()-4;
	else if (start_tick>Server()->Tick())
		start_tick=Server()->Tick();
	obj->m_StartTick = start_tick;
}
//я тут был
//я тоже
