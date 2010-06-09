#include <game/server/gamecontext.h>
#include "character.h"
#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team, vec2 Pos, CCharacter *pOwner)
: CEntity(pGameWorld, NETOBJTYPE_FLAG)
{
	m_Team = Team;
	m_Pos = Pos;
	m_ProximityRadius = m_PhysSize;
	m_pCarryingCCharacter = pOwner;
	
	GameServer()->m_World.InsertEntity(this);
}

void CFlag::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CFlag::Tick()
{
	if(m_pCarryingCCharacter)
		m_Pos = m_pCarryingCCharacter->m_Pos;
}

void CFlag::Snap(int SnappingClient)
{
	if((!m_pCarryingCCharacter && GameServer()->m_apPlayers[SnappingClient]->GetTeam() != m_Team && GameServer()->m_apPlayers[SnappingClient]->GetCharacter() && GameServer()->m_apPlayers[SnappingClient]->GetCharacter()->m_RaceState == CCharacter::RACE_STARTED)
		||(m_pCarryingCCharacter && !GameServer()->m_apPlayers[SnappingClient]->m_ShowOthers && SnappingClient != m_pCarryingCCharacter->GetPlayer()->GetCID()))
		return;
	
	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_Id, sizeof(CNetObj_Flag));
	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = m_Team;
	pFlag->m_CarriedBy = -1;
	
	if(!m_pCarryingCCharacter)
		pFlag->m_CarriedBy = -2;
	else if(m_pCarryingCCharacter && m_pCarryingCCharacter->GetPlayer())
		pFlag->m_CarriedBy = m_pCarryingCCharacter->GetPlayer()->GetCID();
}
