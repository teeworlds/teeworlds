/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/entities/character.h>
#include <game/server/entities/laser.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "laser.h"

CWeaponLaser::CWeaponLaser(CCharacter *pOwner, int Ammo)
: CWeapon(pOwner, WEAPON_LASER, false, true, Ammo)
{
}

void CWeaponLaser::OnFire(vec2 Direction, vec2 ProjStartPos)
{
	vec2 OwnerPos = GetOwner()->GetPos();
	new CLaser(&GameServer()->m_World, GetOwner()->GetPlayer()->GetCID(),
		OwnerPos, Direction);

	GameServer()->CreateSound(OwnerPos, SOUND_LASER_FIRE);
}
