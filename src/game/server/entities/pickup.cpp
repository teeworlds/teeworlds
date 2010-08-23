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
	/*
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
	*/
}

void CPickup::Move()
{
   if (Server()->Tick()%int(Server()->TickSpeed() * 0.15f) == 0)
   {
       int index = GameServer()->Collision()->IsCp(m_Pos.x, m_Pos.y);
       if (index)
       {
           m_Core = GameServer()->Collision()->CpSpeed(index);
       }
       m_Pos += m_Core;
	}
}

void CPickup::Tick()
{
	Move();
	// wait for respawn
	/*if(m_SpawnTick > 0)
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
	}*/
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive())
	{
		bool sound = false;
		// player picked us up, is someone was hooking us, let them go
		int RespawnTime = -1;
		switch (m_Type)
		{
			case POWERUP_HEALTH:
				if(pChr->Freeze()) GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
				break;
				
			case POWERUP_ARMOR:
				for(int i=WEAPON_SHOTGUN;i<NUM_WEAPONS;i++)
				{
					if (pChr->m_aWeapons[i].m_Got)
					{
						pChr->m_aWeapons[i].m_Got = false;
						pChr->m_aWeapons[i].m_Ammo = 0;
						sound = true;
					}
					if(pChr->m_FreezeTime)
					{
						pChr->m_aWeapons[WEAPON_GUN].m_Ammo = 0;
						pChr->m_aWeapons[WEAPON_HAMMER].m_Ammo =0;
					}
					pChr->m_Ninja.m_ActivationDir=vec2(0,0);
					pChr->m_Ninja.m_ActivationTick=0;
					pChr->m_Ninja.m_CurrentMoveTime=0;
				}
				if (sound)
				{
					pChr->m_LastWeapon = WEAPON_GUN;  
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
				}
				pChr->m_ActiveWeapon = WEAPON_HAMMER;
				break;

			case POWERUP_WEAPON:
				
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS && (!pChr->m_aWeapons[m_Subtype].m_Got || pChr->m_aWeapons[m_Subtype].m_Ammo != -1))
				{
					if(pChr->GiveWeapon(m_Subtype, -1))
					{
						//RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
						
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
					//RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
					
					break;
				}
				
			default:
				break;
		};
/*
		if(RespawnTime >= 0)
		{
			dbg_msg("game", "pickup player='%d:%s' item=%d/%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type, m_Subtype);
			m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}*/
	}
}

void CPickup::Snap(int SnappingClient)
{
	/*
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;
	*/
	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_Id, sizeof(CNetObj_Pickup)));
	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
