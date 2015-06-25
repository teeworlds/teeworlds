/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/entities/character.h>
#include <game/server/entities/projectiles/grenade.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "grenade.h"

CWeaponGrenade::CWeaponGrenade(CCharacter *pOwner, int Ammo)
: CWeapon(pOwner, WEAPON_GRENADE, false, true, Ammo)
{
}

void CWeaponGrenade::OnFire(vec2 Direction, vec2 ProjStartPos)
{
	CProjectile *pProj = new CProjectileGrenade(&GameServer()->m_World,
		GetOwner()->GetPlayer()->GetCID(), Direction, ProjStartPos);

	SendProjectile(pProj);

	GameServer()->CreateSound(GetOwner()->GetPos(), SOUND_GRENADE_FIRE);
}
