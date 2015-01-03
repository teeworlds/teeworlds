/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUPS_ARMOR_H
#define GAME_SERVER_ENTITIES_PICKUPS_ARMOR_H

#include <game/server/entities/pickup.h>

class CPickupArmor : public CPickup
{
protected:
	/* Pickup functions */
	virtual bool OnPickup(class CCharacter *pChar);

public:
	/* Constructor */
	CPickupArmor(CGameWorld *pGameWorld, vec2 Pos);
};

#endif
