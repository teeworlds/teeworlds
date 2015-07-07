/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "ninja.h"

CPickupNinja::CPickupNinja(CGameWorld *pGameWorld, vec2 Pos)
: CPickup(pGameWorld, PICKUP_NINJA, SOUND_PICKUP_NINJA, -1, Pos)
{
}

bool CPickupNinja::OnPickup(CCharacter *pChar)
{
	// activate ninja on target player
	pChar->GiveWeapon(WEAPON_NINJA, -1);

	// loop through all players, setting their emotes
	CCharacter *c = GameWorld()->GetFirst<CCharacter>();
	for(; c; c = (CCharacter *)c->GetNextEntity())
	{
		if(c != pChar)
			c->SetEmote(EMOTE_SURPRISE, Server()->TickSpeed());
	}

	// set emote on target player
	pChar->SetEmote(EMOTE_ANGRY, 1200 * Server()->TickSpeed() / 1000);

	return true;
}
