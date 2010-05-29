// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include "tdm.h"

CGameControllerTDM::CGameControllerTDM(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pGameType = "TDM";
	m_GameFlags = GAMEFLAG_TEAMS;
}

int CGameControllerTDM::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);
	
	
	if(Weapon != WEAPON_GAME)
	{
		// do team scoring
		if(pKiller == pVictim->GetPlayer() || pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam())
			m_aTeamscore[pKiller->GetTeam()&1]--; // klant arschel
		else
			m_aTeamscore[pKiller->GetTeam()&1]++; // good shit
	}
		
	return 0;
}

void CGameControllerTDM::Tick()
{
	DoTeamScoreWincheck();
	IGameController::Tick();
}
