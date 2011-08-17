/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/entities/loltext.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();

	m_pAccount = 0;
	blockScore = 0;
	m_LastAnnoyingMsg = 0;

	int* idMap = Server()->GetIdMap(ClientID);
	for (int i = 1;i < VANILLA_MAX_CLIENTS;i++)
	{
	    idMap[i] = -1;
	}
	idMap[0] = ClientID;
	m_ChatScore = 0;

	m_TeamChangeTick = Server()->Tick();
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	if (GetAccount())
	{
		blockScore = GetAccount()->Payload()->blockScore;
	}

	Server()->SetClientScore(m_ClientID, blockScore);

	if (m_ChatScore > 0)
		m_ChatScore--;

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!m_pCharacter && m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW)
		m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

	if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
		m_Spawning = true;

	if(m_pCharacter)
	{
		if(m_pCharacter->IsAlive())
		{
			m_ViewPos = m_pCharacter->m_Pos;
		}
		else
		{
			delete m_pCharacter;
			m_pCharacter = 0;
		}
	}
	else if(m_Spawning && m_RespawnTick <= Server()->Tick())
		TryRespawn();
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	int id = m_ClientID;
	if (!Server()->Translate(id, SnappingClient)) return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	const char* DbgStateChars = "fsizb";

	if (g_Config.m_SvScoringDebug && m_pCharacter)
	{
		char dbgName[200];
		str_format(dbgName, sizeof(dbgName), "%c%d::%d%s", DbgStateChars[m_pCharacter->State], m_pCharacter->lastInteractionPlayer, m_ClientID, Server()->ClientName(m_ClientID));
		StrToInts(&pClientInfo->m_Name0, 4, dbgName);
	}
	else
		StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));
	if (!g_Config.m_SvLoginClan || !GetAccount())
		StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	else StrToInts(&pClientInfo->m_Clan0, 3, GetAccount()->Name());
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
	if (m_StolenSkin && SnappingClient != m_ClientID && g_Config.m_SvSkinStealAction == 1)
	{
		StrToInts(&pClientInfo->m_Skin0, 6, "pinky");
		pClientInfo->m_UseCustomColor = 0;
		pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
		pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;
	} else
	{
		StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
		pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
		pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
		pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;
	}

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, id, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_Score = blockScore;
	pPlayerInfo->m_ClientID = id;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::FakeSnap(int SnappingClient)
{
	// WORK IN PROGRESS STUFF NOT FINISHED
	int id = VANILLA_MAX_CLIENTS - 1;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, " ");
	StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OverrideColors(int Color)
{
	bool Ch = false;
	
	if (Color < 0) // reset
	{
		Ch = (m_TeeInfos.m_UseCustomColor != m_OrigTeeInfos.m_UseCustomColor)
			|| (m_TeeInfos.m_ColorBody != m_OrigTeeInfos.m_ColorBody)
			|| (m_TeeInfos.m_ColorFeet != m_OrigTeeInfos.m_ColorFeet);
		m_EnforcedColors = false;
		m_TeeInfos.m_UseCustomColor = m_OrigTeeInfos.m_UseCustomColor;	
		m_TeeInfos.m_ColorBody = m_OrigTeeInfos.m_ColorBody;	
		m_TeeInfos.m_ColorFeet = m_OrigTeeInfos.m_ColorFeet;	
	}
	else if (g_Config.m_SvOverrideColor)
	{
		Ch = (m_TeeInfos.m_UseCustomColor != 1)
			|| (m_TeeInfos.m_ColorBody != Color)
			|| (m_TeeInfos.m_ColorFeet != Color);
		m_EnforcedColors = true;
		m_TeeInfos.m_UseCustomColor = 1;
		m_TeeInfos.m_ColorBody = m_TeeInfos.m_ColorFeet = Color;	
	}
	if (Ch)
		GameServer()->m_pController->OnPlayerInfoChange(this);
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

int CPlayer::BlockKillCheck()
{
	int killer = m_ClientID;
	if (m_pCharacter->State != BS_FROZEN)
		return killer;

	if (Server()->ClientIngame(m_pCharacter->lastInteractionPlayer))
	{
		char aVictimText[16] = {0};//for loltext
		char aKillerText[16] = {0};

		killer = m_pCharacter->lastInteractionPlayer;
		CAccount* killerAcc = GameServer()->m_apPlayers[killer]->GetAccount();

		bool scorewhore = false;
		if (killerAcc && killerAcc->Head()->m_LastLoginDate > time_timestamp() - g_Config.m_SvFrozenBlocked / 1000)
			scorewhore = true;

		double scoreStolen = min(blockScore * g_Config.m_SvScoreSteal, GameServer()->m_apPlayers[killer]->blockScore * g_Config.m_SvScoreStealLimit) / 100;
		if (scorewhore) scoreStolen *= -3;
		blockScore -= scoreStolen;

		if (GetAccount())
		{
			if (g_Config.m_SvScoringDebugLog)
				dbg_msg("score","%s killed by %s and lost %.1f, now has %.1f", GetAccount()->Name(), GameServer()->m_apPlayers[killer]->GetAccount() ? GameServer()->m_apPlayers[killer]->GetAccount()->Name() : "(unk)", scoreStolen, blockScore);
			GetAccount()->Payload()->blockScore = blockScore;
			GameServer()->m_Rank.UpdateScore(GetAccount());
			if (fabs(scoreStolen) >= .5f)
				str_format(aVictimText, sizeof aVictimText, "%+.1f", -scoreStolen);
		}

		if (killerAcc)
		{
			double minSteal = (double)g_Config.m_SvScoreCreep / 1000;
			if (!scorewhore && scoreStolen < minSteal) // if killer has account, give him score for unreg creeps
				scoreStolen = minSteal;

			GameServer()->m_apPlayers[killer]->blockScore = max(scoreStolen + GameServer()->m_apPlayers[killer]->blockScore, 0.);

			if (g_Config.m_SvScoringDebugLog)
				dbg_msg("score","%s killed %s and gained %.1f, now has %.1f", killerAcc->Name(), GetAccount() ? GetAccount()->Name() : "(unk)", scoreStolen, GameServer()->m_apPlayers[killer]->blockScore);
			killerAcc->Payload()->blockScore = GameServer()->m_apPlayers[killer]->blockScore;
			GameServer()->m_Rank.UpdateScore(killerAcc);
			if (fabs(scoreStolen) >= .5f)
				str_format(aKillerText, sizeof aKillerText, "%+.1f", scoreStolen);
		}
		else
		{
			if (g_Config.m_SvRegisterMessageInterval != 0 && (m_LastAnnoyingMsg == 0 || Server()->Tick() - m_LastAnnoyingMsg > Server()->TickSpeed()*g_Config.m_SvRegisterMessageInterval))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s, say /reg in chat to register and gain score for killing %s", Server()->ClientName(killer), Server()->ClientName(m_ClientID));
				GameServer()->SendChatTarget(killer, aBuf);
				m_LastAnnoyingMsg = Server()->Tick();
			}
			str_copy(aKillerText, "!", sizeof aKillerText);
		}

		CCharacter *killerchar = GameServer()->GetPlayerChar(killer);

		if (*aVictimText && *aKillerText && killerchar)
		{
			vec2 Vs = CLoltext::TextSize(aVictimText);
			vec2 Ks = CLoltext::TextSize(aKillerText);
			// no full overlap check here, its way cheaper to disregard Y and just consider size vs dx

			float XDiff = absolute(m_pCharacter->m_Pos.x - killerchar->m_Pos.x);
			float YDiff = absolute(m_pCharacter->m_Pos.y - killerchar->m_Pos.y);

			if (XDiff < 0.5f*(Vs.x + Ks.x) && YDiff < 0.5f*(Vs.y + Ks.y))
			{
				*aVictimText = 0;
				str_append(aKillerText, "-", sizeof aKillerText);
			}
		}
		if (*aKillerText && killerchar)
			GameServer()->CreateLolText(GameServer()->GetPlayerChar(killer), false, vec2(0,-100), vec2(0.f,0.f), 50, aKillerText);
		if (*aVictimText)
			GameServer()->CreateLolText(m_pCharacter, false, vec2(0,-100), vec2(0.f,0.f), 50, aVictimText);

		if (killerchar) //TODO find out why, i think it needs well-timed kill just when scoring
		{
			killerchar->BlockScored();
			if (m_pCharacter->m_chatFrozen)
				killerchar->ChatBlockScored();
		}
	}
	return killer;
}

void CPlayer::BlockKill()
{
	if (!m_pCharacter) return;
	int killer = BlockKillCheck();
	if (killer == m_ClientID) return;
	m_pCharacter->SendKillMsg(killer, WEAPON_HAMMER, 0);
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		int killer = BlockKillCheck();
		m_pCharacter->Die(killer, killer == m_ClientID ? Weapon : WEAPON_NINJA);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos))
		return;

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos);
}

void CPlayer::FindDuplicateSkins()
{
	if (m_TeeInfos.m_UseCustomColor == 0 && !m_StolenSkin) return;
	m_StolenSkin = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (i == m_ClientID) continue;
		if(GameServer()->m_apPlayers[i])
		{
			if (GameServer()->m_apPlayers[i]->m_StolenSkin) continue;
			if ((GameServer()->m_apPlayers[i]->m_OrigTeeInfos.m_UseCustomColor == m_TeeInfos.m_UseCustomColor) &&
			(GameServer()->m_apPlayers[i]->m_OrigTeeInfos.m_ColorFeet == m_OrigTeeInfos.m_ColorFeet) &&
			(GameServer()->m_apPlayers[i]->m_OrigTeeInfos.m_ColorBody == m_OrigTeeInfos.m_ColorBody) &&
			!str_comp(GameServer()->m_apPlayers[i]->m_TeeInfos.m_SkinName, m_TeeInfos.m_SkinName))
			{
				m_StolenSkin = 1;
				return;
			}
		}
	}
}
