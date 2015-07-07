/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUPS_WEAPON_H
#define GAME_SERVER_ENTITIES_PICKUPS_WEAPON_H

#include <game/server/entities/pickup.h>

class CPickupWeapon : public CPickup
{
private:
	/* Identity */
	int m_Weapon;

protected:
	/* Pickup functions */
	virtual bool OnPickup(class CCharacter *pChar);

public:
	/* Constructor */
	CPickupWeapon(CGameWorld *pGameWorld, int Type, int PickupSound, int Weapon, vec2 Pos);

	/* Getters */
	int GetWeapon() const	{ return m_Weapon; }
};

#endif
