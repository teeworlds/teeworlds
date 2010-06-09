#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType)
: CEntity(pGameWorld, NETOBJTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PickupPhysSize;

	Reset();
	
	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
			m_SpawnTick[i] = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
		else
			m_SpawnTick[i] = -1;
	}
}

void CPickup::Tick()
{
	// wait for respawn
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// reset Pickups after player death
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_ResetPickups)
		{
			// respawn
			m_SpawnTick[i] = -1;
			continue;
		}
		
		if(m_SpawnTick[i] > 0)
		{
			if(Server()->Tick() > m_SpawnTick[i] && g_Config.m_SvPickupRespawn > -1)
			{
				// respawn
				m_SpawnTick[i] = -1;

				if(m_Type == POWERUP_WEAPON)
					GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN, CmaskOne(i));
			}
		}
	}
	
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive() && m_SpawnTick[pChr->GetPlayer()->GetCID()] == -1)
	{
		// player picked us up, is someone was hooking us, let them go
		int RespawnTime = -1;
		switch (m_Type)
		{
			case POWERUP_HEALTH:
				if(pChr->IncreaseHealth(1))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH, CmaskOne(pChr->GetPlayer()->GetCID()));
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
				}
				break;
				
			case POWERUP_ARMOR:
				if(pChr->IncreaseArmor(1))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, CmaskOne(pChr->GetPlayer()->GetCID()));
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
				}
				break;

			case POWERUP_WEAPON:
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS)
				{
					if(pChr->GiveWeapon(m_Subtype, 10))
					{
						RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;

						if(m_Subtype == WEAPON_GRENADE)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE, CmaskOne(pChr->GetPlayer()->GetCID()));
						else if(m_Subtype == WEAPON_SHOTGUN)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, CmaskOne(pChr->GetPlayer()->GetCID()));
						else if(m_Subtype == WEAPON_RIFLE)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, CmaskOne(pChr->GetPlayer()->GetCID()));

						if(pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);
					}
				}
				break;
				
			case POWERUP_NINJA:
				{
					// activate ninja on target player
					pChr->GiveNinja();
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;

					// loop through all players, setting their emotes
					CEntity *apEnts[64];
					int Num = GameServer()->m_World.FindEntities(vec2(0, 0), 1000000, apEnts, 64, NETOBJTYPE_CHARACTER);
					
					for (int i = 0; i < Num; ++i)
					{
						CCharacter *pC = static_cast<CCharacter *>(apEnts[i]);
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
			dbg_msg("game", "pickup player='%d:%s' item=%d/%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type, m_Subtype);
			if(g_Config.m_SvPickupRespawn > -1)
				m_SpawnTick[pChr->GetPlayer()->GetCID()] = Server()->Tick() + Server()->TickSpeed() * g_Config.m_SvPickupRespawn;
			else
				m_SpawnTick[pChr->GetPlayer()->GetCID()] = 1;
		}
	}
}

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick[SnappingClient] != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_Id, sizeof(CNetObj_Pickup)));
	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
