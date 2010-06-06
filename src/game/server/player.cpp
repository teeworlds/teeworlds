#include <new>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }
	
CPlayer::CPlayer(CGameContext *pGameServer, int CID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	Character = 0;
	this->m_ClientID = CID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
}

CPlayer::~CPlayer()
{
	delete Character;
	Character = 0;
}

void CPlayer::Tick()
{
	Server()->SetClientScore(m_ClientID, m_Score);

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
		str_format(Buf, sizeof(Buf),  "%s has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, Buf);

		dbg_msg("game", "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
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
	
	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos = vec2(100.0f, -60.0f);
	
	if(!GameServer()->m_pController->CanSpawn(this, &SpawnPos))
		return;

	// check if the position is occupado
	CEntity *apEnts[2] = {0};
	int NumEnts = GameServer()->m_World.FindEntities(SpawnPos, 64, apEnts, 2, NETOBJTYPE_CHARACTER);
	
	if(NumEnts == 0)
	{
		m_Spawning = false;
		Character = new(m_ClientID) CCharacter(&GameServer()->m_World);
		Character->Spawn(this, SpawnPos);
		GameServer()->CreatePlayerSpawn(SpawnPos);
	}
}
