/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <modapi/mapitem.h>

#include "gamecontext.h"
#include "gamecontroller.h"
#include "player.h"

#include <modapi/server/weapon.h>

#include <mod/entities/character.h>
#include <mod/entities/pickup.h>
#include <mod/weapons/hammer.h>
#include <mod/weapons/gun.h>
#include <mod/weapons/shotgun.h>
#include <mod/weapons/grenade.h>
#include <mod/weapons/laser.h>
#include <mod/weapons/ninja.h>

#include <tw06/protocol.h>

IGameController::IGameController(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();

	// balancing
	m_aTeamSize[TEAM_RED] = 0;
	m_aTeamSize[TEAM_BLUE] = 0;
	m_UnbalancedTick = TBALANCE_OK;

	// game
	m_GameState = IGS_GAME_RUNNING;
	m_GameStateTimer = TIMER_INFINITE;
	m_GameStartTick = Server()->Tick();
	m_MatchCount = 0;
	m_RoundCount = 0;
	m_SuddenDeath = 0;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;
	if(g_Config.m_SvWarmup)
		SetGameState(IGS_WARMUP_USER, g_Config.m_SvWarmup);
	else
		SetGameState(IGS_WARMUP_GAME, TIMER_INFINITE);

	// info
	m_GameFlags = 0;
	m_pGameType = "unknown";
	m_GameInfo.m_MatchCurrent = m_MatchCount+1;
	m_GameInfo.m_MatchNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvMatchesPerMap) ? g_Config.m_SvMatchesPerMap : 0;
	m_GameInfo.m_ScoreLimit = g_Config.m_SvScorelimit;
	m_GameInfo.m_TimeLimit = g_Config.m_SvTimelimit;

	// map
	m_aMapWish[0] = 0;
}

//activity
void IGameController::DoActivityCheck()
{
	if(g_Config.m_SvInactiveKickTime == 0)
		return;

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->IsDummy() && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
			!Server()->IsAuthed(i) && (GameServer()->m_apPlayers[i]->m_InactivityTickCounter > g_Config.m_SvInactiveKickTime*Server()->TickSpeed()*60))
		{
			switch(g_Config.m_SvInactiveKick)
			{
			case 0:
				{
					// move player to spectator
					DoTeamChange(GameServer()->m_apPlayers[i], TEAM_SPECTATORS);
				}
				break;
			case 1:
				{
					// move player to spectator if the reserved slots aren't filled yet, kick him otherwise
					int Spectators = 0;
					for(int j = 0; j < MAX_CLIENTS; ++j)
						if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS)
							++Spectators;
					if(Spectators >= g_Config.m_SvSpectatorSlots)
						Server()->Kick(i, "Kicked for inactivity");
					else
						DoTeamChange(GameServer()->m_apPlayers[i], TEAM_SPECTATORS);
				}
				break;
			case 2:
				{
					// kick the player
					Server()->Kick(i, "Kicked for inactivity");
				}
			}
		}
	}
}

bool IGameController::GetPlayersReadyState()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !GameServer()->m_apPlayers[i]->m_IsReadyToPlay)
			return false;
	}

	return true;
}

void IGameController::SetPlayersReadyState(bool ReadyState)
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (ReadyState || !GameServer()->m_apPlayers[i]->m_DeadSpecMode))
			GameServer()->m_apPlayers[i]->m_IsReadyToPlay = ReadyState;
	}
}

// balancing
bool IGameController::CanBeMovedOnBalance(int ClientID) const
{
	return true;
}

void IGameController::CheckTeamBalance()
{
	if(!IsTeamplay() || !g_Config.m_SvTeambalanceTime)
	{
		m_UnbalancedTick = TBALANCE_OK;
		return;
	}

	// check if teams are unbalanced
	char aBuf[256];
	if(absolute(m_aTeamSize[TEAM_RED]-m_aTeamSize[TEAM_BLUE]) >= NUM_TEAMS)
	{
		str_format(aBuf, sizeof(aBuf), "Teams are NOT balanced (red=%d blue=%d)", m_aTeamSize[TEAM_RED], m_aTeamSize[TEAM_BLUE]);
		if(m_UnbalancedTick <= TBALANCE_OK)
			m_UnbalancedTick = Server()->Tick();
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Teams are balanced (red=%d blue=%d)", m_aTeamSize[TEAM_RED], m_aTeamSize[TEAM_BLUE]);
		m_UnbalancedTick = TBALANCE_OK;
	}
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

void IGameController::DoTeamBalance()
{
	if(!IsTeamplay() || !g_Config.m_SvTeambalanceTime || absolute(m_aTeamSize[TEAM_RED]-m_aTeamSize[TEAM_BLUE]) < NUM_TEAMS)
		return;

	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "Balancing teams");

	float aTeamScore[NUM_TEAMS] = {0};
	float aPlayerScore[MAX_CLIENTS] = {0.0f};

	// gather stats
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			aPlayerScore[i] = GameServer()->m_apPlayers[i]->m_Score*Server()->TickSpeed()*60.0f/
				(Server()->Tick()-GameServer()->m_apPlayers[i]->m_ScoreStartTick);
			aTeamScore[GameServer()->m_apPlayers[i]->GetTeam()] += aPlayerScore[i];
		}
	}

	int BiggerTeam = (m_aTeamSize[TEAM_RED] > m_aTeamSize[TEAM_BLUE]) ? TEAM_RED : TEAM_BLUE;
	int NumBalance = absolute(m_aTeamSize[TEAM_RED]-m_aTeamSize[TEAM_BLUE]) / NUM_TEAMS;

	// balance teams
	do
	{
		CPlayer *pPlayer = 0;
		float ScoreDiff = aTeamScore[BiggerTeam];
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i] || !CanBeMovedOnBalance(i))
				continue;

			// remember the player whom would cause lowest score-difference
			if(GameServer()->m_apPlayers[i]->GetTeam() == BiggerTeam &&
				(!pPlayer || absolute((aTeamScore[BiggerTeam^1]+aPlayerScore[i]) - (aTeamScore[BiggerTeam]-aPlayerScore[i])) < ScoreDiff))
			{
				pPlayer = GameServer()->m_apPlayers[i];
				ScoreDiff = absolute((aTeamScore[BiggerTeam^1]+aPlayerScore[i]) - (aTeamScore[BiggerTeam]-aPlayerScore[i]));
			}
		}

		// move the player to the other team
		if(pPlayer)
		{
			int Temp = pPlayer->m_LastActionTick;
			DoTeamChange(pPlayer, BiggerTeam^1);
			pPlayer->m_LastActionTick = Temp;
			pPlayer->Respawn();
			GameServer()->SendGameMsg(GAMEMSG_TEAM_BALANCE_VICTIM, pPlayer->GetTeam(), pPlayer->GetCID());
		}
	}
	while(--NumBalance);

	m_UnbalancedTick = TBALANCE_OK;
	GameServer()->SendGameMsg(GAMEMSG_TEAM_BALANCE, -1);
}

// event
int IGameController::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	// do scoreing
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;
	if(pKiller == pVictim->GetPlayer())
		pVictim->GetPlayer()->m_Score--; // suicide
	else
	{
		if(IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
			pKiller->m_Score--; // teamkill
		else
			pKiller->m_Score++; // normal kill
	}
	if(Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;


	// update spectator modes for dead players in survival
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_DeadSpecMode)
				GameServer()->m_apPlayers[i]->UpdateDeadSpecMode();
	}

	return 0;
}

void IGameController::OnCharacterSpawn(CCharacter *pChr)
{
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		// give start equipment
		pChr->IncreaseHealth(10);
		pChr->IncreaseArmor(5);

		pChr->GiveWeapon(new CMod_Weapon_Hammer(pChr));
		pChr->GiveWeapon(new CMod_Weapon_Gun(pChr, 10));
		pChr->GiveWeapon(new CMod_Weapon_Shotgun(pChr, 10));
		pChr->GiveWeapon(new CMod_Weapon_Grenade(pChr, 10));
		pChr->GiveWeapon(new CMod_Weapon_Laser(pChr, 5));

		// prevent respawn
		pChr->GetPlayer()->m_RespawnDisabled = GetStartRespawnState();
	}
	else
	{
		// default health
		pChr->IncreaseHealth(10);

		// give default weapons
		pChr->GiveWeapon(new CMod_Weapon_Hammer(pChr));
		pChr->GiveWeapon(new CMod_Weapon_Gun(pChr, 10));
	}
}

void IGameController::OnFlagReturn(CFlag *pFlag)
{
}

bool IGameController::OnEntityPoint(int EditorResource, int EntityType, vec2 Pos)
{
	// don't add pickups in survival
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		if(EntityType < MODAPI_ENTITYPOINTTYPE_SPAWN || EntityType > MODAPI_ENTITYPOINTTYPE_SPAWN_BLUE)
			return false;
	}

	int PickupType = -1;

	switch(EntityType)
	{
		case MODAPI_ENTITYPOINTTYPE_SPAWN:
			m_aaSpawnPoints[0].add(Pos);
			break;
		case MODAPI_ENTITYPOINTTYPE_SPAWN_RED:
			m_aaSpawnPoints[1].add(Pos);
			break;
		case MODAPI_ENTITYPOINTTYPE_SPAWN_BLUE:
			m_aaSpawnPoints[2].add(Pos);
			break;
		case MODAPI_ENTITYPOINTTYPE_ARMOR:
			PickupType = PICKUP_ARMOR;
			break;
		case MODAPI_ENTITYPOINTTYPE_HEALTH:
			PickupType = PICKUP_HEALTH;
			break;
		case MODAPI_ENTITYPOINTTYPE_WEAPON_SHOTGUN:
			PickupType = PICKUP_SHOTGUN;
			break;
		case MODAPI_ENTITYPOINTTYPE_WEAPON_GRENADE:
			PickupType = PICKUP_GRENADE;
			break;
		case MODAPI_ENTITYPOINTTYPE_WEAPON_LASER:
			PickupType = PICKUP_LASER;
			break;
		case MODAPI_ENTITYPOINTTYPE_WEAPON_NINJA:
			if(g_Config.m_SvPowerups)
				PickupType = PICKUP_NINJA;
	}

	if(PickupType != -1)
	{
		for(int i=0; i<MOD_NUM_WORLDS; i++)
		{
			new CPickup(&GameServer()->m_World[i], PickupType, Pos);
		}
		return true;
	}

	return false;
}

void IGameController::OnPlayerConnect(CPlayer *pPlayer)
{
	int ClientID = pPlayer->GetCID();
	pPlayer->Respawn();

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), pPlayer->GetTeam());
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// update game info
	UpdateGameInfo(ClientID);
}

void IGameController::OnPlayerDisconnect(CPlayer *pPlayer)
{
	pPlayer->OnDisconnect();

	int ClientID = pPlayer->GetCID();
	if(Server()->ClientIngame(ClientID))
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientID, Server()->ClientName(ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		--m_aTeamSize[pPlayer->GetTeam()];
		m_UnbalancedTick = TBALANCE_CHECK;
	}
}

void IGameController::OnPlayerInfoChange(CPlayer *pPlayer)
{
}

void IGameController::OnPlayerReadyChange(CPlayer *pPlayer)
{
	if(g_Config.m_SvPlayerReadyMode && pPlayer->GetTeam() != TEAM_SPECTATORS && !pPlayer->m_DeadSpecMode)
	{
		// change players ready state
		pPlayer->m_IsReadyToPlay ^= 1;

		// check if it effects current game state
		switch(m_GameState)
		{
		case IGS_GAME_RUNNING:
			// one player isn't ready -> pause the game
			if(!pPlayer->m_IsReadyToPlay)
				SetGameState(IGS_GAME_PAUSED, TIMER_INFINITE);
			break;
		case IGS_WARMUP_USER:
			// all players are ready -> end warmup
			if(GetPlayersReadyState())
				SetGameState(IGS_WARMUP_USER, 0);
			break;
		case IGS_GAME_PAUSED:
			// all players are ready -> unpause the game
			if(GetPlayersReadyState())
				SetGameState(IGS_GAME_PAUSED, 0);
			break;
		case IGS_WARMUP_GAME:
		case IGS_START_COUNTDOWN:
		case IGS_END_MATCH:
		case IGS_END_ROUND:
			// not effected
			break;
		}
	}
}

void IGameController::OnReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
			if(m_RoundCount == 0)
			{
				GameServer()->m_apPlayers[i]->m_Score = 0;
				GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			}
			GameServer()->m_apPlayers[i]->m_IsReadyToPlay = true;
		}
	}
}

// game
void IGameController::DoWincheckMatch()
{
	if(IsTeamplay())
	{
		// check score win condition
		if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE] || m_GameFlags&GAMEFLAG_SURVIVAL)
				EndMatch();
			else
				m_SuddenDeath = 1;
		}
	}
	else
	{
		// gather some stats
		int Topscore = 0;
		int TopscoreCount = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				if(GameServer()->m_apPlayers[i]->m_Score > Topscore)
				{
					Topscore = GameServer()->m_apPlayers[i]->m_Score;
					TopscoreCount = 1;
				}
				else if(GameServer()->m_apPlayers[i]->m_Score == Topscore)
					TopscoreCount++;
			}
		}

		// check score win condition
		if((m_GameInfo.m_ScoreLimit > 0 && Topscore >= m_GameInfo.m_ScoreLimit) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
		{
			if(TopscoreCount == 1)
				EndMatch();
			else
				m_SuddenDeath = 1;
		}
	}
}

void IGameController::ResetGame()
{
	// reset the game
	for(int i=0; i<MOD_NUM_WORLDS; i++)
	{
		GameServer()->m_World[i].m_ResetRequested = true;
	}
	
	SetGameState(IGS_GAME_RUNNING);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;

	int MatchNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvMatchesPerMap) ? g_Config.m_SvMatchesPerMap : 0;
	if(MatchNum == 0)
		m_MatchCount = 0;
	bool GameInfoChanged = (m_GameInfo.m_MatchCurrent != m_MatchCount+1) || (m_GameInfo.m_MatchNum != MatchNum) ||
							(m_GameInfo.m_ScoreLimit != g_Config.m_SvScorelimit) || (m_GameInfo.m_TimeLimit != g_Config.m_SvTimelimit);
	m_GameInfo.m_MatchCurrent = m_MatchCount+1;
	m_GameInfo.m_MatchNum = MatchNum;
	m_GameInfo.m_ScoreLimit = g_Config.m_SvScorelimit;
	m_GameInfo.m_TimeLimit = g_Config.m_SvTimelimit;
	if(GameInfoChanged)
		UpdateGameInfo(-1);

	// do team-balancing
	DoTeamBalance();
}

void IGameController::SetGameState(EGameState GameState, int Timer)
{
	// change game state
	switch(GameState)
	{
	case IGS_WARMUP_GAME:
		// game based warmup is only possible when game or any warmup is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER)
		{
			if(Timer == TIMER_INFINITE)
			{
				// run warmup till there're enough players
				m_GameState = GameState;
 				m_GameStateTimer = TIMER_INFINITE;
		
				// enable respawning in survival when activating warmup
				if(m_GameFlags&GAMEFLAG_SURVIVAL)
				{
					for(int i = 0; i < MAX_CLIENTS; ++i)
						if(GameServer()->m_apPlayers[i])
							GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
				}
			}
			else if(Timer == 0)
			{
				// start new match
				StartMatch();
			}
		}
		break;
	case IGS_WARMUP_USER:
		// user based warmup is only possible when the game or a user based warmup is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_WARMUP_USER)
		{
			if(Timer != 0)
			{
				// start warmup
				if(Timer < 0 && g_Config.m_SvPlayerReadyMode)
				{
					// run warmup till all players are ready
					m_GameState = GameState;
 					m_GameStateTimer = TIMER_INFINITE;
 					SetPlayersReadyState(false);
				}
				else if(Timer > 0)
				{
					// run warmup for a specific time intervall
					m_GameState = GameState;
					m_GameStateTimer = Timer*Server()->TickSpeed();
				}
		
				// enable respawning in survival when activating warmup
				if(m_GameFlags&GAMEFLAG_SURVIVAL)
				{
					for(int i = 0; i < MAX_CLIENTS; ++i)
						if(GameServer()->m_apPlayers[i])
							GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
				}
			}
			else
			{
				// start new match
				StartMatch();
			}
		}
		break;
	case IGS_START_COUNTDOWN:
		// only possible when game, pause or start countdown is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED || m_GameState == IGS_START_COUNTDOWN)
		{
			m_GameState = GameState;
			m_GameStateTimer = 3*Server()->TickSpeed();
			for(int i=0; i<MOD_NUM_WORLDS; i++)
			{
				GameServer()->m_World[i].m_Paused = true;
			}
		}
		break;
	case IGS_GAME_RUNNING:
		// always possible
		{
			m_GameState = GameState;
			m_GameStateTimer = TIMER_INFINITE;
			SetPlayersReadyState(true);
			for(int i=0; i<MOD_NUM_WORLDS; i++)
			{
				GameServer()->m_World[i].m_Paused = false;
			}
		}
		break;
	case IGS_GAME_PAUSED:
		// only possible when game is running or paused
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED)
		{
			if(Timer != 0)
			{
				// start pause
				if(Timer < 0)
				{
					// pauses infinitely till all players are ready or disabled via rcon command
					m_GameStateTimer = TIMER_INFINITE;
					SetPlayersReadyState(false);
				}
				else
				{
					// pauses for a specific time intervall
					m_GameStateTimer = Timer*Server()->TickSpeed();
				}

				m_GameState = GameState;
				for(int i=0; i<MOD_NUM_WORLDS; i++)
				{
					GameServer()->m_World[i].m_Paused = true;
				}
			}
			else
			{
				// start a countdown to end pause
				SetGameState(IGS_START_COUNTDOWN);
			}
		}
		break;
	case IGS_END_ROUND:
	case IGS_END_MATCH:
		// only possible when game is running or over
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_END_MATCH || m_GameState == IGS_END_ROUND || m_GameState == IGS_GAME_PAUSED)
		{
			m_GameState = GameState;
			m_GameStateTimer = Timer*Server()->TickSpeed();
			m_SuddenDeath = 0;
			for(int i=0; i<MOD_NUM_WORLDS; i++)
			{
				GameServer()->m_World[i].m_Paused = true;
			}
		}
	}
}

void IGameController::StartMatch()
{
	ResetGame();

	m_RoundCount = 0;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;

	// start countdown if there're enough players, otherwise do warmup till there're
	if(HasEnoughPlayers())
		SetGameState(IGS_START_COUNTDOWN);
	else
		SetGameState(IGS_WARMUP_GAME, TIMER_INFINITE);

	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start match type='%s' teamplay='%d'", m_pGameType, m_GameFlags&GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

void IGameController::StartRound()
{
	ResetGame();

	++m_RoundCount;

	// start countdown if there're enough players, otherwise abort to warmup
	if(HasEnoughPlayers())
		SetGameState(IGS_START_COUNTDOWN);
	else
		SetGameState(IGS_WARMUP_GAME, TIMER_INFINITE);
}

// general
void IGameController::Snap06(int Snapshot, int SnappingClient)
{
	CTW06_NetObj_GameInfo *pGameInfoObj = (CTW06_NetObj_GameInfo *)Server()->SnapNewItem(Snapshot, TW06_NETOBJTYPE_GAMEINFO, 0, sizeof(CTW06_NetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	//Get Client World
	int WorldID = GameServer()->m_apPlayers[SnappingClient]->GetWorldID();

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameState == IGS_END_MATCH)
		pGameInfoObj->m_GameStateFlags |= TW06_GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= TW06_GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World[WorldID].m_Paused)
		pGameInfoObj->m_GameStateFlags |= TW06_GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = m_GameStartTick;
	pGameInfoObj->m_WarmupTimer = 0;

	pGameInfoObj->m_ScoreLimit = g_Config.m_SvScorelimit;
	pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;

	pGameInfoObj->m_RoundNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvMatchesPerMap) ? g_Config.m_SvMatchesPerMap : 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount+1;
}

void IGameController::Snap07(int Snapshot, int SnappingClient)
{
	CNetObj_GameData *pGameData = static_cast<CNetObj_GameData *>(Server()->SnapNewItem(Snapshot, NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData)));
	if(!pGameData)
		return;

	pGameData->m_GameStartTick = m_GameStartTick;
	pGameData->m_GameStateFlags = 0;
	pGameData->m_GameStateEndTick = 0; // no timer/infinite = 0, on end = GameEndTick, otherwise = GameStateEndTick
	switch(m_GameState)
	{
	case IGS_WARMUP_GAME:
	case IGS_WARMUP_USER:
		pGameData->m_GameStateFlags |= GAMESTATEFLAG_WARMUP;
		if(m_GameStateTimer != TIMER_INFINITE)
			pGameData->m_GameStateEndTick = Server()->Tick()+m_GameStateTimer;
		break;
	case IGS_START_COUNTDOWN:
		pGameData->m_GameStateFlags |= GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED;
		if(m_GameStateTimer != TIMER_INFINITE)
			pGameData->m_GameStateEndTick = Server()->Tick()+m_GameStateTimer;
		break;
	case IGS_GAME_PAUSED:
		pGameData->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
		if(m_GameStateTimer != TIMER_INFINITE)
			pGameData->m_GameStateEndTick = Server()->Tick()+m_GameStateTimer;
		break;
	case IGS_END_ROUND:
		pGameData->m_GameStateFlags |= GAMESTATEFLAG_ROUNDOVER;
		pGameData->m_GameStateEndTick = Server()->Tick()-m_GameStartTick-TIMER_END*Server()->TickSpeed()+m_GameStateTimer;
		break;
	case IGS_END_MATCH:
		pGameData->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
		pGameData->m_GameStateEndTick = Server()->Tick()-m_GameStartTick-TIMER_END*Server()->TickSpeed()+m_GameStateTimer;
		break;
	case IGS_GAME_RUNNING:
		// not effected
		break;
	}
	if(m_SuddenDeath)
		pGameData->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;

	if(IsTeamplay())
	{
		CNetObj_GameDataTeam *pGameDataTeam = static_cast<CNetObj_GameDataTeam *>(Server()->SnapNewItem(Snapshot, NETOBJTYPE_GAMEDATATEAM, 0, sizeof(CNetObj_GameDataTeam)));
		if(!pGameDataTeam)
			return;

		pGameDataTeam->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
		pGameDataTeam->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
	}

	// demo recording
	if(SnappingClient == -1)
	{
		CNetObj_De_GameInfo *pGameInfo = static_cast<CNetObj_De_GameInfo *>(Server()->SnapNewItem(Snapshot, NETOBJTYPE_DE_GAMEINFO, 0, sizeof(CNetObj_De_GameInfo)));
		if(!pGameInfo)
			return;

		pGameInfo->m_GameFlags = m_GameFlags;
		pGameInfo->m_ScoreLimit = m_GameInfo.m_ScoreLimit;
		pGameInfo->m_TimeLimit = m_GameInfo.m_TimeLimit;
		pGameInfo->m_MatchNum = m_GameInfo.m_MatchNum;
		pGameInfo->m_MatchCurrent = m_GameInfo.m_MatchCurrent;
	}
}

void IGameController::Tick()
{
	// handle game states
	if(m_GameState != IGS_GAME_RUNNING)
	{
		if(m_GameStateTimer > 0)
			--m_GameStateTimer;

		if(m_GameStateTimer == 0)
		{
			// timer fires
			switch(m_GameState)
			{
			case IGS_WARMUP_USER:
				// end warmup
				SetGameState(IGS_WARMUP_USER, 0);
				break;
			case IGS_START_COUNTDOWN:
				// unpause the game
				SetGameState(IGS_GAME_RUNNING);
				break;
			case IGS_GAME_PAUSED:
				// end pause
				SetGameState(IGS_GAME_PAUSED, 0);
				break;
			case IGS_END_ROUND:
				// check if the match is over otherwise start next round
				DoWincheckMatch();
				if(m_GameState != IGS_END_MATCH)
					StartRound();
				break;
			case IGS_END_MATCH:
				// start next match
				CycleMap();
				m_MatchCount++;
				StartMatch();
				break;
			case IGS_WARMUP_GAME:
			case IGS_GAME_RUNNING:
				// not effected
				break;
			}
		}
		else
		{
			// timer still running
			switch(m_GameState)
 			{
			case IGS_WARMUP_USER:
				// check if player ready mode was disabled and it waits that all players are ready -> end warmup
				if(!g_Config.m_SvPlayerReadyMode && m_GameStateTimer == TIMER_INFINITE)
					SetGameState(IGS_WARMUP_USER, 0);
				break;
			case IGS_START_COUNTDOWN:
			case IGS_GAME_PAUSED:
				// freeze the game
				++m_GameStartTick;
				break;
			case IGS_WARMUP_GAME:
			case IGS_GAME_RUNNING:
			case IGS_END_MATCH:
			case IGS_END_ROUND:
				// not effected
				break;
 			}
		}
	}

	// do team-balancing (skip this in survival, done there when a round starts)
	if(IsTeamplay() && !(m_GameFlags&GAMEFLAG_SURVIVAL))
	{
		switch(m_UnbalancedTick)
		{
		case TBALANCE_CHECK:
			CheckTeamBalance();
			break;
		case TBALANCE_OK:
			break;
		default:
			if(Server()->Tick() > m_UnbalancedTick+g_Config.m_SvTeambalanceTime*Server()->TickSpeed()*60)
				DoTeamBalance();
		}
	}

	// check for inactive players
	DoActivityCheck();

	// win check
	if((m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED) && !GameServer()->m_World[MOD_WORLD_DEFAULT].m_ResetRequested)
	{
		if(m_GameFlags&GAMEFLAG_SURVIVAL)
			DoWincheckRound();
		else
			DoWincheckMatch();
	}
}

// info
bool IGameController::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2)
		return false;

	if(IsTeamplay())
	{
		if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if(!g_Config.m_SvTeamdamage && GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}

bool IGameController::IsPlayerReadyMode() const
{
	return g_Config.m_SvPlayerReadyMode != 0 && (m_GameStateTimer == TIMER_INFINITE && (m_GameState == IGS_WARMUP_USER || m_GameState == IGS_GAME_PAUSED));
}

bool IGameController::IsTeamChangeAllowed() const
{
	return !GameServer()->m_World[MOD_WORLD_DEFAULT].m_Paused || (m_GameState == IGS_START_COUNTDOWN && m_GameStartTick == Server()->Tick());
}

void IGameController::UpdateGameInfo(int ClientID)
{
	int Start = (ClientID < 0 ? 0 : ClientID);
	int End = (ClientID < 0 ? MAX_CLIENTS : ClientID+1);
	
	for(int i = Start; i < End; i++)
	{
		if(!GameServer()->m_apPlayers[i] || !Server()->ClientIngame(i))
			continue;
		
		if(Server()->GetClientProtocol(i) != MODAPI_CLIENTPROTOCOL_TW06)
		{
			CNetMsg_Sv_GameInfo GameInfoMsg;
			GameInfoMsg.m_GameFlags = m_GameFlags;
			GameInfoMsg.m_ScoreLimit = m_GameInfo.m_ScoreLimit;
			GameInfoMsg.m_TimeLimit = m_GameInfo.m_TimeLimit;
			GameInfoMsg.m_MatchNum = m_GameInfo.m_MatchNum;
			GameInfoMsg.m_MatchCurrent = m_GameInfo.m_MatchCurrent;
		}
	}
}

// map
static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void IGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));
	EndMatch();
}

void IGameController::CycleMap()
{
	if(m_aMapWish[0] != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "rotating map to %s", m_aMapWish);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		str_copy(g_Config.m_SvMap, m_aMapWish, sizeof(g_Config.m_SvMap));
		m_aMapWish[0] = 0;
		m_MatchCount = 0;
		return;
	}
	if(!str_length(g_Config.m_SvMaprotation))
		return;

	if(m_MatchCount < m_GameInfo.m_MatchNum-1)
	{
		if(g_Config.m_SvMatchSwap)
			GameServer()->SwapTeams();
		return;
	}

	// handle maprotation
	const char *pMapRotation = g_Config.m_SvMaprotation;
	const char *pCurrentMap = g_Config.m_SvMap;

	int CurrentMapLen = str_length(pCurrentMap);
	const char *pNextMap = pMapRotation;
	while(*pNextMap)
	{
		int WordLen = 0;
		while(pNextMap[WordLen] && !IsSeparator(pNextMap[WordLen]))
			WordLen++;

		if(WordLen == CurrentMapLen && str_comp_num(pNextMap, pCurrentMap, CurrentMapLen) == 0)
		{
			// map found
			pNextMap += CurrentMapLen;
			while(*pNextMap && IsSeparator(*pNextMap))
				pNextMap++;

			break;
		}

		pNextMap++;
	}

	// restart rotation
	if(pNextMap[0] == 0)
		pNextMap = pMapRotation;

	// cut out the next map
	char aBuf[512] = {0};
	for(int i = 0; i < 511; i++)
	{
		aBuf[i] = pNextMap[i];
		if(IsSeparator(pNextMap[i]) || pNextMap[i] == 0)
		{
			aBuf[i] = 0;
			break;
		}
	}

	// skip spaces
	int i = 0;
	while(IsSeparator(aBuf[i]))
		i++;

	m_MatchCount = 0;

	char aBufMsg[256];
	str_format(aBufMsg, sizeof(aBufMsg), "rotating map to %s", &aBuf[i]);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	str_copy(g_Config.m_SvMap, &aBuf[i], sizeof(g_Config.m_SvMap));
}

// spawn
bool IGameController::CanSpawn(int ClientID, vec2 *pOutPos) const
{
	if(!GameServer()->m_apPlayers[ClientID])
		return false;
	
	int Team = GameServer()->m_apPlayers[ClientID]->GetTeam();
	int WorldID = GameServer()->m_apPlayers[ClientID]->GetWorldID();
	
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS || GameServer()->m_World[WorldID].m_Paused || GameServer()->m_World[WorldID].m_ResetRequested)
		return false;

	CSpawnEval Eval;

	if(IsTeamplay())
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(WorldID, &Eval, 1+(Team&1));
		if(!Eval.m_Got)
		{
			EvaluateSpawnType(WorldID, &Eval, 0);
			if(!Eval.m_Got)
				EvaluateSpawnType(WorldID, &Eval, 1+((Team+1)&1));
		}
	}
	else
	{
		EvaluateSpawnType(WorldID, &Eval, 0);
		EvaluateSpawnType(WorldID, &Eval, 1);
		EvaluateSpawnType(WorldID, &Eval, 2);
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

float IGameController::EvaluateSpawnPos(int WorldID, CSpawnEval *pEval, vec2 Pos) const
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World[WorldID].FindFirst(MOD_ENTTYPE_CHARACTER));
	for(; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if(pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->GetPos());
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f/d);
	}

	return Score;
}

void IGameController::EvaluateSpawnType(int WorldID, CSpawnEval *pEval, int Type) const
{
	// get spawn point
	for(int i = 0; i < m_aaSpawnPoints[Type].size(); i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = GameServer()->m_World[WorldID].FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity**)aEnts, MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };	// start, left, up, right, down
		int Result = -1;
		for(int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for(int c = 0; c < Num; ++c)
				if(GameServer()->Collision()->CheckPoint(m_aaSpawnPoints[Type][i]+Positions[Index]) ||
					distance(aEnts[c]->GetPos(), m_aaSpawnPoints[Type][i]+Positions[Index]) <= aEnts[c]->GetProximityRadius())
				{
					Result = -1;
					break;
				}
		}
		if(Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i]+Positions[Result];
		float S = EvaluateSpawnPos(WorldID, pEval, P);
		if(!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool IGameController::GetStartRespawnState() const
{
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		// players can always respawn during warmup or match/round start countdown
		if(m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER || (m_GameState == IGS_START_COUNTDOWN && m_GameStartTick == Server()->Tick()))
			return false;
		else
			return true;
	}
	else
		return false;
}

// team
bool IGameController::CanChangeTeam(CPlayer *pPlayer, int JoinTeam) const
{
	if(!IsTeamplay() || JoinTeam == TEAM_SPECTATORS || !g_Config.m_SvTeambalanceTime)
		return true;

	// simulate what would happen if the player changes team
	int aPlayerCount[NUM_TEAMS] = { m_aTeamSize[TEAM_RED], m_aTeamSize[TEAM_BLUE] };
	aPlayerCount[JoinTeam]++;
	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
		aPlayerCount[JoinTeam^1]--;

	// check if the player-difference decreases or is smaller than 2
	return aPlayerCount[JoinTeam]-aPlayerCount[JoinTeam^1] < NUM_TEAMS;
}

bool IGameController::CanJoinTeam(int Team, int NotThisID) const
{
	if(Team == TEAM_SPECTATORS)
		return true;

	// check if there're enough player slots left
	int TeamMod = GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS ? -1 : 0;
	return TeamMod+m_aTeamSize[TEAM_RED]+m_aTeamSize[TEAM_BLUE] < Server()->MaxClients()-g_Config.m_SvSpectatorSlots;
}

int IGameController::ClampTeam(int Team) const
{
	if(Team < TEAM_RED)
		return TEAM_SPECTATORS;
	if(IsTeamplay())
		return Team&1;
	return TEAM_RED;
}

void IGameController::DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	int OldTeam = pPlayer->GetTeam();
	pPlayer->SetTeam(Team);

	int ClientID = pPlayer->GetCID();

	// notify clients
	CNetMsg_Sv_Team Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Team = Team;
	Msg.m_Silent = DoChatMsg ? 0 : 1;
	Msg.m_CooldownTick = pPlayer->m_TeamChangeTick;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", ClientID, Server()->ClientName(ClientID), Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// update effected game settings
	if(OldTeam != TEAM_SPECTATORS)
	{
		--m_aTeamSize[OldTeam];
		m_UnbalancedTick = TBALANCE_CHECK;
	}
	if(Team != TEAM_SPECTATORS)
	{
		++m_aTeamSize[Team];
		m_UnbalancedTick = TBALANCE_CHECK;
		if(m_GameState == IGS_WARMUP_GAME && HasEnoughPlayers())
			SetGameState(IGS_WARMUP_GAME, 0);
		pPlayer->m_IsReadyToPlay = !IsPlayerReadyMode();
		if(m_GameFlags&GAMEFLAG_SURVIVAL)
			pPlayer->m_RespawnDisabled = GetStartRespawnState();
	}
	OnPlayerInfoChange(pPlayer);
	GameServer()->OnClientTeamChange(ClientID);
}

int IGameController::GetStartTeam()
{
	// this will force the auto balancer to work overtime aswell
	if(g_Config.m_DbgStress)
		return TEAM_RED;

	if(g_Config.m_SvTournamentMode)
		return TEAM_SPECTATORS;

	// determine new team
	int Team = TEAM_RED;
	if(IsTeamplay())
		Team = m_aTeamSize[TEAM_RED] > m_aTeamSize[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;

	// check if there're enough player slots left
	if(m_aTeamSize[TEAM_RED]+m_aTeamSize[TEAM_BLUE] < Server()->MaxClients()-g_Config.m_SvSpectatorSlots)
	{
		++m_aTeamSize[Team];
		m_UnbalancedTick = TBALANCE_CHECK;
		if(m_GameState == IGS_WARMUP_GAME && HasEnoughPlayers())
			SetGameState(IGS_WARMUP_GAME, 0);
		return Team;
	}
	return TEAM_SPECTATORS;
}

void IGameController::RegisterEntityPointLayer(int ID, const char* m_pModName)
{
	m_lEditorResources.set_size(m_lEditorResources.size()+1);
	CRegisteredEditorResource& EditorResource = m_lEditorResources[m_lEditorResources.size()-1];
	
	EditorResource.m_ID = ID;
	str_copy(EditorResource.m_aModName, m_pModName, sizeof(EditorResource.m_aModName));
}
