/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "biologist-laser.h"
#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>

CBiologistLaser::CBiologistLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, int Owner, int Dmg)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos)
{
	m_Dmg = Dmg;
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = 400.0f;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
	DoBounce();
}


void CBiologistLaser::HitCharacter(vec2 From, vec2 To)
{
	for(CCharacter *p = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
	{
		if(p->IsHuman()) continue;
		vec2 IntersectPos = closest_point_on_line(From, To, p->GetPos());
		float Len = distance(p->GetPos(), IntersectPos);
		if(Len < p->GetProximityRadius())
		{
			p->TakeDamage(vec2(0.f, 0.f), vec2(p->GetPos().x, p->GetPos().y), 10, m_Owner, WEAPON_LASER);
			break;
		}
	}
}

void CBiologistLaser::DoBounce()
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
		HitCharacter(m_Pos, To);
		
		// intersected
		m_From = m_Pos;
		m_Pos = To;

		vec2 TempPos = m_Pos;
		vec2 TempDir = m_Dir * 4.0f;

		GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
		m_Pos = TempPos;
		m_Dir = normalize(TempDir);

		m_Energy += 100.0f;
		m_Energy -= max(0.0f, distance(m_From, m_Pos));
		m_Bounces++;

		if(m_Bounces > 4)
			m_Energy = -1;

		GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE);
	}
	else
	{
		HitCharacter(m_Pos, To);
		
		m_From = m_Pos;
		m_Pos = To;
		m_Energy = -1;
	}
}

void CBiologistLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CBiologistLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CBiologistLaser::TickPaused()
{
	++m_EvalTick;
}

void CBiologistLaser::Snap(int SnappingClient)
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
