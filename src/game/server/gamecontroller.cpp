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
	m_UnbalancedTick = TBALANCE_OK;

	// game
	m_GameState = GS_GAME;
	m_GameStateTimer = TIMER_INFINITE;
	m_GameStartTick = Server()->Tick();
	m_MatchCount = 0;
	m_StartCountdownType = SCT_DEFAULT;
	m_SuddenDeath = 0;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;
	if(g_Config.m_SvWarmup)
		SetGameState(GS_WARMUP, g_Config.m_SvWarmup);
	else
		SetGameState(GS_STARTCOUNTDOWN, TIMER_STARTCOUNTDOWN);

	// info
	m_GameFlags = 0;
	m_pGameType = "unknown";

	// map
	m_aMapWish[0] = 0;

	// spawn
	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;
}

//activity
void IGameController::DoActivityCheck()
{
	if(g_Config.m_SvInactiveKickTime == 0)
		return;

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->IsDummy() && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
			!Server()->IsAuthed(i) && (Server()->Tick() > GameServer()->m_apPlayers[i]->m_LastActionTick+g_Config.m_SvInactiveKickTime*Server()->TickSpeed()*60))
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
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
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

	// calc team sizes
	int aPlayerCount[NUM_TEAMS] = {0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			aPlayerCount[GameServer()->m_apPlayers[i]->GetTeam()]++;
	}

	// check if teams are unbalanced
	char aBuf[256];
	if(absolute(aPlayerCount[TEAM_RED]-aPlayerCount[TEAM_BLUE]) >= NUM_TEAMS)
	{
		str_format(aBuf, sizeof(aBuf), "Teams are NOT balanced (red=%d blue=%d)", aPlayerCount[TEAM_RED], aPlayerCount[TEAM_BLUE]);
		if(m_UnbalancedTick <= TBALANCE_OK)
			m_UnbalancedTick = Server()->Tick();
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Teams are balanced (red=%d blue=%d)", aPlayerCount[TEAM_RED], aPlayerCount[TEAM_BLUE]);
		m_UnbalancedTick = TBALANCE_OK;
	}
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

void IGameController::DoTeamBalance()
{
	if(!IsTeamplay())
		return;

	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "Balancing teams");

	int aPlayerCount[NUM_TEAMS] = {0};
	float aTeamScore[NUM_TEAMS] = {0};
	float aPlayerScore[MAX_CLIENTS] = {0.0f};

	// gather stats
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			aPlayerCount[GameServer()->m_apPlayers[i]->GetTeam()]++;
			aPlayerScore[i] = GameServer()->m_apPlayers[i]->m_Score*Server()->TickSpeed()*60.0f/
				(Server()->Tick()-GameServer()->m_apPlayers[i]->m_ScoreStartTick);
			aTeamScore[GameServer()->m_apPlayers[i]->GetTeam()] += aPlayerScore[i];
		}
	}

	// check if teams are unbalanced
	if(absolute(aPlayerCount[TEAM_RED]-aPlayerCount[TEAM_BLUE]) >= NUM_TEAMS)
	{
		int BiggerTeam = (aPlayerCount[TEAM_RED] > aPlayerCount[TEAM_BLUE]) ? TEAM_RED : TEAM_BLUE;
		int NumBalance = absolute(aPlayerCount[TEAM_RED]-aPlayerCount[TEAM_BLUE]) / NUM_TEAMS;

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
			int Temp = pPlayer->m_LastActionTick;
			DoTeamChange(pPlayer, BiggerTeam^1);
			pPlayer->m_LastActionTick = Temp;
			pPlayer->Respawn();
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "You were moved to %s due to team balancing", GetTeamName(pPlayer->GetTeam()));
			GameServer()->SendBroadcast(aBuf, pPlayer->GetCID());
		}
		while(--NumBalance);
	}

	m_UnbalancedTick = TBALANCE_OK;
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
}

// event
int IGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
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
	return 0;
}

void IGameController::OnCharacterSpawn(class CCharacter *pChr)
{
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		// give start equipment
		pChr->IncreaseHealth(10);
		pChr->IncreaseArmor(5);

		pChr->GiveWeapon(WEAPON_HAMMER, -1);
		pChr->GiveWeapon(WEAPON_GUN, 10);
		pChr->GiveWeapon(WEAPON_SHOTGUN, 10);
		pChr->GiveWeapon(WEAPON_GRENADE, 10);
		pChr->GiveWeapon(WEAPON_RIFLE, 5);

		// prevent respawn
		pChr->GetPlayer()->m_RespawnDisabled = GetStartRespawnState();
	}
	else
	{
		// default health
		pChr->IncreaseHealth(10);

		// give default weapons
		pChr->GiveWeapon(WEAPON_HAMMER, -1);
		pChr->GiveWeapon(WEAPON_GUN, 10);
	}
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
	int SubType = 0;

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
		Type = POWERUP_ARMOR;
		break;
	case ENTITY_HEALTH_1:
		Type = POWERUP_HEALTH;
		break;
	case ENTITY_WEAPON_SHOTGUN:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
		break;
	case ENTITY_WEAPON_GRENADE:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
		break;
	case ENTITY_WEAPON_RIFLE:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
		break;
	case ENTITY_POWERUP_NINJA:
		if(g_Config.m_SvPowerups)
		{
			Type = POWERUP_NINJA;
			SubType = WEAPON_NINJA;
		}
	}

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

void IGameController::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	pPlayer->OnDisconnect();

	int ClientID = pPlayer->GetCID();
	if(Server()->ClientIngame(ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientID, Server()->ClientName(ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}

	m_UnbalancedTick = TBALANCE_CHECK;
}

void IGameController::OnPlayerInfoChange(class CPlayer *pPlayer)
{
	const int aTeamColors[2] = {65387, 10223467};
	if(IsTeamplay())
	{
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		if(pPlayer->GetTeam() >= TEAM_RED && pPlayer->GetTeam() <= TEAM_BLUE)
		{
			pPlayer->m_TeeInfos.m_ColorBody = aTeamColors[pPlayer->GetTeam()];
			pPlayer->m_TeeInfos.m_ColorFeet = aTeamColors[pPlayer->GetTeam()];
		}
		else
		{
			pPlayer->m_TeeInfos.m_ColorBody = 12895054;
			pPlayer->m_TeeInfos.m_ColorFeet = 12895054;
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
			GameServer()->m_apPlayers[i]->m_Score = 0;
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
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
		if((g_Config.m_SvScorelimit > 0 && (m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit)) ||
			(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_GameStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
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
		if((g_Config.m_SvScorelimit > 0 && Topscore >= g_Config.m_SvScorelimit) ||
			(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_GameStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
		{
			if(TopscoreCount == 1)
				EndMatch();
			else
				m_SuddenDeath = 1;
		}
	}
}

void IGameController::EndMatch()
{
	SetGameState(GS_GAMEOVER, 10);
}

void IGameController::EndRound()
{
	SetGameState(GS_ROUNDOVER, 10);
}

void IGameController::ResetGame()
{
	GameServer()->m_World.m_ResetRequested = true;
	
	SetGameState(GS_GAME);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;
}

void IGameController::SetGameState(int GameState, int Seconds)
{
	switch(GameState)
	{
	case GS_WARMUP:
		{
			if(GetGameState() == GS_GAME || GetGameState() == GS_WARMUP)
			{
				if(Seconds < 0 && g_Config.m_SvPlayerReadyMode)
				{
					m_GameState = GS_WARMUP;
					m_GameStateTimer = TIMER_INFINITE;
					SetPlayersReadyState(false);
				}
				else if(Seconds > 0)
				{
					m_GameState = GS_WARMUP;
					m_GameStateTimer = Seconds*Server()->TickSpeed();
				}
				else
					SetGameState(GS_GAME);

				if(GetGameState() == GS_WARMUP)
				{
					for(int i = 0; i < MAX_CLIENTS; ++i)
						if(GameServer()->m_apPlayers[i])
							GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
				}
			}
		}
		break;
	case GS_STARTCOUNTDOWN:
		{
			if(m_GameState == GS_GAME || m_GameState == GS_STARTCOUNTDOWN)
			{
				m_GameState = GS_STARTCOUNTDOWN;
				m_GameStateTimer = Seconds*Server()->TickSpeed();
				m_StartCountdownType = SCT_DEFAULT;
				GameServer()->m_World.m_Paused = true;
			}
		}
		break;
	case GS_GAME:
		{
			m_GameState = GS_GAME;
			m_GameStateTimer = TIMER_INFINITE;
			SetPlayersReadyState(true);
			GameServer()->m_World.m_Paused = false;
		}
		break;
	case GS_PAUSED:
		{
			if(GetGameState() == GS_GAME || GetGameState() == GS_PAUSED)
			{
				if(Seconds != 0)
				{
					if(Seconds < 0)
					{
						m_GameStateTimer = TIMER_INFINITE;
						SetPlayersReadyState(false);
					}
					else
						m_GameStateTimer = Seconds*Server()->TickSpeed();

					m_GameState = GS_PAUSED;
					GameServer()->m_World.m_Paused = true;
				}
 				else
				{
					bool NeedStartCountdown = m_GameStateTimer != 0;
					SetGameState(GS_GAME);
					if(NeedStartCountdown)
					{
						SetGameState(GS_STARTCOUNTDOWN, TIMER_STARTCOUNTDOWN);
						m_StartCountdownType = SCT_PAUSED;
					}
				}
			}
		}
		break;
	case GS_ROUNDOVER:
		{
			if(GetGameState() != GS_WARMUP && GetGameState() != GS_PAUSED)
			{
				m_GameState = GS_ROUNDOVER;
				m_GameStateTimer = Seconds*Server()->TickSpeed();
				m_SuddenDeath = 0;
				GameServer()->m_World.m_Paused = true;			
			}
		}
		break;
	case GS_GAMEOVER:
		{
			if(GetGameState() != GS_WARMUP && GetGameState() != GS_PAUSED)
			{
				m_GameState = GS_GAMEOVER;
				m_GameStateTimer = Seconds*Server()->TickSpeed();
				m_SuddenDeath = 0;
				GameServer()->m_World.m_Paused = true;			
			}
		}
	}
}

void IGameController::StartMatch()
{
	ResetGame();

	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;

	if(IsTeamplay())
	{
		if(m_UnbalancedTick == TBALANCE_CHECK)
			CheckTeamBalance();
		if(m_UnbalancedTick > TBALANCE_OK)
			DoTeamBalance();
	}

	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start match type='%s' teamplay='%d'", m_pGameType, m_GameFlags&GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

// general
void IGameController::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;

	pGameInfoObj->m_GameStateFlags = 0;
	pGameInfoObj->m_GameStateTimer = 0;
	switch(GetGameState())
	{
	case GS_WARMUP:
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_WARMUP;
		pGameInfoObj->m_GameStateTimer = m_GameStateTimer;
		break;
	case GS_STARTCOUNTDOWN:
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED;
		pGameInfoObj->m_GameStateTimer = m_GameStateTimer;
		break;
	case GS_PAUSED:
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
		pGameInfoObj->m_GameStateTimer = m_GameStateTimer;
		break;
	case GS_ROUNDOVER:
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_ROUNDOVER;
		pGameInfoObj->m_GameStateTimer = Server()->Tick()-m_GameStartTick-10*Server()->TickSpeed()+m_GameStateTimer;
		break;
	case GS_GAMEOVER:
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
		pGameInfoObj->m_GameStateTimer = Server()->Tick()-m_GameStartTick-10*Server()->TickSpeed()+m_GameStateTimer;
	}
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;

	pGameInfoObj->m_GameStartTick = m_GameStartTick;

	pGameInfoObj->m_ScoreLimit = g_Config.m_SvScorelimit;
	pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;

	pGameInfoObj->m_MatchNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvMatchesPerMap) ? g_Config.m_SvMatchesPerMap : 0;
	pGameInfoObj->m_MatchCurrent = m_MatchCount+1;
}

void IGameController::Tick()
{
	// handle game states
	if(GetGameState() != GS_GAME)
	{
		if(m_GameStateTimer > 0)
			--m_GameStateTimer;

		switch(GetGameState())
		{
		case GS_WARMUP:
			if(m_GameStateTimer == 0 || (m_GameStateTimer == TIMER_INFINITE && (!g_Config.m_SvPlayerReadyMode || GetPlayersReadyState())))
			{
				if(m_GameStateTimer == 0)
					StartMatch();
				else
					SetGameState(GS_STARTCOUNTDOWN, TIMER_STARTCOUNTDOWN);
			}
			break;
		case GS_STARTCOUNTDOWN:
			if(m_GameStateTimer == 0)
			{
				switch(m_StartCountdownType)
				{
				case SCT_PAUSED:
					SetGameState(GS_GAME);
					break;
				case SCT_ROUNDOVER:
					ResetGame();
					break;
				case SCT_DEFAULT:
					StartMatch();
				};
			}
			else
				++m_GameStartTick;
			break;
		case GS_PAUSED:
			if(m_GameStateTimer == 0 || (m_GameStateTimer == TIMER_INFINITE && g_Config.m_SvPlayerReadyMode && GetPlayersReadyState()))
				SetGameState(GS_PAUSED, 0);
			else
				++m_GameStartTick;
			break;
		case GS_ROUNDOVER:
			if(m_GameStateTimer == 0)
			{
				ResetGame();
				DoWincheckMatch();
				if(IsTeamplay())
				{
					if(m_UnbalancedTick == TBALANCE_CHECK)
						CheckTeamBalance();
					if(m_UnbalancedTick > TBALANCE_OK && Server()->Tick() > m_UnbalancedTick+g_Config.m_SvTeambalanceTime*Server()->TickSpeed()*60)
						DoTeamBalance();
				}
				SetGameState(GS_STARTCOUNTDOWN, TIMER_STARTCOUNTDOWN);
				m_StartCountdownType = SCT_ROUNDOVER;
			}
			break;
		case GS_GAMEOVER:
			if(m_GameStateTimer == 0)
			{
				CycleMap();
				StartMatch();
				m_MatchCount++;
			}
		}
	}

	// check if the game needs to be paused
	if(GetGameState() != GS_PAUSED && g_Config.m_SvPlayerReadyMode && !GetPlayersReadyState())
		SetGameState(GS_PAUSED, TIMER_INFINITE);

	// do team-balancing
	if(IsTeamplay() && !(m_GameFlags&GAMEFLAG_SURVIVAL))
	{
		if(m_UnbalancedTick == TBALANCE_CHECK)
			CheckTeamBalance();
		if(m_UnbalancedTick > TBALANCE_OK && Server()->Tick() > m_UnbalancedTick+g_Config.m_SvTeambalanceTime*Server()->TickSpeed()*60)
			DoTeamBalance();
	}

	// check for inactive players
	DoActivityCheck();

	// win check
	if(GetGameState() == GS_GAME && !GameServer()->m_World.m_ResetRequested)
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
	return g_Config.m_SvPlayerReadyMode != 0 && (m_GameStateTimer == TIMER_INFINITE && (GetGameState() == GS_WARMUP || GetGameState() == GS_PAUSED));
}

const char *IGameController::GetTeamName(int Team) const
{
	if(IsTeamplay())
	{
		if(Team == TEAM_RED)
			return "red team";
		else if(Team == TEAM_BLUE)
			return "blue team";
	}
	else
	{
		if(Team == 0)
			return "game";
	}

	return "spectators";
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

	if(m_MatchCount < g_Config.m_SvMatchesPerMap-1)
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
	char aBuf[512];
	for(int i = 0; i < 512; i++)
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
bool IGameController::CanSpawn(int Team, vec2 *pOutPos) const
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS || GameServer()->m_World.m_Paused || GameServer()->m_World.m_ResetRequested)
		return false;

	CSpawnEval Eval;

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

		float d = distance(Pos, pC->m_Pos);
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
					distance(aEnts[c]->m_Pos, m_aaSpawnPoints[Type][i]+Positions[Index]) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
		}
		if(Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i]+Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
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
		if(GetGameState() == GS_WARMUP || (GetGameState() == GS_STARTCOUNTDOWN && m_StartCountdownType == SCT_DEFAULT))
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

	// calc team sizes
	int aPlayerCount[NUM_TEAMS] = {0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			aPlayerCount[GameServer()->m_apPlayers[i]->GetTeam()]++;
	}

	// simulate what would happen if the player changes team
	aPlayerCount[JoinTeam]++;
	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
		aPlayerCount[JoinTeam^1]--;

	// check if the player-difference decreases or is smaller than 2
	return aPlayerCount[JoinTeam]-aPlayerCount[JoinTeam^1] < NUM_TEAMS;
}

bool IGameController::CanJoinTeam(int Team, int NotThisID) const
{
	if(Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int PlayerCount = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			++PlayerCount;
	}

	return PlayerCount < Server()->MaxClients()-g_Config.m_SvSpectatorSlots;
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

	pPlayer->SetTeam(Team);

	int ClientID = pPlayer->GetCID();
	char aBuf[128];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(ClientID), GetTeamName(Team));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", ClientID, Server()->ClientName(ClientID), Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	m_UnbalancedTick = TBALANCE_CHECK;
	pPlayer->m_IsReadyToPlay = GetGameState() != GS_WARMUP && GetGameState() != GS_PAUSED;
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
		pPlayer->m_RespawnDisabled = GetStartRespawnState();
	OnPlayerInfoChange(pPlayer);
}

int IGameController::GetStartTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime aswell
	if(g_Config.m_DbgStress)
		return TEAM_RED;

	if(g_Config.m_SvTournamentMode)
		return TEAM_SPECTATORS;

	int aPlayerCount[NUM_TEAMS] = {0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i != NotThisID && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
			aPlayerCount[GameServer()->m_apPlayers[i]->GetTeam()]++;
	}

	int Team = TEAM_RED;
	if(IsTeamplay())
		Team = aPlayerCount[TEAM_RED] > aPlayerCount[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;

	if(aPlayerCount[TEAM_RED]+aPlayerCount[TEAM_BLUE] < Server()->MaxClients()-g_Config.m_SvSpectatorSlots)
	{
		m_UnbalancedTick = TBALANCE_CHECK;
		return Team;
	}
	return TEAM_SPECTATORS;
}
