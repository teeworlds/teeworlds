/* (c) Redix and Sushi */

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>

#include <game/teerace.h>

#include "menus.h"
#include "race_demo.h"

CRaceDemo::CRaceDemo() : m_RaceState(RACE_NONE), m_RaceStartTick(-1), m_RecordStopTick(-1), m_Time(0) {}

void CRaceDemo::OnRender()
{
	if(!g_Config.m_ClAutoRaceRecord || !m_pClient->m_Snap.m_pGameInfoObj || !m_pClient->m_Snap.m_pLocalCharacter)
		return;

	// only for race
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo) || !m_pClient->m_NewTick)
		return;

	static int s_LastRaceTick = -1;

	bool RaceFlag = m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_RACETIME;
	bool ServerControl = RaceFlag && g_Config.m_ClRaceRecordServerControl;
	int RaceTick = -m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer;

	// start the demo
	bool ForceStart = ServerControl && s_LastRaceTick != RaceTick;
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
				Client()->DemoRecorder_StartRace();
			m_RaceStartTick = Client()->GameTick();
			m_RaceState = RACE_STARTED;
		}
	}

	// start recording before the player passes the start line, so we can see some preparation steps
	if(m_RaceState == RACE_NONE)
	{
		Client()->DemoRecorder_StartRace();
		m_RaceStartTick = Client()->GameTick();
		m_RaceState = RACE_PREPARE;
	}

	// stop recording if the player did not pass the start line after 20 seconds
	if(m_RaceState == RACE_PREPARE && Client()->GameTick() - m_RaceStartTick >= Client()->GameTickSpeed() * 20)
	{
		OnReset();
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

	char aDemoName[128];
	char aTmpFilename[512];
	Client()->RaceDemo_GetName(aDemoName, sizeof(aDemoName));
	Client()->RaceDemo_GetPath(aTmpFilename, sizeof(aTmpFilename), aDemoName);

	if(Time > 0 && CheckDemo(Time))
	{
		// save file
		char aNewFilename[512];
		Client()->RaceDemo_GetName(aDemoName, sizeof(aDemoName), m_Time);
		Client()->RaceDemo_GetPath(aNewFilename, sizeof(aNewFilename), aDemoName);
		
		Storage()->RenameFile(aTmpFilename, aNewFilename, IStorage::TYPE_SAVE);
	}
	else // no new record
		Storage()->RemoveFile(aTmpFilename, IStorage::TYPE_SAVE);

	m_Time = 0;
	m_RaceState = RACE_NONE;
	m_RaceStartTick = -1;
	m_RecordStopTick = -1;
}

bool CRaceDemo::CheckDemo(int Time) const
{
	if(str_comp(m_pClient->m_pMenus->GetCurrentDemoFolder(), "demos") != 0)
		return true;

	char aTmpDemoName[128];
	Client()->RaceDemo_GetName(aTmpDemoName, sizeof(aTmpDemoName));

	// loop through demo files
	m_pClient->m_pMenus->DemolistPopulate();
	for(int i = 0; i < m_pClient->m_pMenus->m_lDemos.size(); i++)
	{
		// skip temp file
		const char *pDemoName = m_pClient->m_pMenus->m_lDemos[i].m_aName;
		if(str_comp(pDemoName, aTmpDemoName) == 0)
			continue;

		const char *pTime = Client()->RaceDemo_ParseName(pDemoName);
		int DemoTime = 0;
		if(pTime)
		{
			const char *pTmp = pTime;
			while(*pTmp >= '0' && *pTmp <= '9')
				pTmp++;

			if(*pTmp == '.' || *pTmp == ',')
				DemoTime = IRace::TimeFromSecondsStr(pTime);
			else
				DemoTime = str_toint(pTime);
		}

		if(DemoTime > 0)
		{
			if(Time >= DemoTime) // found a better demo
				return false;

			// delete old demo
			char aFilename[512];
			str_format(aFilename, sizeof(aFilename), "demos/%s", m_pClient->m_pMenus->m_lDemos[i].m_aFilename);
			Storage()->RemoveFile(aFilename, IStorage::TYPE_SAVE);
		}
	}
	
	return true;
}
