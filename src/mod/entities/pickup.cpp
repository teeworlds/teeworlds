/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "pickup.h"

#include <mod/weapons/shotgun.h>
#include <mod/weapons/grenade.h>
#include <mod/weapons/laser.h>
#include <mod/weapons/ninja.h>
#include <modapi/server/event.h>
#include <tw06/protocol.h>

CPickup::CPickup(CGameWorld *pGameWorld, int Type, vec2 Pos)
: CModAPI_Entity(pGameWorld, MOD_ENTTYPE_PICKUP, Pos, 1, PickupPhysSize)
{
	m_Type = Type;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CPickup::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;

			if(m_Type == PICKUP_GRENADE || m_Type == PICKUP_SHOTGUN || m_Type == PICKUP_LASER)
			{
				CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
					.Send(m_Pos, SOUND_WEAPON_SPAWN);
			}
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive())
	{
		// player picked us up, is someone was hooking us, let them go
		bool Picked = false;
		switch (m_Type)
		{
			case PICKUP_HEALTH:
				if(pChr->IncreaseHealth(1))
				{
					Picked = true;
					CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
						.Send(m_Pos, SOUND_PICKUP_HEALTH);
				}
				break;

			case PICKUP_ARMOR:
				if(pChr->IncreaseArmor(1))
				{
					Picked = true;
					CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
						.Send(m_Pos, SOUND_PICKUP_ARMOR);
				}
				break;

			case PICKUP_GRENADE:
				if(pChr->HasWeapon(WEAPON_GRENADE))
				{
					Picked = pChr->GiveAmmo(MOD_WEAPON_GRENADE, g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Maxammo);
				}
				else
				{
					pChr->GiveWeapon(new CMod_Weapon_Grenade(pChr, g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Maxammo));
					Picked = true;
				}
				
				if(Picked)
				{
					CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
						.Send(m_Pos, SOUND_PICKUP_GRENADE);
						
					if(pChr->GetPlayer())
						CModAPI_Event_WeaponPickup(GameServer()).Client(pChr->GetPlayer()->GetCID()).Send(WEAPON_GRENADE);
				}
				break;
			case PICKUP_SHOTGUN:
				if(pChr->HasWeapon(WEAPON_SHOTGUN))
				{
					Picked = pChr->GiveAmmo(MOD_WEAPON_SHOTGUN, g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_Maxammo);
				}
				else
				{
					pChr->GiveWeapon(new CMod_Weapon_Shotgun(pChr, g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_Maxammo));
					Picked = true;
				}
				
				if(Picked)
				{
					CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
						.Send(m_Pos, SOUND_PICKUP_SHOTGUN);
						
					if(pChr->GetPlayer())
						CModAPI_Event_WeaponPickup(GameServer()).Client(pChr->GetPlayer()->GetCID()).Send(WEAPON_SHOTGUN);
				}
				break;
			case PICKUP_LASER:
				if(pChr->HasWeapon(WEAPON_LASER))
				{
					Picked = pChr->GiveAmmo(MOD_WEAPON_SHOTGUN, g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Maxammo);
				}
				else
				{
					pChr->GiveWeapon(new CMod_Weapon_Laser(pChr, g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Maxammo));
					Picked = true;
				}
				
				if(Picked)
				{
					CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
						.Send(m_Pos, SOUND_PICKUP_SHOTGUN);
												
					if(pChr->GetPlayer())
						CModAPI_Event_WeaponPickup(GameServer()).Client(pChr->GetPlayer()->GetCID()).Send(WEAPON_LASER);
				}
				break;

			case PICKUP_NINJA:
				{
					pChr->GiveWeapon(new CMod_Weapon_Ninja(pChr));
					pChr->SetWeapon(MOD_WEAPON_NINJA);
					Picked = true;

					CModAPI_Event_Sound(GameServer()).World(GameWorld()->m_WorldID)
						.Send(m_Pos, SOUND_PICKUP_NINJA);

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameWorld()->FindFirst(MOD_ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
				}
				break;
		};

		if(Picked)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d/%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
			int RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
			if(RespawnTime >= 0)
				m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}
	}
}

void CPickup::TickPaused()
{
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CPickup::Snap06(int Snapshot, int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CTW06_NetObj_Pickup *pP = static_cast<CTW06_NetObj_Pickup *>(Server()->SnapNewItem(Snapshot, TW06_NETOBJTYPE_PICKUP, GetID(Snapshot, 0), sizeof(CTW06_NetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	
	switch(m_Type)
	{
		case PICKUP_HEALTH:
			pP->m_Type = TW06_POWERUP_HEALTH;
			pP->m_Subtype = 0;
			break;
		case PICKUP_ARMOR:
			pP->m_Type = TW06_POWERUP_ARMOR;
			pP->m_Subtype = 0;
			break;
		case PICKUP_GRENADE:
			pP->m_Type = TW06_POWERUP_WEAPON;
			pP->m_Subtype = TW06_WEAPON_GRENADE;
			break;
		case PICKUP_SHOTGUN:
			pP->m_Type = TW06_POWERUP_WEAPON;
			pP->m_Subtype = TW06_WEAPON_SHOTGUN;
			break;
		case PICKUP_LASER:
			pP->m_Type = TW06_POWERUP_WEAPON;
			pP->m_Subtype = TW06_WEAPON_RIFLE;
			break;
		case PICKUP_NINJA:
			pP->m_Type = TW06_POWERUP_NINJA;
			pP->m_Subtype = 0;
			break;
	}
}

void CPickup::Snap07(int Snapshot, int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(Snapshot, NETOBJTYPE_PICKUP, GetID(Snapshot, 0), sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
}
