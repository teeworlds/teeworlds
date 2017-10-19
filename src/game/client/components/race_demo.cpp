/* (c) Redix and Sushi */

#include <ctype.h>

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <engine/demo.h>
#include <engine/storage.h>

#include <game/teerace.h>

#include "race_demo.h"

const char *CRaceDemo::ms_pRaceDemoDir = "demos/auto/race";

struct CDemoItem
{
	char m_aName[128];
	int m_Time;
};

struct CDemoListParam
{
	array<CDemoItem> *m_plDemos;
	const char *pMap;
};

CRaceDemo::CRaceDemo() : m_RaceState(RACE_NONE), m_RaceStartTick(-1), m_RecordStopTick(-1), m_Time(0) {}

void CRaceDemo::GetPath(char *pBuf, int Size, int Time) const
{
	const char *pMap = Client()->GetCurrentMap();

	char aPlayerName[MAX_NAME_LENGTH];
	str_copy(aPlayerName, g_Config.m_PlayerName, sizeof(aPlayerName));
	str_sanitize_filename(aPlayerName);

	if(Time < 0)
		str_format(pBuf, Size, "%s/%s_%s_tmp.demo", ms_pRaceDemoDir, pMap, aPlayerName);
	else
		str_format(pBuf, Size, "%s/%s_%d.%03d_%s.demo", ms_pRaceDemoDir, pMap, Time / 1000, Time % 1000, aPlayerName);
}

void CRaceDemo::OnNewSnapshot()
{
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo) || !g_Config.m_ClAutoRaceRecord || Client()->State() != IClient::STATE_ONLINE)
		return;

	if(!m_pClient->m_Snap.m_pGameInfoObj || m_pClient->m_Snap.m_SpecInfo.m_Active || !m_pClient->m_Snap.m_pLocalCharacter || !m_pClient->m_Snap.m_pLocalPrevCharacter)
		return;

	static int s_LastRaceTick = -1;

	bool RaceFlag = m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_RACETIME;
	bool ServerControl = RaceFlag && g_Config.m_ClRaceRecordServerControl;
	int RaceTick = -m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer;

	// start the demo
	bool ForceStart = ServerControl && s_LastRaceTick != RaceTick && Client()->GameTick() - RaceTick < Client()->GameTickSpeed();
	bool AllowRestart = ForceStart && m_RaceStartTick + 10 * Client()->GameTickSpeed() < Client()->GameTick();
	if(m_RaceState == RACE_IDLE || m_RaceState == RACE_PREPARE || (m_RaceState == RACE_STARTED && AllowRestart))
	{
		vec2 PrevPos = vec2(m_pClient->m_Snap.m_pLocalPrevCharacter->m_X, m_pClient->m_Snap.m_pLocalPrevCharacter->m_Y);
		vec2 Pos = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_X, m_pClient->m_Snap.m_pLocalCharacter->m_Y);

		if(ForceStart || (!ServerControl && m_pClient->IsRaceStart(PrevPos, Pos)))
		{
			if(m_RaceState == RACE_STARTED)
				Client()->DemoRecorder_Stop();
			if(m_RaceState != RACE_PREPARE) // start recording again
			{
				GetPath(m_aTmpFilename, sizeof(m_aTmpFilename));
				Client()->DemoRecorder_StartRace(m_aTmpFilename);
			}
			m_RaceStartTick = Client()->GameTick();
			m_RaceState = RACE_STARTED;
		}
	}

	// start recording before the player passes the start line, so we can see some preparation steps
	if(m_RaceState == RACE_NONE)
	{
		GetPath(m_aTmpFilename, sizeof(m_aTmpFilename));
		Client()->DemoRecorder_StartRace(m_aTmpFilename);
		m_RaceStartTick = Client()->GameTick();
		m_RaceState = RACE_PREPARE;
	}

	// stop recording if the player did not pass the start line after 20 seconds
	if(m_RaceState == RACE_PREPARE && Client()->GameTick() - m_RaceStartTick >= Client()->GameTickSpeed() * 20)
	{
		StopRecord();
		m_RaceState = RACE_IDLE;
	}

	// stop the demo
	if(m_RaceState == RACE_FINISHED && m_RecordStopTick <= Client()->GameTick())
		StopRecord(m_Time);

	s_LastRaceTick = RaceFlag ? RaceTick : -1;
}

void CRaceDemo::OnReset()
{
	StopRecord();
}

void CRaceDemo::OnMessage(int MsgType, void *pRawMsg)
{
	if(!g_Config.m_ClAutoRaceRecord || m_pClient->m_Snap.m_SpecInfo.m_Active)
		return;
	
	// only for race
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo))
		return;
		
	// check for messages from server
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalClientID)
			StopRecord(m_Time);
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID == -1 && m_RaceState == RACE_STARTED)
		{
			char aName[MAX_NAME_LENGTH];
			int Time = IRace::TimeFromFinishMessage(pMsg->m_pMessage, aName, sizeof(aName));
			if(Time > 0 && str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) == 0)
			{
				m_RaceState = RACE_FINISHED;
				m_RecordStopTick = Client()->GameTick() + Client()->GameTickSpeed();
				m_Time = Time;
			}
		}
	}
}

void CRaceDemo::StopRecord(int Time)
{
	if(DemoRecorder()->IsRecording())
		Client()->DemoRecorder_Stop();

	if(Time > 0 && CheckDemo(Time))
	{
		// save file
		char aNewFilename[512];
		GetPath(aNewFilename, sizeof(aNewFilename), m_Time);

		Storage()->RenameFile(m_aTmpFilename, aNewFilename, IStorage::TYPE_SAVE);
	}
	else // no new record
		Storage()->RemoveFile(m_aTmpFilename, IStorage::TYPE_SAVE);

	m_aTmpFilename[0] = 0;

	m_Time = 0;
	m_RaceState = RACE_NONE;
	m_RaceStartTick = -1;
	m_RecordStopTick = -1;
}

int CRaceDemo::RaceDemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	CDemoListParam *pParam = (CDemoListParam*) pUser;
	int Length = str_length(pName);
	int MapLen = str_length(pParam->pMap);
	if(IsDir || Length < 5 || str_comp(pName + Length - 5, ".demo") != 0 || str_comp_num(pName, pParam->pMap, MapLen) != 0 || pName[MapLen] != '_')
		return 0;

	CDemoItem Item;
	str_copy(Item.m_aName, pName, min(static_cast<int>(sizeof(Item.m_aName)), Length - 4));

	const char *pTime = Item.m_aName + MapLen + 1;
	const char *pTEnd = pTime;
	while(isdigit(*pTEnd) || *pTEnd == ' ' || *pTEnd == '.' || *pTEnd == ',')
		pTEnd++;

	char aPlayerName[MAX_NAME_LENGTH];
	str_copy(aPlayerName, g_Config.m_PlayerName, sizeof(aPlayerName));
	str_sanitize_filename(aPlayerName);

	if(pTEnd[0] != '_' || str_comp(pTEnd + 1, aPlayerName) != 0)
		return 0;
	
	const char *pTmp = pTime;
	while(isdigit(*pTmp) || *pTmp == ' ')
		pTmp++;

	if(*pTmp == '.' || *pTmp == ',')
		Item.m_Time = IRace::TimeFromSecondsStr(pTime);
	else
		Item.m_Time = str_toint(pTime);

	if(Item.m_Time > 0)
		pParam->m_plDemos->add(Item);

	return 0;
}

bool CRaceDemo::CheckDemo(int Time) const
{
	array<CDemoItem> lDemos;
	CDemoListParam Param = { &lDemos, Client()->GetCurrentMap() };
	Storage()->ListDirectory(IStorage::TYPE_SAVE, ms_pRaceDemoDir, RaceDemolistFetchCallback, &Param);

	// loop through demo files
	for(int i = 0; i < lDemos.size(); i++)
	{
		if(Time >= lDemos[i].m_Time) // found a better demo
			return false;

		// delete old demo
		char aFilename[512];
		str_format(aFilename, sizeof(aFilename), "%s/%s.demo", ms_pRaceDemoDir, lDemos[i].m_aName);
		Storage()->RemoveFile(aFilename, IStorage::TYPE_SAVE);
	}

	return true;
}
