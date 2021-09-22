/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos, PickupPhysSize)
{
	m_Type = Type;
	m_Vel = vec2(0, 0);
	Reset();

	m_AmountOfHarpoons = 0;
	m_MarkForHarpoonDeallocation = false;
	m_OriginalPos = m_Pos;
	m_DisturbedTick = 0;
	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;

	m_MarkForHarpoonDeallocation = false;
}

void CPickup::Tick()
{
	if (m_DisturbedTick > 0)
	{
		m_DisturbedTick--;
		if (m_DisturbedTick == 0)
		{
			m_Pos = m_OriginalPos;
			m_MarkForHarpoonDeallocation = false;
		}
		else 
		{
			if (m_Type >= PICKUP_GRENADE && m_Type <= PICKUP_HARPOON)
			{
				m_Vel.y += GameWorld()->m_Core.m_Tuning.m_Gravity;
				GameServer()->Collision()->MoveWaterBox(&m_Pos, &m_Vel, vec2(16.0f, 16.0f), 0.25f, 0, 0.8f);
			}
			else
			{
				m_Vel.y += GameServer()->Collision()->TestBox(m_Pos, vec2(16.0f, 16.0f), 8) ? -GameWorld()->m_Core.m_Tuning.m_Gravity : GameWorld()->m_Core.m_Tuning.m_Gravity;
				GameServer()->Collision()->MoveWaterBox(&m_Pos, &m_Vel, vec2(16.0f, 16.0f), 0.75f, 0, 0.9f);
			}
			
		}
	}
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if(Server()->Tick() > m_SpawnTick)
		{
			m_Pos = m_OriginalPos;
			m_Vel = vec2(0, 0);
			m_MarkForHarpoonDeallocation = false;
			// respawn
			m_SpawnTick = -1;

			if(m_Type == PICKUP_GRENADE || m_Type == PICKUP_SHOTGUN || m_Type == PICKUP_LASER)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = (CCharacter *)GameWorld()->ClosestEntity(m_Pos, 20.0f, CGameWorld::ENTTYPE_CHARACTER, 0);
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
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
				}
				break;

			case PICKUP_ARMOR:
				if(pChr->IncreaseArmor(1))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
				}
				break;

			case PICKUP_GRENADE:
				if(pChr->GiveWeapon(WEAPON_GRENADE, g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Maxammo))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_GRENADE);
				}
				break;
			case PICKUP_SHOTGUN:
				if(pChr->GiveWeapon(WEAPON_SHOTGUN, g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_Maxammo))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_SHOTGUN);
				}
				break;
			case PICKUP_LASER:
				if(pChr->GiveWeapon(WEAPON_LASER, g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Maxammo))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_LASER);
				}
				break;

			case PICKUP_NINJA:
				{
					Picked = true;
					// activate ninja on target player
					pChr->GiveNinja();

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
					break;
				}
			case PICKUP_HARPOON:
				{
					if (pChr->GiveHarpoon(1))
					{
						Picked = true;
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
						if (pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_HARPOON);
					}
					break;
				}
			case PICKUP_DIVING:
				{
					if (!pChr->HasDivingGear())
					{
						Picked = true;
						pChr->GiveDiving();
					}
					break;
				}
			default:
				break;
		};

		if(Picked)
		{
			m_MarkForHarpoonDeallocation = true;
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d",
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

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = round_to_int(m_Pos.x);
	pP->m_Y = round_to_int(m_Pos.y);
	pP->m_Type = m_Type;
	pP->m_DisturbedTick = m_DisturbedTick;
}

void CPickup::DeallocateHarpoon()
{
	m_DisturbedTick = GameServer()->Server()->TickSpeed() * GameServer()->Tuning()->m_PickUpDisturbedTime;
	m_Vel = vec2(0, 0);

	/*int i = 0;
	while (true)
	{
		if(m_apHarpoons)
	}*/
}

void CPickup::AllocateHarpoon(CHarpoon* pHarpoon)
{
	m_DisturbedTick = 0;
	
	/*m_apHarpoons[m_AmountOfHarpoons] = pHarpoon;
	m_AmountOfHarpoons++;*/
}

bool CPickup::IsValidForHarpoon(CHarpoon* pHarpoon)
{
	if (m_SpawnTick > 0)
		return false;
	if (pHarpoon->GetOwner() && distance(m_Pos, pHarpoon->GetOwner()->GetPos()) < 16.0f * GameServer()->Tuning()->m_HarpoonPickupIgnoreRange)
		return false;
	return true;
}