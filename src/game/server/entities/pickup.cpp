/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int PickupSound, int RespawnSound, vec2 Pos)
: CEntity(pGameWorld, ENTTYPE_PICKUP, Pos, ms_PhysSize)
{
	m_Type = Type;
	m_PickupSound = PickupSound;
	m_RespawnSound = RespawnSound;

	Reset();
}

void CPickup::Reset()
{
	if(g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CPickup::Tick()
{
	if(!IsSpawned())
	{
		// wait for respawn
		if(Server()->Tick() >= m_SpawnTick)
		{
			m_SpawnTick = -1;

			if(m_RespawnSound != -1)
				GameServer()->CreateSound(m_Pos, m_RespawnSound);
		}
		else
			return;
	}

	// check if a player intersected us
	CCharacter *pChar = GameWorld()->ClosestEntity<CCharacter>(m_Pos, 20.0f);
	if(pChar && pChar->IsAlive())
	{
		if(OnPickup(pChar))
		{
			int RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
			if(RespawnTime >= 0)
				m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;

			if(m_PickupSound != -1)
				GameServer()->CreateSound(m_Pos, m_PickupSound);

			char aBuf[256];
			int PlayerID = pChar->GetPlayer()->GetCID();
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d", PlayerID,
				Server()->ClientName(PlayerID), m_Type);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		}
	}
}

void CPickup::TickPaused()
{
	if(!IsSpawned())
		m_SpawnTick++;
}

void CPickup::Snap(int SnappingClient)
{
	if(!IsSpawned() || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(!pPickup)
		return;

	pPickup->m_X = (int)m_Pos.x;
	pPickup->m_Y = (int)m_Pos.y;
	pPickup->m_Type = m_Type;
}
