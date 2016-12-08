/* (c) Redix and Sushi */

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>

#include <game/teerace.h>

#include "menus.h"
#include "race_demo.h"

CRaceDemo::CRaceDemo()
{
	m_RaceState = RACE_NONE;
	m_RecordStopTime = 0;
	m_Time = 0;
	m_DemoStartTick = 0;
}

void CRaceDemo::OnRender()
{
	if(!g_Config.m_ClAutoRaceRecord || !m_pClient->m_Snap.m_pGameInfoObj || m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active;
		return;
	}

	// only for race
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo))
		return;

	bool FastCap = IsFastCap(&ServerInfo);
	
	// start the demo
	int EnemyTeam = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Team^1;
	if(((!m_Active && !FastCap && m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active) ||
		(FastCap && m_pClient->m_aFlagPos[EnemyTeam] != vec2(-1, -1) && distance(m_pClient->m_LocalCharacterPos, m_pClient->m_aFlagPos[EnemyTeam]) < 32)) && m_DemoStartTick < Client()->GameTick())
	{
		if(m_RaceState == RACE_STARTED)
			OnReset();
		
		Client()->DemoRecorder_StartRace();
		m_DemoStartTick = Client()->GameTick() + Client()->GameTickSpeed();
		m_RaceState = RACE_STARTED;
	}
	
	// stop the demo
	if(m_RaceState == RACE_FINISHED && m_RecordStopTime < Client()->GameTick() && m_Time > 0)
	{
		CheckDemo();
		OnReset();
	}
	
	m_Active = m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active;
}

void CRaceDemo::OnReset()
{
	if(DemoRecorder()->IsRecording())
		DemoRecorder()->Stop();

	char aDemoName[128];
	char aFilename[512];
	Client()->RaceDemo_GetName(aDemoName, sizeof(aDemoName));
	Client()->RaceDemo_GetPath(aFilename, sizeof(aFilename), aDemoName);

	Storage()->RemoveFile(aFilename, IStorage::TYPE_SAVE);

	m_Time = 0;
	m_RaceState = RACE_NONE;
	m_RecordStopTime = 0;
	m_DemoStartTick = 0;
}

void CRaceDemo::OnShutdown()
{
	OnReset();
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
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalClientID && m_RaceState == RACE_FINISHED)
		{
			// check for new record
			CheckDemo();
			OnReset();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID == -1 && m_RaceState == RACE_STARTED)
		{
			char aName[MAX_NAME_LENGTH];
			int Time = IRace::TimeFromFinishMessage(pMsg->m_pMessage, aName, sizeof(aName));
			if(Time && str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) == 0)
			{
				m_RaceState = RACE_FINISHED;
				m_RecordStopTime = Client()->GameTick() + Client()->GameTickSpeed();
				m_Time = Time;
			}
		}
	}
}

void CRaceDemo::CheckDemo()
{
	// stop the demo recording
	DemoRecorder()->Stop();
	
	if(str_comp(m_pClient->m_pMenus->GetCurrentDemoFolder(), "demos") == 0)
	{
		char aTmpDemoName[128];
		Client()->RaceDemo_GetName(aTmpDemoName, sizeof(aTmpDemoName));

		// loop through demo files
		m_pClient->m_pMenus->DemolistPopulate();
		for (int i = 0; i < m_pClient->m_pMenus->m_lDemos.size(); i++)
		{
			const char *pDemoName = m_pClient->m_pMenus->m_lDemos[i].m_aName;
			if(str_comp(pDemoName, aTmpDemoName) == 0)
				continue;

			int DemoTime = Client()->RaceDemo_ParseName(pDemoName);
			if(DemoTime)
			{
				if (m_Time >= DemoTime)
				{
					// found a better one
					m_Time = 0;
					return;
				}

				// delete old demo
				char aFilename[512];
				Client()->RaceDemo_GetPath(aFilename, sizeof(aFilename), pDemoName);
				Storage()->RemoveFile(aFilename, IStorage::TYPE_SAVE);
			}
		}
	}
	
	// save demo if there is none
	SaveDemo();
	m_Time = 0;
}

void CRaceDemo::SaveDemo()
{
	char aDemoName[128];
	char aNewFilename[512];
	char aOldFilename[512];
	Client()->RaceDemo_GetName(aDemoName, sizeof(aDemoName), m_Time);
	Client()->RaceDemo_GetPath(aNewFilename, sizeof(aNewFilename), aDemoName);
	Client()->RaceDemo_GetName(aDemoName, sizeof(aDemoName));
	Client()->RaceDemo_GetPath(aOldFilename, sizeof(aOldFilename), aDemoName);
	
	Storage()->RenameFile(aOldFilename, aNewFilename, IStorage::TYPE_SAVE);
}
