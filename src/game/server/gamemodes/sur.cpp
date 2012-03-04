/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include "sur.h"

CGameControllerSUR::CGameControllerSUR(CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pGameType = "SUR";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_SURVIVAL;
}

// game
void CGameControllerSUR::DoWincheckRound()
{
	int Count[2] = {0};
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
			(!GameServer()->m_apPlayers[i]->m_RespawnDisabled || 
			(GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
			++Count[GameServer()->m_apPlayers[i]->GetTeam()];
	}

	if(Count[TEAM_RED]+Count[TEAM_BLUE] == 0 || (g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_GameStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
	{
		++m_aTeamscore[TEAM_BLUE];
		++m_aTeamscore[TEAM_RED];
		EndRound();
	}
	else if(Count[TEAM_RED] == 0)
	{
		++m_aTeamscore[TEAM_BLUE];
		EndRound();
	}
	else if(Count[TEAM_BLUE] == 0)
	{
		++m_aTeamscore[TEAM_RED];
		EndRound();
	}
}

// general
void CGameControllerSUR::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];

	pGameDataObj->m_FlagCarrierRed = 0;
	pGameDataObj->m_FlagCarrierBlue = 0;
	pGameDataObj->m_FlagDropTickRed = 0;
	pGameDataObj->m_FlagDropTickBlue = 0;
}
