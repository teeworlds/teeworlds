/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "armor.h"

CPickupArmor::CPickupArmor(CGameWorld *pGameWorld, vec2 Pos)
: CPickup(pGameWorld, PICKUP_ARMOR, SOUND_PICKUP_ARMOR, -1, Pos)
{
}

bool CPickupArmor::OnPickup(CCharacter *pChar)
{
	bool Picked = pChar->IncreaseArmor(1);

	return Picked;
}
