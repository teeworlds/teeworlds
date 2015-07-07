/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_WEAPONS_GUN_H
#define GAME_SERVER_WEAPONS_GUN_H

#include <game/server/weapon.h>

class CWeaponGun : public CWeapon
{
protected:
	/* CWeapon functions */
	virtual void OnFire(vec2 Direction, vec2 ProjStartPos);

public:
	/* Constructor */
	CWeaponGun(class CCharacter *pOwner, int Ammo);
};

#endif
