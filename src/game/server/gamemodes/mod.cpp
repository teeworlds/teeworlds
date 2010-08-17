// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include "mod.h"

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "MOD";
	
	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void CGameControllerMOD::Tick()
{
	// this is the main part of the gamemode, this function is run every tick
	DoPlayerScoreWincheck(); // checks for winners, no teams version
	//DoTeamScoreWincheck(); // checks for winners, two teams version
	
	IGameController::Tick();
}
