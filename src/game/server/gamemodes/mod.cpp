/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>

#include "mod.h"

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "DDRace";
	m_NextBroadcastTick = 0;
	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void CGameControllerMOD::Tick()
{
	// this is the main part of the gamemode, this function is run every tick
	DoPlayerScoreWincheck(); // checks for winners, no teams version
	//DoTeamScoreWincheck(); // checks for winners, two teams version

	IGameController::Tick();

	if (*g_Config.m_SvBroadcast && m_NextBroadcastTick <= Server()->Tick())
	{
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (GameServer()->IsClientReady(i))
				GameServer()->SendBroadcast(g_Config.m_SvBroadcast, i);
		}
		m_NextBroadcastTick = Server()->Tick() + (Server()->TickSpeed()<<1);
	}
}
