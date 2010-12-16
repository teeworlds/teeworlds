/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <game/server/teams.h>
#include "projectile.h"

CProjectile::CProjectile
	(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
		bool Freeze,
		bool Explosive,
		float Force,
		int SoundImpact,
		int Weapon,
		int Layer,
		int Number
	)
: CEntity(pGameWorld, NETOBJTYPE_PROJECTILE)
{
	m_Layer = Layer;
	m_Number = Number;
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	//m_Damage = Damage;
	m_Freeze = Freeze;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	if(m_LifeSpan > -2)
		GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;
	
	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;
			
		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;
			
		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}
	
	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

void CProjectile::SetBouncing(int Value)
{
	m_Bouncing = Value;
}

void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	vec2 ColPos;
	vec2 NewPos;
	vec2 Speed = CurPos - PrevPos;
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos, false);
	CCharacter *OwnerChar = 0;
	

	
	if(m_Owner >= 0)
		OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, (m_Freeze) ? 1.0f : 6.0f, ColPos, OwnerChar);

	if(m_LifeSpan > -1)
		m_LifeSpan--;
	
	int TeamMask = -1;
	bool isWeaponCollide = false;
	if
	(
			OwnerChar &&
			TargetChr &&
			OwnerChar->m_Alive &&
			TargetChr->m_Alive &&
			!TargetChr->CanCollide(m_Owner)
			)
	{
			isWeaponCollide = true;
			//TeamMask = OwnerChar->Teams()->TeamMask( OwnerChar->Team());
	}
	if (OwnerChar &&
		OwnerChar->m_Alive)
	{
			TeamMask = OwnerChar->Teams()->TeamMask( OwnerChar->Team());
	}
	if( ((TargetChr && (g_Config.m_SvHit || m_Owner == -1 || TargetChr == OwnerChar)) || Collide) && !isWeaponCollide)//TODO:TEAM
	{
		if(m_Explosive/*??*/ && (!TargetChr || (TargetChr && !m_Freeze)))
		{
			GameServer()->CreateExplosion(ColPos, m_Owner, m_Weapon, m_Owner == -1, (!TargetChr ? -1 : TargetChr->Team()),
			(m_Owner != -1)? TeamMask : -1);
			GameServer()->CreateSound(ColPos, m_SoundImpact, 
			(m_Owner != -1)? TeamMask : -1);
		}
		else if(TargetChr && m_Freeze && ((m_Layer == LAYER_SWITCH && GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[TargetChr->Team()]) || m_Layer != LAYER_SWITCH))
			TargetChr->Freeze(Server()->TickSpeed()*3);
		if(Collide && m_Bouncing != 0)
		{
			m_StartTick = Server()->Tick();
			m_Pos = NewPos+(-(m_Direction*4));
			if (m_Bouncing == 1)
				m_Direction.x = -m_Direction.x;
			else if(m_Bouncing == 2)
				m_Direction.y =- m_Direction.y;
			m_Pos += m_Direction;
		}
		else if (m_Weapon == WEAPON_GUN)
		{
			GameServer()->CreateDamageInd(CurPos, -atan2(m_Direction.x, m_Direction.y), 10, (m_Owner != -1)? TeamMask : -1);
			GameServer()->m_World.DestroyEntity(this);
		}
		else
			if (!m_Freeze)
				GameServer()->m_World.DestroyEntity(this);
	}
	 if (m_LifeSpan == -1)  		 
	{
		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	
	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;
	CCharacter * SnapChar = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%((m_Explosive)?6:20);
	if (SnapChar && SnapChar->m_Alive && (m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[SnapChar->Team()] && (!Tick))) return;

	if
	(
			SnapChar &&
			m_Owner != -1 &&
			!SnapChar->CanCollide(m_Owner)
			)
		return;
	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_Id, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
