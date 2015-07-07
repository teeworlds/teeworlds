/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "grenade.h"

CProjectileGrenade::CProjectileGrenade(CGameWorld *pGameWorld, int Owner, vec2 Dir, vec2 Pos)
: CProjectile(pGameWorld, PROJECTILE_GRENADE, Owner, Dir, Pos)
{
}

float CProjectileGrenade::GetCurvature()
{
	return GameServer()->Tuning()->m_GrenadeCurvature;
}

float CProjectileGrenade::GetSpeed()
{
	return GameServer()->Tuning()->m_GrenadeSpeed;
}

float CProjectileGrenade::GetLifeSpan()
{
	return GameServer()->Tuning()->m_GrenadeLifetime;
}

void CProjectileGrenade::OnLifeOver(vec2 Pos)
{
	Explode(Pos);
}

bool CProjectileGrenade::OnCharacterHit(vec2 Pos, class CCharacter *pChar)
{
	Explode(Pos);
	return true;
}

bool CProjectileGrenade::OnGroundHit(vec2 Pos)
{
	Explode(Pos);
	return true;
}

void CProjectileGrenade::Explode(vec2 Pos)
{
	// KABOOM!
	int Damage = g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Damage;
	GameServer()->CreateSound(Pos, SOUND_GRENADE_EXPLODE);
	GameServer()->CreateExplosion(Pos, GetOwner(), WEAPON_GRENADE, Damage);
}
