/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "gun.h"

CProjectileGun::CProjectileGun(CGameWorld *pGameWorld, int Owner, vec2 Dir, vec2 Pos)
: CProjectile(pGameWorld, PROJECTILE_GUN, Owner, Dir, Pos)
{
}

float CProjectileGun::GetCurvature()
{
	return GameServer()->Tuning()->m_GunCurvature;
}

float CProjectileGun::GetSpeed()
{
	return GameServer()->Tuning()->m_GunSpeed;
}

float CProjectileGun::GetLifeSpan()
{
	return GameServer()->Tuning()->m_GunLifetime;
}

void CProjectileGun::OnLifeOver(vec2 Pos)
{
}

bool CProjectileGun::OnCharacterHit(vec2 Pos, class CCharacter *pChar)
{
	int Damage = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Damage;
	pChar->TakeDamage(vec2(0, 0), Damage, GetOwner(), WEAPON_GUN);
	return true;
}

bool CProjectileGun::OnGroundHit(vec2 Pos)
{
	return true;
}
