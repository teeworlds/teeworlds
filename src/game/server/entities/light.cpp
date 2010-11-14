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



CLight::CLight(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length, int Layer, int Number)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Layer = Layer;
	m_Number = Number;
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
	std::list < CCharacter * > HitCharacters = GameServer()->m_World.IntersectedCharacters(m_Pos, m_To, 0.0f, 0);
	if(HitCharacters.empty()) return false;
	for(std::list < CCharacter * >::iterator i = HitCharacters.begin(); i != HitCharacters.end(); i++) {
		CCharacter * Char = *i;
		if(m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Char->Team()]) continue;
		Char->Freeze(Server()->TickSpeed()*3);
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
		int Flags;
		m_EvalTick=Server()->Tick();
		int index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y, &Flags);
		if (index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(index, Flags);
		}
		m_Pos+=m_Core;
		Step();
	}

	HitCharacter();
	return;


}

void CLight::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient,m_Pos) && NetworkClipped(SnappingClient,m_To))
		return;

	
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;

	CCharacter * Char = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%6;
	if(!Char) return;
	if (Char->m_Alive && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Char->Team()] && (Tick)) return;

	if(Char->Team() == TEAM_SUPER)
	{
		pObj->m_FromX = (int)m_Pos.x;
		pObj->m_FromY = (int)m_Pos.y;
	}
	else if(GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Char->Team()])
	{
		pObj->m_FromX = (int)m_To.x;
		pObj->m_FromY = (int)m_To.y;
	}
	else if(!GameServer()->Collision()->m_pSwitchers)
	{
		pObj->m_FromX = (int)m_To.x;
		pObj->m_FromY = (int)m_To.y;
	}
	else
	{
		pObj->m_FromX = (int)m_Pos.x;
		pObj->m_FromY = (int)m_Pos.y;
	}


	int start_tick = m_EvalTick;
	if (start_tick<Server()->Tick()-4)
		start_tick=Server()->Tick()-4;
	else if (start_tick>Server()->Tick())
		start_tick=Server()->Tick();
	pObj->m_StartTick = start_tick;
}
