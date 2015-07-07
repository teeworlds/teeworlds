/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/entities/character.h>
#include <game/server/entities/projectiles/gun.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "gun.h"

CWeaponGun::CWeaponGun(CCharacter *pOwner, int Ammo)
: CWeapon(pOwner, WEAPON_GUN, false, false, Ammo)
{
}

void CWeaponGun::OnFire(vec2 Direction, vec2 ProjStartPos)
{
	CProjectile *pProj = new CProjectileGun(&GameServer()->m_World,
		GetOwner()->GetPlayer()->GetCID(), Direction, ProjStartPos);

	SendProjectile(pProj);

	GameServer()->CreateSound(GetOwner()->GetPos(), SOUND_GUN_FIRE);
}
