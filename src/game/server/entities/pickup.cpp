/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType, int Layer, int Number)
: CEntity(pGameWorld, NETOBJTYPE_PICKUP)
{
	m_Layer = Layer;
	m_Number = Number;
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
		int Flags;
		int index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y, &Flags);
		if (index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(index, Flags);
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
	CCharacter *apEnts[MAX_CLIENTS];
	int Num = GameWorld()->FindEntities(m_Pos, 20.0f, (CEntity**)apEnts, 64, NETOBJTYPE_CHARACTER);
	for(int i = 0; i < Num; ++i) {
		CCharacter * pChr = apEnts[i];
		if(pChr && pChr->IsAlive())
		{
			if(m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[pChr->Team()]) continue;
			bool sound = false;
			// player picked us up, is someone was hooking us, let them go
			switch (m_Type)
			{
				case POWERUP_HEALTH:
					if(pChr->Freeze()) GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
					break;
				
				case POWERUP_ARMOR:
					if(pChr->Team() == TEAM_SUPER) continue;
					for(int i=WEAPON_SHOTGUN;i<NUM_WEAPONS;i++)
					{
						if (pChr->m_aWeapons[i].m_Got)
						{
							if(!(pChr->m_FreezeTime && i == WEAPON_NINJA))
							{
								pChr->m_aWeapons[i].m_Got = false;
								pChr->m_aWeapons[i].m_Ammo = 0;
								sound = true;
							}
						}
					}
					pChr->m_Ninja.m_ActivationDir=vec2(0,0);
					pChr->m_Ninja.m_ActivationTick=-500;
					pChr->m_Ninja.m_CurrentMoveTime=0;
					if (sound)
					{
						pChr->m_LastWeapon = WEAPON_GUN;  
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
					}
					if(!pChr->m_FreezeTime) pChr->m_ActiveWeapon = WEAPON_HAMMER;
					break;

				case POWERUP_WEAPON:
				
					if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS && (!pChr->m_aWeapons[m_Subtype].m_Got || (pChr->m_aWeapons[m_Subtype].m_Ammo != -1 && !pChr->m_FreezeTime)))
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
						//if(!pChr->m_FreezeTime) pChr->GiveNinja();
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
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d/%d",
					pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type, m_Subtype);
				GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
			}*/
		}
	}
	
}

void CPickup::Snap(int SnappingClient)
{
	/*
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;
	*/
	CCharacter * SnapChar = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%11;
	if (SnapChar && SnapChar->m_Alive && (m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[SnapChar->Team()]) && (!Tick)) return;
	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_Id, sizeof(CNetObj_Pickup)));
	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
