/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/config.h>
#include <engine/server.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "light.h"
#include <game/mapitems.h>

//////////////////////////////////////////////////
// CLight
//////////////////////////////////////////////////



CLight::CLight(CGameWorld *pGameWorld, vec2 pos, float rotation, int length)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	TICK=(Server()->TickSpeed()*0.15f);
	this->m_Pos = pos;
	this->rotation = rotation;
	this->length = length;
	this->eval_tick=Server()->Tick();
	GameWorld()->InsertEntity(this);
	step();
}


bool CLight::hit_character()
{
	vec2 nothing;
	CCharacter *hit = GameServer()->m_World.IntersectCharacter(m_Pos, to, 0.0f, nothing, 0);
	if(!hit)
		return false;
	hit->Freeze(Server()->TickSpeed()*3);
	return true;

}

void CLight::move()
{
	if (speed != 0)
	{
		if ((cur_length>=length && speed>0) || (cur_length<=0 && speed<0))
				speed=-speed;
		cur_length+=speed*TICK + length_l;
		length_l=0;
		if (cur_length>length)
		{
			length_l=cur_length-length;
			cur_length=length;
		}
		else if(cur_length<0)
		{
			length_l=0+cur_length;
			cur_length=0;
		}
	}
	
	rotation+=ang_speed*TICK;
	if (rotation>pi*2)
		rotation-=pi*2;
	else if(rotation<0)
		rotation+=pi*2;
}

void CLight::step()
{
	move();
	vec2 dir(sin(rotation), cos(rotation));
	vec2 to2 = m_Pos + normalize(dir)*cur_length;
	GameServer()->Collision()->IntersectNolaser(m_Pos, to2, &to,0 );
}
	
void CLight::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLight::Tick()
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
		step();
	}

	hit_character();
	return;


}

void CLight::Snap(int snapping_client)
{
	if(NetworkClipped(snapping_client,m_Pos) && NetworkClipped(snapping_client,to))
		return;

	
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)to.x;
	pObj->m_FromY = (int)to.y;


	int start_tick = eval_tick;
	if (start_tick<Server()->Tick()-4)
		start_tick=Server()->Tick()-4;
	else if (start_tick>Server()->Tick())
		start_tick=Server()->Tick();
	pObj->m_StartTick = start_tick;
}
