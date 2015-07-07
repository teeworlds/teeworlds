/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/gamecontext.h>

#include "character.h"
#include "laser.h"

CLaser::CLaser(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir)
: CEntity(pGameWorld, ENTTYPE_LASER, Pos)
{
	m_Owner = Owner;

	m_Dir = Dir;
	m_Energy = GameServer()->Tuning()->m_LaserReach;
	m_Bounces = 0;

	DoBounce();
}

void CLaser::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CLaser::DepleteEnergy()
{
	m_Energy = -1;
}

bool CLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameWorld()->IntersectEntity<CCharacter>(m_Pos, To, 0.0f, At, pOwnerChar);
	if(!pHit)
		return false;

	m_From = From;
	m_Pos = At;
	DepleteEnergy();

	int Damage = g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage;
	pHit->TakeDamage(vec2(0, 0), Damage, m_Owner, WEAPON_LASER);

	return true;
}

void CLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	// check energy
	if(m_Energy < 0)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	// guess new position
	vec2 To = m_Pos + m_Dir * m_Energy;

	// check ground collision
	bool Collide = GameServer()->Collision()->IntersectLine(m_Pos, To, 0, &To);

	// check player collision
	if(HitCharacter(m_Pos, To))
		return;

	// update position
	m_From = m_Pos;
	m_Pos = To;

	if(Collide) // intersected with ground
	{
		// find new direction
		vec2 TempPos = m_Pos;
		vec2 TempDir = m_Dir * 4.0f;

		GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);

		m_Pos = TempPos;
		m_Dir = normalize(TempDir);

		// update state
		m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;

		// check bounces amount
		m_Bounces++;
		if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
			DepleteEnergy();

		// play sound
		GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE);
	}
	else // not intersected with ground
	{
		// affect energy
		DepleteEnergy();
	}
}

void CLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CLaser::TickPaused()
{
	m_EvalTick++;
}

void CLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_From, m_Pos))
		return;

	CNetObj_Laser *pLaser = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pLaser)
		return;

	pLaser->m_X = (int)m_Pos.x;
	pLaser->m_Y = (int)m_Pos.y;
	pLaser->m_FromX = (int)m_From.x;
	pLaser->m_FromY = (int)m_From.y;
	pLaser->m_StartTick = m_EvalTick;
}
