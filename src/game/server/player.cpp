#include <new>
#include <stdio.h>
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
	m_CharacterCopy = 0;
	m_Muted = 0;
	this->m_ClientID = CID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	
	m_LastPlaytime = time_get();
	m_LastTarget_x = 0;
	m_LastTarget_y = 0;
	m_SentAfkWarning = 0; // afk timer's 1st warning after 50% of sv_max_afk_time
	m_SentAfkWarning2 = 0;
	
	m_PauseInfo.m_Respawn = false;
}

CPlayer::~CPlayer()
{
	delete Character;
	Character = 0;
}

void CPlayer::Tick()
{
	int pos=0;
	CPlayerScore *pscore = ((CGameControllerDDRace*)GameServer()->m_pController)->m_Score.SearchName(Server()->ClientName(m_ClientID), pos);
	if(pscore && pos > -1 && pscore->m_Score != -1)
	{
		float time = pscore->m_Score;
		//if (!config.sv_hide_score)
			m_Score = time * 100;
		//else
		//	score=authed;
	} else
		m_Score = 0.0f;
	Server()->SetClientAuthed(m_ClientID, m_Authed);
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
			delete Character;
			Character = 0;
		}
	}
	else if(m_Spawning && m_RespawnTick <= Server()->Tick())
		TryRespawn();
}

void CPlayer::Snap(int SnappingClient)
{
	CNetObj_ClientInfo *ClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));
	StrToInts(&ClientInfo->m_Name0, 6, Server()->ClientName(m_ClientID));
	StrToInts(&ClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	ClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	ClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	ClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *Info = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo)));

	Info->m_Latency = m_Latency.m_Min;
	Info->m_LatencyFlux = m_Latency.m_Max-m_Latency.m_Min;
	Info->m_Local = 0;
	Info->m_ClientId = m_ClientID;
	Info->m_Score = m_Score;
	Info->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		Info->m_Local = 1;	
}

void CPlayer::OnDisconnect()
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char Buf[512];
		const char * Name = Server()->ClientName(m_ClientID);
		str_format(Buf, sizeof(Buf),  "%s has left the game", Name);
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, Buf);
		char Cmd[64];
		dbg_msg("game", "leave player='%d:%s'", m_ClientID, Name);
		if(m_Muted > 0) {
			str_format(Cmd, sizeof(Cmd), "ban %d %d '%s'", m_ClientID, m_Muted/Server()->TickSpeed(), "ppc");
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
	AfkTimer(NewInput->m_TargetX, NewInput->m_TargetY);
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
		delete Character;
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
		
	char Buf[512];
	str_format(Buf, sizeof(Buf), "%s joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, Buf); 
	
	KillCharacter();

	m_Team = Team;
	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	dbg_msg("game", "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	
	//GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);
}

void CPlayer::TryRespawn()
{
	if(m_PauseInfo.m_Respawn) {
		Character = new(m_ClientID) CCharacter(&GameServer()->m_World);
		Character->Spawn(this, m_PauseInfo.m_Core.m_Pos);
		GameServer()->CreatePlayerSpawn(m_PauseInfo.m_Core.m_Pos);
		LoadCharacter();
	} else {
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

void CPlayer::LoadCharacter() {
	Character->m_Core = m_PauseInfo.m_Core;
	Character->m_StartTime = m_PauseInfo.m_StartTime;
	Character->m_RaceState = m_PauseInfo.m_RaceState;
	Character->m_RefreshTime = Server()->Tick();
	for(int i = 0; i < NUM_WEAPONS; ++i) {
		if(m_PauseInfo.m_aHasWeapon[i])
		 if(!m_PauseInfo.m_FreezeTime)
			Character->GiveWeapon(i, -1);
		 else
			 Character->GiveWeapon(i, 0);
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
	Character->m_LastSpeedup = m_PauseInfo.m_LastSpeedup;
	Character->m_PrevPos = m_PauseInfo.m_PrevPos;
	Character->m_ActiveWeapon = m_PauseInfo.m_ActiveWeapon;
	Character->m_LastWeapon = m_PauseInfo.m_LastWeapon;
	Character->m_HammerType = m_PauseInfo.m_HammerType;
	Character->m_Super = m_PauseInfo.m_Super;
	m_PauseInfo.m_Respawn = false;
}

void CPlayer::SaveCharacter()
{
	m_PauseInfo.m_Core = Character->m_Core;
	m_PauseInfo.m_StartTime = Character->m_StartTime;
	m_PauseInfo.m_RaceState = Character->m_RaceState;
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
	m_PauseInfo.m_LastSpeedup = Character->m_LastSpeedup;
	m_PauseInfo.m_PrevPos = Character->m_PrevPos;
	m_PauseInfo.m_ActiveWeapon = Character->m_ActiveWeapon;
	m_PauseInfo.m_LastWeapon = Character->m_LastWeapon;
	m_PauseInfo.m_HammerType = Character->m_HammerType;
	m_PauseInfo.m_Super = Character->m_Super;
	//m_PauseInfo.m_RefreshTime = Character->m_RefreshTime;
}

void CPlayer::AfkTimer(int new_target_x, int new_target_y)
{
	/*
		afk timer (x, y = mouse coordinates)
		Since a player has to move the mouse to play, this is a better method than checking
		the player's position in the game world, because it can easily be bypassed by just locking a key.
		Frozen players could be kicked as well, because they can't move.
		It also works for spectators.
	*/
	
	if(m_Authed) return; // don't kick admins
	if(g_Config.m_SvMaxAfkTime == 0) return; // 0 = disabled
	
	if(new_target_x != m_LastTarget_x || new_target_y != m_LastTarget_y)
	{
		m_LastPlaytime = time_get();
		m_LastTarget_x = new_target_x;
		m_LastTarget_y = new_target_y;
		m_SentAfkWarning = 0; // afk timer's 1st warning after 50% of sv_max_afk_time
		m_SentAfkWarning2 = 0;
		
	}
	else
	{
		// not playing, check how long
		if(m_SentAfkWarning == 0 && m_LastPlaytime < time_get()-time_freq()*(int)(g_Config.m_SvMaxAfkTime*0.5))
		{
			sprintf(
				m_pAfkMsg,
				"You have been afk for %d seconds now. Please note that you get kicked after not playing for %d seconds.",
				(int)(g_Config.m_SvMaxAfkTime*0.5),
				g_Config.m_SvMaxAfkTime
			);
			m_pGameServer->SendChatTarget(m_ClientID, m_pAfkMsg);
			m_SentAfkWarning = 1;
		} else if(m_SentAfkWarning2 == 0 && m_LastPlaytime < time_get()-time_freq()*(int)(g_Config.m_SvMaxAfkTime*0.9))
		{
			sprintf(
				m_pAfkMsg,
				"You have been afk for %d seconds now. Please note that you get kicked after not playing for %d seconds.",
				(int)(g_Config.m_SvMaxAfkTime*0.9),
				g_Config.m_SvMaxAfkTime
			);
			m_pGameServer->SendChatTarget(m_ClientID, m_pAfkMsg);
			m_SentAfkWarning = 1;
		} else if(m_LastPlaytime < time_get()-time_freq()*g_Config.m_SvMaxAfkTime)
		{
			CServer* serv =	(CServer*)m_pGameServer->Server();
			serv->Kick(m_ClientID,"Away from keyboard");
		}
	}
}
