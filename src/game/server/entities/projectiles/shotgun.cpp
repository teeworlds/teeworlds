/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "shotgun.h"

CProjectileShotgun::CProjectileShotgun(CGameWorld *pGameWorld, int Owner, vec2 Dir, vec2 Pos)
: CProjectile(pGameWorld, PROJECTILE_SHOTGUN, Owner, Dir, Pos)
{
}

float CProjectileShotgun::GetCurvature()
{
	return GameServer()->Tuning()->m_ShotgunCurvature;
}

float CProjectileShotgun::GetSpeed()
{
	return GameServer()->Tuning()->m_ShotgunSpeed;
}

float CProjectileShotgun::GetLifeSpan()
{
	return GameServer()->Tuning()->m_ShotgunLifetime;
}

void CProjectileShotgun::OnLifeOver(vec2 Pos)
{
}

bool CProjectileShotgun::OnCharacterHit(vec2 Pos, class CCharacter *pChar)
{
	int Damage = g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_Damage;
	pChar->TakeDamage(vec2(0, 0), Damage, GetOwner(), WEAPON_SHOTGUN);
	return true;
}

bool CProjectileShotgun::OnGroundHit(vec2 Pos)
{
	return true;
}
