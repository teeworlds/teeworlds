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



CLight::CLight(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Tick=(Server()->TickSpeed()*0.15f);
	m_Pos = Pos;
	m_Rotation = Rotation;
	m_Length = Length;
	m_EvalTick = Server()->Tick();
	GameWorld()->InsertEntity(this);
	Step();
}


bool CLight::HitCharacter()
{
	vec2 At;
	CCharacter *Hit = GameServer()->m_World.IntersectCharacter(m_Pos, m_To, 1.f, At, 0);
	if(Hit)
	{
		Hit->Freeze(Server()->TickSpeed()*3);
		vec2 Points[38];
		Hit = 0;
		for(int i=0;i<38;i++)
		{
			Points[i].x = m_Pos.x * (1 - (i/37.0)) + m_To.x * (i / 37.0);
			Points[i].y = m_Pos.y * (1 - (i/37.0)) + m_To.y * (i / 37.0);
			//if(i == 0 || i == 37 || i == 19) dbg_msg("CLight","(%d)\nPos(%f,%f)\nTo(%f,%f)\nPoint(%f,%f)",i,m_Pos.x,m_Pos.y,m_To.x,m_To.y,Points[i].x,Points[i].y);
		}
		for(int i = 0; i < 38; i++)
		{
			Hit = GameServer()->m_World.IntersectCharacter(Points[i], Points[i+1], 1.f, At, 0);
			if(Hit)
				Hit->Freeze(Server()->TickSpeed()*3);
			Hit = 0;
			/*Hit = GameServer()->m_World.IntersectCharacter(Points[i+1], Points[i], 1.f, At, 0);
			if(Hit)
				Hit->Freeze(Server()->TickSpeed()*3);
			Hit = 0;*/
		}
	}
	return true;
}

void CLight::Move()
{
	if (m_Speed != 0)
	{
		if ((m_CurveLength>=m_Length && m_Speed>0) || (m_CurveLength<=0 && m_Speed<0))
				m_Speed=-m_Speed;
		m_CurveLength+=m_Speed*m_Tick + m_LengthL;
		m_LengthL=0;
		if (m_CurveLength>m_Length)
		{
			m_LengthL=m_CurveLength-m_Length;
			m_CurveLength=m_Length;
		}
		else if(m_CurveLength<0)
		{
			m_LengthL=0+m_CurveLength;
			m_CurveLength=0;
		}
	}

	m_Rotation+=m_AngularSpeed*m_Tick;
	if (m_Rotation>pi*2)
		m_Rotation-=pi*2;
	else if(m_Rotation<0)
		m_Rotation+=pi*2;
}

void CLight::Step()
{
	Move();
	vec2 dir(sin(m_Rotation), cos(m_Rotation));
	vec2 to2 = m_Pos + normalize(dir)*m_CurveLength;
	GameServer()->Collision()->IntersectNoLaser(m_Pos, to2, &m_To,0 );
}
	
void CLight::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLight::Tick()
{
	
	if (Server()->Tick()%int(Server()->TickSpeed()*0.15f)==0)
	{
		m_EvalTick=Server()->Tick();
		int index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y);
		if (index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(index);
		}
		m_Pos+=m_Core;
		Step();
	}

	HitCharacter();
	return;


}

void CLight::Snap(int snapping_client)
{
	if(NetworkClipped(snapping_client,m_Pos) && NetworkClipped(snapping_client,m_To))
		return;

	
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_To.x;
	pObj->m_FromY = (int)m_To.y;


	int start_tick = m_EvalTick;
	if (start_tick<Server()->Tick()-4)
		start_tick=Server()->Tick()-4;
	else if (start_tick>Server()->Tick())
		start_tick=Server()->Tick();
	pObj->m_StartTick = start_tick;
}
