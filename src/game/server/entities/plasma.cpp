/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/server.h>
#include <engine/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "plasma.h"

const float ACCEL=1.1f;


//////////////////////////////////////////////////
// turret
//////////////////////////////////////////////////
CPlasma::CPlasma(CGameWorld *pGameWorld, vec2 pos,vec2 dir)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	this->m_Pos = pos;
	this->core = dir;
	this->eval_tick=Server()->Tick();
	this->lifetime=Server()->TickSpeed()*1.5;
	GameWorld()->InsertEntity(this);
}

bool CPlasma::hit_character()
{
	vec2 to2;
	CCharacter *hit = GameServer()->m_World.IntersectCharacter(m_Pos, m_Pos+core, 0.0f,to2);
	if(!hit)
		return false;
	hit->Freeze(Server()->TickSpeed()*3);
	GameServer()->m_World.DestroyEntity(this);
	return true;
}

void CPlasma::move()
{
	m_Pos += core;
	core *= ACCEL;
}
	
void CPlasma::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CPlasma::Tick()
{
	if (lifetime==0)
	{
		Reset();
		return;
	}
	lifetime--;
	move();
	hit_character();

	int res=0;
	res = GameServer()->Collision()->IntersectNolaser(m_Pos, m_Pos+core,0, 0);
	if(res)
	{
		GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, false);
		Reset();
	}
	
}

void CPlasma::Snap(int snapping_client)
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
