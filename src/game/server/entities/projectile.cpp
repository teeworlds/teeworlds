/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, vec2(round_to_int(Pos.x), round_to_int(Pos.y)))
{
	m_Type = Type;
	m_Direction.x = round_to_int(Dir.x*100.0f) / 100.0f;
	m_Direction.y = round_to_int(Dir.y*100.0f) / 100.0f;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_OwnerTeam = GameServer()->m_apPlayers[Owner]->GetTeam();
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;
	m_Water = false;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CProjectile::LoseOwner()
{
	if(m_OwnerTeam == TEAM_BLUE)
		m_Owner = PLAYER_TEAM_BLUE;
	else
		m_Owner = PLAYER_TEAM_RED;
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;
	float WaterResistance = GameServer()->Tuning()->m_LiquidProjectileResistance;
	switch(m_Type)
	{
		case WEAPON_HARPOON:
			Curvature = GameServer()->Tuning()->m_HarpoonCurvature;
			Speed = GameServer()->Tuning()->m_HarpoonSpeed;
			break;

		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			WaterResistance = GameServer()->Tuning()->m_ShotgunWaterResistance;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
		
	}
	vec2 ReturnPos;
	vec2 TestDirection;
	
	if (m_Water)
	{
		if (m_Water==1)
		{
			ReturnPos = CalcPos(m_Pos, m_Direction, Curvature, Speed * WaterResistance, Time);
		}
		else
		{
			ReturnPos = CalcPos(m_Pos, m_Direction, Curvature * 2, Speed * 0.6, Time);
		}
	}
		
	else
		ReturnPos = CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
	if ((m_Water<=0)&&GameServer()->Collision()->TestBox(vec2(ReturnPos.x, ReturnPos.y), vec2(6.0f, 6.0f), 8))
	{
		m_Pos.x = ReturnPos.x;
		m_Pos.y = ReturnPos.y;
		m_Direction = normalize(CalcPos(m_Pos, m_Direction, Curvature, Speed, Time)- CalcPos(m_Pos, m_Direction, Curvature, Speed, Time-0.001f));
		m_StartTick = Server()->Tick();
		m_Water = 1;
	}
	else if (m_Water>0 && GameServer()->Tuning()->m_LiquidSlowProjectilesAfterWater&& !GameServer()->Collision()->TestBox(vec2(ReturnPos.x, ReturnPos.y), vec2(6.0f, 6.0f), 8))
	{
		m_Pos.x = ReturnPos.x;
		m_Pos.y = ReturnPos.y;
		m_Direction = normalize(CalcPos(m_Pos, m_Direction, Curvature, Speed * GameServer()->Tuning()->m_LiquidProjectileResistance, Time) - CalcPos(m_Pos, m_Direction, Curvature, Speed * GameServer()->Tuning()->m_LiquidProjectileResistance, Time - 0.001f));
		m_StartTick = Server()->Tick();
		m_Water = -1;
	}
	return ReturnPos;
}


void CProjectile::Tick()
{
	bool IsInWater = false;

	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	IsInWater = GameServer()->Collision()->TestBox(vec2(PrevPos.x, PrevPos.y), vec2(6.0f, 6.0f), 8);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameWorld()->IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		if (m_Explosive)
		{
			if (IsInWater)
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, m_Damage * GameServer()->Tuning()->m_LiquidExplosionDamageMultiplier, GameServer()->Tuning()->m_LiquidExplosionRadiusMultiplier);
			else
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, m_Damage);
		}

		else if(TargetChr)
			TargetChr->TakeDamage(m_Direction * maximum(0.001f, m_Force), m_Direction*-1, m_Damage, m_Owner, m_Weapon);

		GameWorld()->DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = round_to_int(m_Pos.x);
	pProj->m_Y = round_to_int(m_Pos.y);
	pProj->m_VelX = round_to_int(m_Direction.x*100.0f);
	pProj->m_VelY = round_to_int(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
	pProj->m_Water = m_Water;
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
