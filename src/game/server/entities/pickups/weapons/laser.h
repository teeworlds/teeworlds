/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUPS_WEAPONS_LASER_H
#define GAME_SERVER_ENTITIES_PICKUPS_WEAPONS_LASER_H

#include <game/server/entities/pickups/weapon.h>

class CPickupWeaponLaser : public CPickupWeapon
{
public:
	/* Constructor */
	CPickupWeaponLaser(CGameWorld *pGameWorld, vec2 Pos);
};

#endif
