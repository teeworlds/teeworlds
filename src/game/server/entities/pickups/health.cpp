/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "health.h"

CPickupHealth::CPickupHealth(CGameWorld *pGameWorld, vec2 Pos)
: CPickup(pGameWorld, PICKUP_HEALTH, SOUND_PICKUP_HEALTH, -1, Pos)
{
}

bool CPickupHealth::OnPickup(CCharacter *pChar)
{
	bool Picked = pChar->IncreaseHealth(1);

	return Picked;
}
