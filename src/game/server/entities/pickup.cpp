/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PickupPhysSize;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	int SpawnDelay = 0;
	switch (m_Type) {
		case (POWERUP_HEALTH): SpawnDelay = g_Config.m_SvSpawnDelayHealth; break;
		case (POWERUP_ARMOR ): SpawnDelay = g_Config.m_SvSpawnDelayArmor ; break;
		case (POWERUP_NINJA ): SpawnDelay = g_Config.m_SvSpawnDelayNinja ; break;
	}
	switch (m_Subtype) {
		case (WEAPON_SHOTGUN): SpawnDelay = g_Config.m_SvSpawnDelayShotgun; break;
		case (WEAPON_GRENADE): SpawnDelay = g_Config.m_SvSpawnDelayGrenade; break;
		case (WEAPON_RIFLE  ): SpawnDelay = g_Config.m_SvSpawnDelayRifle  ; break;
	}
	
	if (SpawnDelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * SpawnDelay;
	else
		m_SpawnTick = -1;
}

void CPickup::Tick()
{
	if (!(g_Config.m_SvSpawnArmor   && m_Type == POWERUP_ARMOR ) &&
	    !(g_Config.m_SvSpawnHealth  && m_Type == POWERUP_HEALTH) &&
	    !(g_Config.m_SvPowerups     && m_Type == POWERUP_NINJA ) &&
	    !(g_Config.m_SvSpawnShotgun && m_Type == POWERUP_WEAPON && m_Subtype == WEAPON_SHOTGUN) &&
	    !(g_Config.m_SvSpawnGrenade && m_Type == POWERUP_WEAPON && m_Subtype == WEAPON_GRENADE) &&
	    !(g_Config.m_SvSpawnRifle   && m_Type == POWERUP_WEAPON && m_Subtype == WEAPON_RIFLE  ))
	{
		m_SpawnTick = 1;
		return;
	}
	
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;

			if(m_Type == POWERUP_WEAPON)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive())
	{
		// player picked us up, is someone was hooking us, let them go
		int RespawnTime = -1;
		switch (m_Type)
		{
			case POWERUP_HEALTH:
				if(pChr->IncreaseHealth(g_Config.m_SvPickupGiveHealth))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
					RespawnTime = g_Config.m_SvRespawnDelayHealth;
				}
				break;

			case POWERUP_ARMOR:
				if(pChr->IncreaseArmor(g_Config.m_SvPickupGiveArmor))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
					RespawnTime = g_Config.m_SvRespawnDelayArmor;
				}
				break;

			case POWERUP_WEAPON:
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS)
				{
					int Ammo = 10;
					switch (m_Subtype) {
						case (WEAPON_SHOTGUN): Ammo = g_Config.m_SvPickupGiveAmmoShotgun; break;
						case (WEAPON_GRENADE): Ammo = g_Config.m_SvPickupGiveAmmoGrenade; break;
						case (WEAPON_RIFLE  ): Ammo = g_Config.m_SvPickupGiveAmmoRifle  ; break;
					}
						
					if(pChr->GiveWeapon(m_Subtype, Ammo))
					{
						switch (m_Subtype) {
							case (WEAPON_SHOTGUN): RespawnTime = g_Config.m_SvRespawnDelayShotgun; break;
							case (WEAPON_GRENADE): RespawnTime = g_Config.m_SvRespawnDelayGrenade; break;
							case (WEAPON_RIFLE  ): RespawnTime = g_Config.m_SvRespawnDelayRifle  ; break;
						}

						if(m_Subtype == WEAPON_GRENADE)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
						else if(m_Subtype == WEAPON_SHOTGUN)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
						else if(m_Subtype == WEAPON_RIFLE)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);

						if(pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);
					}
				}
				break;

			case POWERUP_NINJA:
				{
					// activate ninja on target player
					pChr->GiveNinja();
					RespawnTime = g_Config.m_SvRespawnDelayNinja; 

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
					break;
				}

			default:
				break;
		};

		if(RespawnTime >= 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d/%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type, m_Subtype);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
			m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}
	}
}

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
