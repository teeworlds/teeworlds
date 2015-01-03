/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include "grenade.h"

CPickupWeaponGrenade::CPickupWeaponGrenade(CGameWorld *pGameWorld, vec2 Pos)
: CPickupWeapon(pGameWorld, PICKUP_GRENADE, SOUND_PICKUP_GRENADE, WEAPON_GRENADE, Pos)
{
}
