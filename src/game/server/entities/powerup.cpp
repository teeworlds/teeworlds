/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "powerup.h"

IPowerup::IPowerup(CGameWorld *pGameworld, int type)
  : CPickup(pGameworld,type,0)
{
}

CArmor::CArmor(CGameWorld *pGameworld)
  : IPowerup(pGameworld,POWERUP_ARMOR)
{
	m_Value = 1;
}

int
CArmor::OnPickup(CCharacter *pChr)
{
	if(pChr->IncreaseArmor(m_Value))
	{
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
		return g_pData->m_aPickups[m_Type].m_Respawntime;
	}
	return -1;
}

CHeart::CHeart(CGameWorld *pGameworld)
  : IPowerup(pGameworld,POWERUP_HEALTH)
{
	m_Value = 1;
}

int
CHeart::OnPickup(CCharacter *pChr)
{
	if(pChr->IncreaseHealth(m_Value))
	{
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
		return g_pData->m_aPickups[m_Type].m_Respawntime;
	}
	return -1;
}
