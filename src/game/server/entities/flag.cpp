/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/*
#include <game/server/gamecontext.h>
#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team)
: CEntity(pGameWorld, NETOBJTYPE_FLAG)
{
	m_Number = Team;
	m_ProximityRadius = ms_PhysSize;
	m_pCarryingCharacter = NULL;
	m_GrabTick = 0;
	
	Reset();
}

void CFlag::Reset()
{
	m_pCarryingCharacter = NULL;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0,0);
	m_GrabTick = 0;
}

void CFlag::Snap(int SnappingClient)
{
	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_Number, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Number = m_Number;
	pFlag->m_CarriedBy = -1;
	
	if(m_AtStand)
		pFlag->m_CarriedBy = -2;
	else if(m_pCarryingCharacter && m_pCarryingCharacter->GetPlayer())
		pFlag->m_CarriedBy = m_pCarryingCharacter->GetPlayer()->GetCID();
}
*/
