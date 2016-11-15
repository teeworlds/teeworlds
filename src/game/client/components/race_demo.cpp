/* (c) Redix and Sushi */

#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/teerace.h>

#include "menus.h"
#include "race_demo.h"

// TODO: rework the path handling
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
	if(!m_pClient->m_IsRace)
		return;
	
	vec2 PlayerPos = m_pClient->m_LocalCharacterPos;
	
	// start the demo
	int EnemyTeam = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Team^1;
	if(((!m_Active && !m_pClient->m_IsFastCap && m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active) ||
		(m_pClient->m_IsFastCap && m_pClient->m_aFlagPos[EnemyTeam] != vec2(-1, -1) && distance(PlayerPos, m_pClient->m_aFlagPos[EnemyTeam]) < 200)) && m_DemoStartTick < Client()->GameTick())
	{
		if(m_RaceState == RACE_STARTED)
			OnReset();
		
		m_pMap = Client()->DemoRecorder_StartRace("tmp");
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
		
	char aFilename[512];
	str_format(aFilename, sizeof(aFilename), "demos/%s_tmp.demo", m_pMap);
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
	if(!m_pClient->m_IsRace)
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
	
	char aTmpDemoName[128];
	str_format(aTmpDemoName, sizeof(aTmpDemoName), "%s_tmp", m_pMap);
	
	// loop through demo files
	m_pClient->m_pMenus->DemolistPopulate();
	for(int i = 0; i < m_pClient->m_pMenus->m_lDemos.size(); i++)
	{
		if(!str_comp_num(m_pClient->m_pMenus->m_lDemos[i].m_aName, m_pMap, str_length(m_pMap)) && str_comp_num(m_pClient->m_pMenus->m_lDemos[i].m_aName, aTmpDemoName, str_length(aTmpDemoName)))
 		{
			char aDemo[128];
			str_copy(aDemo, m_pClient->m_pMenus->m_lDemos[i].m_aName, sizeof(aDemo));
			
			char *pDemo = aDemo + str_length(m_pMap)+1;
			const char *pTimeEnd = str_find(pDemo, "_");
			if(pTimeEnd)
			{
				int TimeLen = pTimeEnd - pDemo;
				pDemo[TimeLen] = 0;
			}
			
			int DemoTime;
			if(str_find(pDemo, ".")) // detect old demos
				DemoTime = str_tofloat(pDemo) * 1000;
			else
				DemoTime = str_toint(pDemo);

			if(m_Time < DemoTime)
			{
				// save new record
				SaveDemo(m_pMap);
				
				// delete old demo
				char aFilename[512];
				str_format(aFilename, sizeof(aFilename), "demos/%s.demo", m_pClient->m_pMenus->m_lDemos[i].m_aName);
				Storage()->RemoveFile(aFilename, IStorage::TYPE_SAVE);
			}
	
			m_Time = 0;
			
			return;
		}
	}
	
	// save demo if there is none
	SaveDemo(m_pMap);
	
	m_Time = 0;
}

void CRaceDemo::SaveDemo(const char* pDemo)
{
	char aNewFilename[512];
	char aOldFilename[512];
	if(g_Config.m_ClDemoName)
	{
		char aPlayerName[MAX_NAME_LENGTH];
		str_copy(aPlayerName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName, sizeof(aPlayerName));
		ClearFilename(aPlayerName, MAX_NAME_LENGTH);
		str_format(aNewFilename, sizeof(aNewFilename), "demos/%s_%d_%s.demo", pDemo, m_Time, aPlayerName);
	}
	else
		str_format(aNewFilename, sizeof(aNewFilename), "demos/%s_%d.demo", pDemo, m_Time);

	str_format(aOldFilename, sizeof(aOldFilename), "demos/%s_tmp.demo", m_pMap);
	
	Storage()->RenameFile(aOldFilename, aNewFilename, IStorage::TYPE_SAVE);
}

// TODO: remove this
void CRaceDemo::ClearFilename(char *pFilename, int Size)
{
	for (int i = 0; i < Size; i++)
	{
		if (!pFilename[i])
			break;

		if (pFilename[i] == '\\' || pFilename[i] == '/' || pFilename[i] == '|' || pFilename[i] == ':' || pFilename[i] == '*' || pFilename[i] == '?' || pFilename[i] == '<' || pFilename[i] == '>' || pFilename[i] == '"')
			pFilename[i] = '%';
	}
}
