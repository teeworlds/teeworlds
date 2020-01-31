/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/mapitems.h>

#include "entities/character.h"
#include "entities/pickup.h"
#include "gamecontext.h"
#include "gamecontroller.h"
#include "player.h"


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
	if(GameServer()->Config()->Values()->m_SvWarmup)
		SetGameState(IGS_WARMUP_USER, GameServer()->Config()->Values()->m_SvWarmup);
	else
		SetGameState(IGS_WARMUP_GAME, TIMER_INFINITE);

	// info
	m_GameFlags = 0;
	m_pGameType = "unknown";
	m_GameInfo.m_MatchCurrent = m_MatchCount+1;
	m_GameInfo.m_MatchNum = (str_length(GameServer()->Config()->Values()->m_SvMaprotation) && GameServer()->Config()->Values()->m_SvMatchesPerMap) ? GameServer()->Config()->Values()->m_SvMatchesPerMap : 0;
	m_GameInfo.m_ScoreLimit = GameServer()->Config()->Values()->m_SvScorelimit;
	m_GameInfo.m_TimeLimit = GameServer()->Config()->Values()->m_SvTimelimit;

	// map
	m_aMapWish[0] = 0;

	// spawn
	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	// commands
	CommandsManager()->OnInit();
}

//activity
void IGameController::DoActivityCheck()
{
	if(GameServer()->Config()->Values()->m_SvInactiveKickTime == 0)
		return;

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->IsDummy() && (GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS || GameServer()->Config()->Values()->m_SvInactiveKick > 0) &&
			!Server()->IsAuthed(i) && (GameServer()->m_apPlayers[i]->m_InactivityTickCounter > GameServer()->Config()->Values()->m_SvInactiveKickTime*Server()->TickSpeed()*60))
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
			{
				if(GameServer()->Config()->Values()->m_SvInactiveKickSpec)
					Server()->Kick(i, "Kicked for inactivity");
			}
			else
			{
				switch(GameServer()->Config()->Values()->m_SvInactiveKick)
				{
				case 1:
					{
						// move player to spectator
						DoTeamChange(GameServer()->m_apPlayers[i], TEAM_SPECTATORS);
					}
					break;
				case 2:
					{
						// move player to spectator if the reserved slots aren't filled yet, kick him otherwise
						int Spectators = 0;
						for(int j = 0; j < MAX_CLIENTS; ++j)
							if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS)
								++Spectators;
						if(Spectators >= GameServer()->Config()->Values()->m_SvMaxClients - GameServer()->Config()->Values()->m_SvPlayerSlots)
							Server()->Kick(i, "Kicked for inactivity");
						else
							DoTeamChange(GameServer()->m_apPlayers[i], TEAM_SPECTATORS);
					}
					break;
				case 3:
					{
						// kick the player
						Server()->Kick(i, "Kicked for inactivity");
					}
				}
			}
		}
	}
}

bool IGameController::GetPlayersReadyState(int WithoutID)
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(i == WithoutID)
			continue; // skip
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
	if(!IsTeamplay() || !GameServer()->Config()->Values()->m_SvTeambalanceTime)
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
	if(!IsTeamplay() || !GameServer()->Config()->Values()->m_SvTeambalanceTime || absolute(m_aTeamSize[TEAM_RED]-m_aTeamSize[TEAM_BLUE]) < NUM_TEAMS)
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
	// default health
	pChr->IncreaseHealth(10);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
}

void IGameController::OnFlagReturn(CFlag *pFlag)
{
}

bool IGameController::OnEntity(int Index, vec2 Pos)
{
	// don't add pickups in survival
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		if(Index < ENTITY_SPAWN || Index > ENTITY_SPAWN_BLUE)
			return false;
	}

	int Type = -1;

	switch(Index)
	{
	case ENTITY_SPAWN:
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
		break;
	case ENTITY_SPAWN_RED:
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
		break;
	case ENTITY_SPAWN_BLUE:
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
		break;
	case ENTITY_ARMOR_1:
		Type = PICKUP_ARMOR;
		break;
	case ENTITY_HEALTH_1:
		Type = PICKUP_HEALTH;
		break;
	case ENTITY_WEAPON_SHOTGUN:
		Type = PICKUP_SHOTGUN;
		break;
	case ENTITY_WEAPON_GRENADE:
		Type = PICKUP_GRENADE;
		break;
	case ENTITY_WEAPON_LASER:
		Type = PICKUP_LASER;
		break;
	case ENTITY_POWERUP_NINJA:
		if(GameServer()->Config()->Values()->m_SvPowerups)
			Type = PICKUP_NINJA;
	}

	if(Type != -1)
	{
		new CPickup(&GameServer()->m_World, Type, Pos);
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

	CommandsManager()->OnPlayerConnect(Server(), pPlayer);
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

	CheckReadyStates(ClientID);
}

void IGameController::OnPlayerInfoChange(CPlayer *pPlayer)
{
}

void IGameController::OnPlayerReadyChange(CPlayer *pPlayer)
{
	if(GameServer()->Config()->Values()->m_SvPlayerReadyMode && pPlayer->GetTeam() != TEAM_SPECTATORS && !pPlayer->m_DeadSpecMode)
	{
		// change players ready state
		pPlayer->m_IsReadyToPlay ^= 1;

		if(m_GameState == IGS_GAME_RUNNING && !pPlayer->m_IsReadyToPlay)
		{
			SetGameState(IGS_GAME_PAUSED, TIMER_INFINITE); // one player isn't ready -> pause the game
			GameServer()->SendGameMsg(GAMEMSG_GAME_PAUSED, pPlayer->GetCID(), -1);
		}

		CheckReadyStates();
	}
}

// to be called when a player changes state, spectates or disconnects
void IGameController::CheckReadyStates(int WithoutID)
{
	if(GameServer()->Config()->Values()->m_SvPlayerReadyMode)
	{
		switch(m_GameState)
		{
		case IGS_WARMUP_USER:
			// all players are ready -> end warmup
			if(GetPlayersReadyState(WithoutID))
				SetGameState(IGS_WARMUP_USER, 0);
			break;
		case IGS_GAME_PAUSED:
			// all players are ready -> unpause the game
			if(GetPlayersReadyState(WithoutID))
				SetGameState(IGS_GAME_PAUSED, 0);
			break;
		case IGS_GAME_RUNNING:
		case IGS_WARMUP_GAME:
		case IGS_START_COUNTDOWN:
		case IGS_END_MATCH:
		case IGS_END_ROUND:
			// not affected
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
bool IGameController::DoWincheckMatch()
{
	if(IsTeamplay())
	{
		// check score win condition
		if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE] || m_GameFlags&GAMEFLAG_SURVIVAL)
			{
				EndMatch();
				return true;
			}
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
			{
				EndMatch();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return false;
}

void IGameController::ResetGame()
{
	// reset the game
	GameServer()->m_World.m_ResetRequested = true;
	
	SetGameState(IGS_GAME_RUNNING);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;

	CheckGameInfo();

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
				if(Timer < 0)
				{
					m_GameState = GameState;
					m_GameStateTimer = TIMER_INFINITE;
					if(GameServer()->Config()->Values()->m_SvPlayerReadyMode)
					{
						// run warmup till all players are ready
						SetPlayersReadyState(false);
					}
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
			if(GameServer()->Config()->Values()->m_SvCountdown == 0 && m_GameFlags&GAMEFLAG_SURVIVAL)
			{
				m_GameState = GameState;
				m_GameStateTimer = 3*Server()->TickSpeed();
				GameServer()->m_World.m_Paused = true;

			}
			else if(GameServer()->Config()->Values()->m_SvCountdown > 0)
			{
				m_GameState = GameState;
				m_GameStateTimer = GameServer()->Config()->Values()->m_SvCountdown*Server()->TickSpeed();
				GameServer()->m_World.m_Paused = true;
			}
			else
			{
				// no countdown, start new match right away
				SetGameState(IGS_GAME_RUNNING);
			}
		}
		break;
	case IGS_GAME_RUNNING:
		// always possible
		{
			m_GameState = GameState;
			m_GameStateTimer = TIMER_INFINITE;
			SetPlayersReadyState(true);
			GameServer()->m_World.m_Paused = false;
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
				GameServer()->m_World.m_Paused = true;
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
		if(GameState == IGS_END_ROUND && DoWincheckMatch())
			break;
		// only possible when game is running or over
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_END_MATCH || m_GameState == IGS_END_ROUND || m_GameState == IGS_GAME_PAUSED)
		{
			m_GameState = GameState;
			m_GameStateTimer = Timer*Server()->TickSpeed();
			m_SuddenDeath = 0;
			GameServer()->m_World.m_Paused = true;
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

void IGameController::SwapTeamscore()
{
	if(!IsTeamplay())
		return;

	int Score = m_aTeamscore[TEAM_RED];
	m_aTeamscore[TEAM_RED] = m_aTeamscore[TEAM_BLUE];
	m_aTeamscore[TEAM_BLUE] = Score;
}

// general
void IGameController::Snap(int SnappingClient)
{
	CNetObj_GameData *pGameData = static_cast<CNetObj_GameData *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData)));
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
		pGameData->m_GameStateEndTick = Server()->Tick()-m_GameStartTick-TIMER_END/2*Server()->TickSpeed()+m_GameStateTimer;
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
		CNetObj_GameDataTeam *pGameDataTeam = static_cast<CNetObj_GameDataTeam *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATATEAM, 0, sizeof(CNetObj_GameDataTeam)));
		if(!pGameDataTeam)
			return;

		pGameDataTeam->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
		pGameDataTeam->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
	}

	// demo recording
	if(SnappingClient == -1)
	{
		CNetObj_De_GameInfo *pGameInfo = static_cast<CNetObj_De_GameInfo *>(Server()->SnapNewItem(NETOBJTYPE_DE_GAMEINFO, 0, sizeof(CNetObj_De_GameInfo)));
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
				StartRound();
				break;
			case IGS_END_MATCH:
				// start next match
				if(m_MatchCount >= m_GameInfo.m_MatchNum-1)
					CycleMap();

				if(GameServer()->Config()->Values()->m_SvMatchSwap)
					GameServer()->SwapTeams();
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
				if(!GameServer()->Config()->Values()->m_SvPlayerReadyMode && m_GameStateTimer == TIMER_INFINITE)
					SetGameState(IGS_WARMUP_USER, 0);
				else if(m_GameStateTimer == 3 * Server()->TickSpeed())
					StartMatch();
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
			if(Server()->Tick() > m_UnbalancedTick+GameServer()->Config()->Values()->m_SvTeambalanceTime*Server()->TickSpeed()*60)
				DoTeamBalance();
		}
	}

	// check for inactive players
	DoActivityCheck();

	// win check
	if((m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED) && !GameServer()->m_World.m_ResetRequested)
	{
		if(m_GameFlags&GAMEFLAG_SURVIVAL)
			DoWincheckRound();
		else
			DoWincheckMatch();
	}
}

// info
void IGameController::CheckGameInfo()
{
	int MatchNum = (str_length(GameServer()->Config()->Values()->m_SvMaprotation) && GameServer()->Config()->Values()->m_SvMatchesPerMap) ? GameServer()->Config()->Values()->m_SvMatchesPerMap : 0;
	if(MatchNum == 0)
		m_MatchCount = 0;
	bool GameInfoChanged = (m_GameInfo.m_MatchCurrent != m_MatchCount + 1) || (m_GameInfo.m_MatchNum != MatchNum) ||
		(m_GameInfo.m_ScoreLimit != GameServer()->Config()->Values()->m_SvScorelimit) || (m_GameInfo.m_TimeLimit != GameServer()->Config()->Values()->m_SvTimelimit);
	m_GameInfo.m_MatchCurrent = m_MatchCount + 1;
	m_GameInfo.m_MatchNum = MatchNum;
	m_GameInfo.m_ScoreLimit = GameServer()->Config()->Values()->m_SvScorelimit;
	m_GameInfo.m_TimeLimit = GameServer()->Config()->Values()->m_SvTimelimit;
	if(GameInfoChanged)
		UpdateGameInfo(-1);
}

bool IGameController::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2)
		return false;

	if(IsTeamplay())
	{
		if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if(!GameServer()->Config()->Values()->m_SvTeamdamage && GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}

bool IGameController::IsFriendlyTeamFire(int Team1, int Team2) const
{
	return IsTeamplay() && !GameServer()->Config()->Values()->m_SvTeamdamage && Team1 == Team2;
}

bool IGameController::IsPlayerReadyMode() const
{
	return GameServer()->Config()->Values()->m_SvPlayerReadyMode != 0 && (m_GameStateTimer == TIMER_INFINITE && (m_GameState == IGS_WARMUP_USER || m_GameState == IGS_GAME_PAUSED));
}

bool IGameController::IsTeamChangeAllowed() const
{
	return !GameServer()->m_World.m_Paused || (m_GameState == IGS_START_COUNTDOWN && m_GameStartTick == Server()->Tick());
}

void IGameController::UpdateGameInfo(int ClientID)
{
	CNetMsg_Sv_GameInfo GameInfoMsg;
	GameInfoMsg.m_GameFlags = m_GameFlags;
	GameInfoMsg.m_ScoreLimit = m_GameInfo.m_ScoreLimit;
	GameInfoMsg.m_TimeLimit = m_GameInfo.m_TimeLimit;
	GameInfoMsg.m_MatchNum = m_GameInfo.m_MatchNum;
	GameInfoMsg.m_MatchCurrent = m_GameInfo.m_MatchCurrent;

	CNetMsg_Sv_GameInfo GameInfoMsgNoRace = GameInfoMsg;
	GameInfoMsgNoRace.m_GameFlags &= ~GAMEFLAG_RACE;

	if(ClientID == -1)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(!GameServer()->m_apPlayers[i] || !Server()->ClientIngame(i))
				continue;

			CNetMsg_Sv_GameInfo *pInfoMsg = (Server()->GetClientVersion(i) < CGameContext::MIN_RACE_CLIENTVERSION) ? &GameInfoMsgNoRace : &GameInfoMsg;
			Server()->SendPackMsg(pInfoMsg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
		}
	}
	else
	{
		CNetMsg_Sv_GameInfo *pInfoMsg = (Server()->GetClientVersion(ClientID) < CGameContext::MIN_RACE_CLIENTVERSION) ? &GameInfoMsgNoRace : &GameInfoMsg;
		Server()->SendPackMsg(pInfoMsg, MSGFLAG_VITAL|MSGFLAG_NORECORD, ClientID);
	}
}

// map
static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void IGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));

	m_MatchCount = m_GameInfo.m_MatchNum-1;
	if(m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER)
		SetGameState(IGS_GAME_RUNNING);
	EndMatch();
	
	if(m_GameState != IGS_END_MATCH)
	{
		// game could not been ended, force cycle
		CycleMap();
	}
}

void IGameController::CycleMap()
{
	if(m_aMapWish[0] != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "rotating map to %s", m_aMapWish);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		str_copy(GameServer()->Config()->Values()->m_SvMap, m_aMapWish, sizeof(GameServer()->Config()->Values()->m_SvMap));
		m_aMapWish[0] = 0;
		m_MatchCount = 0;
		return;
	}
	if(!str_length(GameServer()->Config()->Values()->m_SvMaprotation))
		return;

	// handle maprotation
	const char *pMapRotation = GameServer()->Config()->Values()->m_SvMaprotation;
	const char *pCurrentMap = GameServer()->Config()->Values()->m_SvMap;

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
	str_copy(GameServer()->Config()->Values()->m_SvMap, &aBuf[i], sizeof(GameServer()->Config()->Values()->m_SvMap));
}

// spawn
bool IGameController::CanSpawn(int Team, vec2 *pOutPos) const
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS || GameServer()->m_World.m_Paused || GameServer()->m_World.m_ResetRequested)
		return false;

	CSpawnEval Eval;
	Eval.m_RandomSpawn = IsSurvival();

	if(IsTeamplay())
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(&Eval, 1+(Team&1));
		if(!Eval.m_Got)
		{
			EvaluateSpawnType(&Eval, 0);
			if(!Eval.m_Got)
				EvaluateSpawnType(&Eval, 1+((Team+1)&1));
		}
	}
	else
	{
		EvaluateSpawnType(&Eval, 0);
		EvaluateSpawnType(&Eval, 1);
		EvaluateSpawnType(&Eval, 2);
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

float IGameController::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos) const
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
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

void IGameController::EvaluateSpawnType(CSpawnEval *pEval, int Type) const
{
	// get spawn point
	for(int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = GameServer()->m_World.FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
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
		float S = pEval->m_RandomSpawn ? random_int() : EvaluateSpawnPos(pEval, P);
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
	if(!IsTeamplay() || JoinTeam == TEAM_SPECTATORS || !GameServer()->Config()->Values()->m_SvTeambalanceTime)
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
	return TeamMod+m_aTeamSize[TEAM_RED]+m_aTeamSize[TEAM_BLUE] < GameServer()->Config()->Values()->m_SvPlayerSlots;
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
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d->%d", ClientID, Server()->ClientName(ClientID), OldTeam, Team);
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
	CheckReadyStates();

	// reset inactivity counter when joining the game
	if(OldTeam == TEAM_SPECTATORS)
		pPlayer->m_InactivityTickCounter = 0;
}

int IGameController::GetStartTeam()
{
	if(GameServer()->Config()->Values()->m_SvTournamentMode)
		return TEAM_SPECTATORS;

	// determine new team
	int Team = TEAM_RED;
	if(IsTeamplay())
	{
		if(!GameServer()->Config()->Values()->m_DbgStress)	// this will force the auto balancer to work overtime aswell
			Team = m_aTeamSize[TEAM_RED] > m_aTeamSize[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;
	}

	// check if there're enough player slots left
	if(m_aTeamSize[TEAM_RED]+m_aTeamSize[TEAM_BLUE] < GameServer()->Config()->Values()->m_SvPlayerSlots)
	{
		++m_aTeamSize[Team];
		m_UnbalancedTick = TBALANCE_CHECK;
		if(m_GameState == IGS_WARMUP_GAME && HasEnoughPlayers())
			SetGameState(IGS_WARMUP_GAME, 0);
		return Team;
	}
	return TEAM_SPECTATORS;
}

IGameController::CChatCommands::CChatCommands()
{
	mem_zero(m_aCommands, sizeof(m_aCommands));
}

void IGameController::CChatCommands::AddCommand(const char *pName, const char *pArgsFormat, const char *pHelpText, COMMAND_CALLBACK pfnCallback)
{
	if(GetCommand(pName))
		return;

	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(!m_aCommands[i].m_Used)
		{
			mem_zero(&m_aCommands[i], sizeof(CChatCommand));

			str_copy(m_aCommands[i].m_aName, pName, sizeof(m_aCommands[i].m_aName));
			str_copy(m_aCommands[i].m_aHelpText, pHelpText, sizeof(m_aCommands[i].m_aHelpText));
			str_copy(m_aCommands[i].m_aArgsFormat, pArgsFormat, sizeof(m_aCommands[i].m_aArgsFormat));

			m_aCommands[i].m_pfnCallback = pfnCallback;
			m_aCommands[i].m_Used = true;
			break;
		}
	}
}

void IGameController::CChatCommands::SendRemoveCommand(IServer *pServer, const char *pName, int ID)
{
	CNetMsg_Sv_CommandInfoRemove Msg;
	Msg.m_pName = pName;

	pServer->SendPackMsg(&Msg, MSGFLAG_VITAL, ID);
}

void IGameController::CChatCommands::RemoveCommand(const char *pName)
{
	CChatCommand *pCommand = GetCommand(pName);

	if(pCommand)
	{
		mem_zero(pCommand, sizeof(CChatCommand));
	}
}

IGameController::CChatCommand *IGameController::CChatCommands::GetCommand(const char *pName)
{
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(m_aCommands[i].m_Used && str_comp(m_aCommands[i].m_aName, pName) == 0)
		{
			return &m_aCommands[i];
		}
	}
	return 0;
}

void IGameController::CChatCommands::OnPlayerConnect(IServer *pServer, CPlayer *pPlayer)
{
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		CChatCommand *pCommand = &m_aCommands[i];

		if(pCommand->m_Used)
		{
			CNetMsg_Sv_CommandInfo Msg;
			Msg.m_pName = pCommand->m_aName;
			Msg.m_HelpText = pCommand->m_aHelpText;
			Msg.m_ArgsFormat = pCommand->m_aArgsFormat;

			pServer->SendPackMsg(&Msg, MSGFLAG_VITAL, pPlayer->GetCID());
		}
	}
}

void IGameController::OnPlayerCommand(CPlayer *pPlayer, const char *pCommandName, const char *pCommandArgs)
{
	// TODO: Add a argument parser?
	CChatCommand *pCommand = CommandsManager()->GetCommand(pCommandName);

	if(pCommand)
		pCommand->m_pfnCallback(pPlayer, pCommandArgs);
}

void IGameController::CChatCommands::OnInit()
{
	//AddCommand("example", "si", "I am a description", Com_Example);
}

/*void IGameController::Com_Example(class CPlayer *pPlayer, const char *pArgs)
{
	// Do something with the player here
}*/
