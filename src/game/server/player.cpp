/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
//#include <stdio.h> //TODO:GFX check if linux still needs this
#include <engine/server.h>
#include <engine/server/server.h>
#include <engine/shared/config.h>

#include "player.h"
#include "gamecontext.h"
#include <game/gamecore.h>
#include "gamemodes/DDRace.h"

MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }
	
CPlayer::CPlayer(CGameContext *pGameServer, int CID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	Character = 0;
	m_Muted = 0;
	this->m_ClientID = CID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_LastActionTick = Server()->Tick();

	m_PauseInfo.m_Respawn = false;
	
	GameServer()->Score()->PlayerData(CID)->Reset();
	
	m_Invisible = false;
	m_IsUsingDDRaceClient = false;

	// Variable initialized:
	m_Last_Pause = 0;
}

CPlayer::~CPlayer()
{
	if(Character) Character->Destroy();
}

void CPlayer::Tick()
{
	Server()->SetClientAuthed(m_ClientID, m_Authed);
	if(!Server()->ClientIngame(m_ClientID))
		return;

	Server()->SetClientScore(m_ClientID, m_Score);

	if(m_Muted > 0) m_Muted--;
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
	
	if(!Character && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
		m_Spawning = true;

	if(Character)
	{
		if(Character->IsAlive())
		{
			m_ViewPos = Character->m_Pos;
		}
		else
		{
			Character->MarkDestroy();
			Character = 0;
		}
	}
	else if(m_Spawning && m_RespawnTick <= Server()->Tick())
		TryRespawn();
}

void CPlayer::Snap(int SnappingClient)
{
	if(!Server()->ClientIngame(m_ClientID))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 6, Server()->ClientName(m_ClientID));
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = m_Latency.m_Min;
	pPlayerInfo->m_LatencyFlux = m_Latency.m_Max-m_Latency.m_Min;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientId = m_ClientID;
	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;	
	
	// send 0 if times of others are not shown
	if(SnappingClient != m_ClientID && g_Config.m_SvHideScore)
		pPlayerInfo->m_Score = 0;
	else
		pPlayerInfo->m_Score = m_Score;
		
	pPlayerInfo->m_Team = m_Team;
}

void CPlayer::OnDisconnect()
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf),  "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		char Cmd[64];
		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		if(m_Muted > 0) {
			str_format(Cmd, sizeof(Cmd), "ban %d %d '%s'", m_ClientID, m_Muted/Server()->TickSpeed(), "Mute evasion");
			GameServer()->Console()->ExecuteLine(Cmd, 3, -1);
		}
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	if(Character)
		Character->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(Character)
		Character->OnDirectInput(NewInput);

	if(!Character && m_Team >= 0 && (NewInput->m_Fire&1))
		m_Spawning = true;
	
	if(!Character && m_Team == -1)
		m_ViewPos = vec2(NewInput->m_TargetX, NewInput->m_TargetY);

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
	if(Character && Character->IsAlive())
		return Character;
	return 0;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(Character)
	{
		Character->Die(m_ClientID, Weapon);
		//delete Character;
		Character = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team > -1)
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
	//str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	//GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	
	//GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);
}

void CPlayer::TryRespawn()
{
	if(m_PauseInfo.m_Respawn)
	{
		Character = new(m_ClientID) CCharacter(&GameServer()->m_World);
		Character->Spawn(this, m_PauseInfo.m_Core.m_Pos);
		GameServer()->CreatePlayerSpawn(m_PauseInfo.m_Core.m_Pos);
		LoadCharacter();
	}
	else
	{
		vec2 SpawnPos = vec2(100.0f, -60.0f);
		if(!GameServer()->m_pController->CanSpawn(this, &SpawnPos))
			return;

		// check if the position is occupado
		CEntity *apEnts[2] = {0};
		int NumEnts = GameServer()->m_World.FindEntities(SpawnPos, 64, apEnts, 2, NETOBJTYPE_CHARACTER);
		if(NumEnts < 3)
		{
			m_Spawning = false;
			Character = new(m_ClientID) CCharacter(&GameServer()->m_World);
			Character->Spawn(this, SpawnPos);
			GameServer()->CreatePlayerSpawn(SpawnPos);
		} 
	}
}

void CPlayer::LoadCharacter()
{
	Character->m_Core = m_PauseInfo.m_Core;
	if(g_Config.m_SvPauseTime)
		Character->m_StartTime = Server()->Tick() - (m_PauseInfo.m_PauseTime - m_PauseInfo.m_StartTime);
	else
		Character->m_StartTime = m_PauseInfo.m_StartTime;
	Character->m_DDRaceState = m_PauseInfo.m_DDRaceState;
	Character->m_RefreshTime = Server()->Tick();
	for(int i = 0; i < NUM_WEAPONS; ++i)
	{
		if(m_PauseInfo.m_aHasWeapon[i])
		{
			if(!m_PauseInfo.m_FreezeTime)
				Character->GiveWeapon(i, -1);
			else
				Character->GiveWeapon(i, 0);
		}
	}
	Character->m_FreezeTime = m_PauseInfo.m_FreezeTime;
	Character->m_Doored = m_PauseInfo.m_Doored;
	Character->m_OldPos = m_PauseInfo.m_OldPos;
	Character->m_OlderPos = m_PauseInfo.m_OlderPos;
	Character->m_LastAction = m_PauseInfo.m_LastAction;
	Character->m_Jumped = m_PauseInfo.m_Jumped;
	Character->m_Health = m_PauseInfo.m_Health;
	Character->m_Armor = m_PauseInfo.m_Armor;
	Character->m_PlayerState = m_PauseInfo.m_PlayerState;
	Character->m_LastMove = m_PauseInfo.m_LastMove;
	Character->m_PrevPos = m_PauseInfo.m_PrevPos;
	Character->m_ActiveWeapon = m_PauseInfo.m_ActiveWeapon;
	Character->m_LastWeapon = m_PauseInfo.m_LastWeapon;
	Character->m_HammerType = m_PauseInfo.m_HammerType;
	Character->m_Super = m_PauseInfo.m_Super;
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	Controller->m_Teams.m_Core.Team(GetCID(), m_PauseInfo.m_Team);
	m_PauseInfo.m_Respawn = false;
	m_InfoSaved = false;
}

void CPlayer::SaveCharacter()
{
	m_PauseInfo.m_Core = Character->m_Core;
	m_PauseInfo.m_StartTime = Character->m_StartTime;
	m_PauseInfo.m_DDRaceState = Character->m_DDRaceState;
	for(int i = 0; i < WEAPON_NINJA; ++i)
	{
		m_PauseInfo.m_aHasWeapon[i] = Character->m_aWeapons[i].m_Got;
	}
	m_PauseInfo.m_FreezeTime=Character->m_FreezeTime;
	m_PauseInfo.m_Doored = Character->m_Doored;
	m_PauseInfo.m_OldPos = Character->m_OldPos;
	m_PauseInfo.m_OlderPos = Character->m_OlderPos;
	m_PauseInfo.m_LastAction = Character->m_LastAction;
	m_PauseInfo.m_Jumped = Character->m_Jumped;
	m_PauseInfo.m_Health = Character->m_Health;
	m_PauseInfo.m_Armor = Character->m_Armor;
	m_PauseInfo.m_PlayerState = Character->m_PlayerState;
	m_PauseInfo.m_LastMove = Character->m_LastMove;
	m_PauseInfo.m_PrevPos = Character->m_PrevPos;
	m_PauseInfo.m_ActiveWeapon = Character->m_ActiveWeapon;
	m_PauseInfo.m_LastWeapon = Character->m_LastWeapon;
	m_PauseInfo.m_HammerType = Character->m_HammerType;
	m_PauseInfo.m_Super = Character->m_Super;
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	m_PauseInfo.m_Team = Controller->m_Teams.m_Core.Team(GetCID());
	m_PauseInfo.m_PauseTime = Server()->Tick();
	//m_PauseInfo.m_RefreshTime = Character->m_RefreshTime;
}
