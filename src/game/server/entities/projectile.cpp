/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>

#include "character.h"
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Dir, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, Pos)
{
	m_Type = Type;
	m_Owner = Owner;
	m_Dir = Dir;

	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	return CalcPos(m_Pos, m_Dir, GetCurvature(), GetSpeed(), Time);
}

void CProjectile::Tick()
{
	float PrevTime = (Server()->Tick()-m_StartTick-1) / (float)Server()->TickSpeed();
	float CurTime = (Server()->Tick()-m_StartTick) / (float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(PrevTime);
	vec2 CurPos = GetPos(CurTime);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pTargetChar = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, pOwnerChar);

	if(pTargetChar)
	{
		if(OnCharacterHit(CurPos, pTargetChar))
		{
			GameServer()->m_World.DestroyEntity(this);
			return;
		}
	}

	if(Collide || GameLayerClipped(CurPos))
	{
		if(OnGroundHit(CurPos))
		{
			GameServer()->m_World.DestroyEntity(this);
			return;
		}
	}

	if(GetLifeSpan() > 0.0f && Server()->Tick() > m_StartTick + Server()->TickSpeed() * GetLifeSpan())
	{
		OnLifeOver(CurPos);
		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	m_StartTick++;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj) const
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Dir.x*100.0f);
	pProj->m_VelY = (int)(m_Dir.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
