/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/vmath.h>
#include <game/server/gamecontext.h>
#include <infcroya/entities/growingexplosion.h>
#include <game/server/entities/character.h>

#include "scatter-grenade.h"

CScatterGrenade::CScatterGrenade(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_SCATTER_GRENADE, Pos)
{
	m_Pos = Pos;
	m_ActualPos = Pos;
	m_ActualDir = Dir;
	m_Direction = Dir;
	m_Owner = Owner;
	m_StartTick = Server()->Tick();
	m_IsFlashGrenade = false;

	GameWorld()->InsertEntity(this);
}

void CScatterGrenade::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CScatterGrenade::GetPos(float Time)
{
	float Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
	float Speed = GameServer()->Tuning()->m_GrenadeSpeed;

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

void CScatterGrenade::TickPaused()
{
	m_StartTick++;
}

void CScatterGrenade::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	
	m_ActualPos = CurPos;
	m_ActualDir = normalize(CurPos - PrevPos);
	
	if(m_IsFlashGrenade) {
		
		CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
		CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);
		
		if(TargetChr)
		{
			Explode();
		}
	}
	
	if(GameLayerClipped(CurPos))
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}
	
	vec2 LastPos;
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, NULL, &LastPos);
	if(Collide)
	{
		
		if(m_IsFlashGrenade) {
			Explode();
		}
		
		//Thanks to TeeBall 0.6
		vec2 CollisionPos;
		CollisionPos.x = LastPos.x;
		CollisionPos.y = CurPos.y;
		int CollideY = GameServer()->Collision()->IntersectLine(PrevPos, CollisionPos, NULL, NULL);
		CollisionPos.x = CurPos.x;
		CollisionPos.y = LastPos.y;
		int CollideX = GameServer()->Collision()->IntersectLine(PrevPos, CollisionPos, NULL, NULL);
		
		m_Pos = LastPos;
		m_ActualPos = m_Pos;
		vec2 vel;
		vel.x = m_Direction.x;
		vel.y = m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*Ct*GameServer()->Tuning()->m_GrenadeSpeed;
		
		if (CollideX && !CollideY)
		{
			m_Direction.x = -vel.x;
			m_Direction.y = vel.y;
		}
		else if (!CollideX && CollideY)
		{
			m_Direction.x = vel.x;
			m_Direction.y = -vel.y;
		}
		else
		{
			m_Direction.x = -vel.x;
			m_Direction.y = -vel.y;
		}
		
		m_Direction.x *= (100 - 50) / 100.0;
		m_Direction.y *= (100 - 50) / 100.0;
		m_StartTick = Server()->Tick();
		
		m_ActualDir = normalize(m_Direction);
	}
}

void CScatterGrenade::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = WEAPON_GRENADE;
}

void CScatterGrenade::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	
	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;
	
	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
	
void CScatterGrenade::Explode()
{
	if(m_IsFlashGrenade)
	{
		new CGrowingExplosion(GameWorld(), m_ActualPos, m_ActualDir, m_Owner, 4, GROWINGEXPLOSIONEFFECT_FREEZE_INFECTED);
	}
	else
	{
		new CGrowingExplosion(GameWorld(), m_ActualPos, m_ActualDir, m_Owner, 4, GROWINGEXPLOSIONEFFECT_POISON_INFECTED);
	}
	
	GameServer()->m_World.DestroyEntity(this);
	
}

void CScatterGrenade::FlashGrenade()
{
	m_IsFlashGrenade = true;
}
