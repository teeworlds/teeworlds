/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUPS_WEAPONS_SHOTGUN_H
#define GAME_SERVER_ENTITIES_PICKUPS_WEAPONS_SHOTGUN_H

#include <game/server/entities/pickups/weapon.h>

class CPickupWeaponShotgun : public CPickupWeapon
{
public:
	/* Constructor */
	CPickupWeaponShotgun(CGameWorld *pGameWorld, vec2 Pos);
};

#endif
