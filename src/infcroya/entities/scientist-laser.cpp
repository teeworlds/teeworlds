/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include "scientist-laser.h"


// #include "white-hole.h"
#include "growingexplosion.h"
#include <game/server/entities/character.h>

CScientistLaser::CScientistLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Dmg)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos)
{
	m_Dmg = Dmg;
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	GameWorld()->InsertEntity(this);
	DoBounce();
}


bool CScientistLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, m_OwnerChar);
	if(!pHit)
		return false;

	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	
	return true;
}

void CScientistLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir * m_Energy;

	if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}
	
	GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_LASER, m_Dmg);
	
	//Create a white hole entity
	//if(m_OwnerChar && m_OwnerChar->m_HasWhiteHole)
	//{
	new CGrowingExplosion(GameWorld(), m_Pos, vec2(0.0, -1.0), m_Owner, 1, GROWINGEXPLOSIONEFFECT_BOOM_INFECTED);
	/*
	new CWhiteHole(GameWorld(), To, m_Owner);
		
	//Make it unavailable
	m_OwnerChar->m_HasWhiteHole = false;
	m_OwnerChar->m_HasIndicator = false;
	m_OwnerChar->GetPlayer()->ResetNumberKills();
	*/
	//}
}

void CScientistLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CScientistLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CScientistLaser::TickPaused()
{
	++m_EvalTick;
}

void CScientistLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
