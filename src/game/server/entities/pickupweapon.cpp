/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickupweapon.h"

CPickupWeapon::CPickupWeapon(CGameWorld *pGameworld, int weaponType)
  : CPickup(pGameworld,POWERUP_WEAPON,weaponType)
{
	m_Ammo = g_pData->m_Weapons.m_aId[weaponType].m_Maxammo;
}

int
CPickupWeapon::OnPickup(CCharacter *pChr)
{
	if(pChr->GiveWeapon(m_Subtype, m_Ammo))
	{
		CreatePickupSound();
		GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);
		return g_pData->m_aPickups[m_Type].m_Respawntime;
	}
	return -1;
}

CPickupNinja::CPickupNinja(CGameWorld *pGameworld)
  : CPickup(pGameworld,POWERUP_NINJA,WEAPON_NINJA)
{
}

CPickupGrenade::CPickupGrenade(CGameWorld *pGameworld) : CPickupWeapon(pGameworld,WEAPON_GRENADE)
{
}

CPickupShotgun::CPickupShotgun(CGameWorld *pGameworld) : CPickupWeapon(pGameworld,WEAPON_SHOTGUN)
{
}

CPickupLaser::CPickupLaser(CGameWorld *pGameworld) : CPickupWeapon(pGameworld,WEAPON_RIFLE)
{
}

void
CPickupGrenade::CreatePickupSound()
{
	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
}

void
CPickupShotgun::CreatePickupSound()
{
	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
}

void
CPickupLaser::CreatePickupSound()
{
	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
}

int
CPickupNinja::OnPickup(CCharacter *pChr)
{
	// activate ninja on target player
	pChr->GiveNinja();

	// loop through all players, setting their emotes
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for(; pC; pC = (CCharacter *)pC->TypeNext())
	{
		if (pC != pChr)
			pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
	}

	pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
	return g_pData->m_aPickups[m_Type].m_Respawntime;
}
