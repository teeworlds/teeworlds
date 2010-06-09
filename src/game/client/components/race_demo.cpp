#include <stdio.h>

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>

#include <game/client/components/menus.h>

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
	if(!g_Config.m_ClAutoRecord || !m_pClient->m_Snap.m_pGameobj || m_pClient->m_Snap.m_Spectate)
	{
		m_Active = m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active;
		return;
	}

	// only for race
	if(!m_pClient->m_IsRace)
		return;
	
	vec2 PlayerPos = m_pClient->m_LocalCharacterPos;
	
	// start the demo
	if(!m_Active && m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active && m_DemoStartTick < Client()->GameTick())
	{
		if(m_RaceState == RACE_STARTED)
			OnReset();
		
		m_pMap = Client()->RaceRecordStart("tmp");
		m_DemoStartTick = Client()->GameTick() + Client()->GameTickSpeed();
		m_RaceState = RACE_STARTED;
	}
	
	// stop the demo
	if(m_RaceState == RACE_FINISHED && m_RecordStopTime < Client()->GameTick() && m_Time > 0)
	{
		CheckDemo();
		OnReset();
	}
	
	m_Active = m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active;
}

void CRaceDemo::OnReset()
{
	if(m_RaceState > RACE_NONE)
		Client()->RaceRecordStop();
	
	// remove tmp demo
	if(m_RaceState == RACE_STARTED)
	{
		char aBuffer[512];
		char aFilename[512];
		str_format(aBuffer, sizeof(aBuffer), "demos/%s_tmp.demo", m_pMap);
		Storage()->SavePath(aBuffer, aFilename, sizeof(aFilename));
		remove(aFilename);
	}
	
	m_Time = 0;
	m_RaceState = RACE_NONE;
	m_RecordStopTime = 0;
	m_DemoStartTick = 0;
}

void CRaceDemo::OnMessage(int MsgType, void *pRawMsg)
{
	if(!g_Config.m_ClAutoRecord || m_pClient->m_Snap.m_Spectate)
		return;
	
	// only for race
	if(!m_pClient->m_IsRace)
		return;
		
	// check for messages from server
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalCid && m_RaceState == RACE_FINISHED)
		{
			// check for new record
			CheckDemo();
			OnReset();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_Cid == -1 && m_RaceState == RACE_STARTED)
		{
			const char* pMessage = pMsg->m_pMessage;
			
			int Num = 0;
			while(str_comp_num(pMessage, " finished in: ", 14))
			{
				pMessage++;
				Num++;
				if(!pMessage[0])
					return;
			}
			
			// store the name
			char aName[64];
			str_copy(aName, pMsg->m_pMessage, Num+1);
			
			// prepare values and state for saving
			int Minutes;
			float Seconds;
			if(!str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalCid].m_aName) && sscanf(pMessage, " finished in: %d minute(s) %f", &Minutes, &Seconds) == 2)
			{
				m_RaceState = RACE_FINISHED;
				m_RecordStopTime = Client()->GameTick() + Client()->GameTickSpeed();
				m_Time = Minutes*60 + Seconds;
			}
		}
	}
}

void CRaceDemo::CheckDemo()
{
	// stop the demo recording
	Client()->RaceRecordStop();
	
	char aDemoName[128];
	str_format(aDemoName, sizeof(aDemoName), "%s_best_", m_pMap);
	
	// loop through demo files
	m_pClient->m_pMenus->DemolistPopulate();
	for(int i = 0; i < m_pClient->m_pMenus->m_lDemos.size(); i++)
	{
		if(!str_comp_num(m_pClient->m_pMenus->m_lDemos[i].m_aName, aDemoName, str_length(aDemoName)))
		{
			const char *pDemo = m_pClient->m_pMenus->m_lDemos[i].m_aName;
			
			// set cursor
			while(str_comp_num(pDemo, "best_", 5))
			{
				pDemo++;
				if(!pDemo[0])
					return;
			}

			float DemoTime;
			sscanf(pDemo, "best_%f", &DemoTime);
			if(m_Time < DemoTime)
			{
				// save new record
				SaveDemo(aDemoName);
				
				// delete old demo
				remove(m_pClient->m_pMenus->m_lDemos[i].m_aFilename);
			}
	
			m_Time = 0;
			
			return;
		}
	}
	
	// save demo if there is none
	SaveDemo(aDemoName);
	
	m_Time = 0;
}

void CRaceDemo::SaveDemo(const char* pDemo)
{
	char aFilename[512];
	char aOldFilename[512];
	char aBuffer[512];
	str_format(aBuffer, sizeof(aBuffer), "demos/%s%5.2f.demo", pDemo, m_Time);
	Storage()->SavePath(aBuffer, aFilename, sizeof(aFilename));
	str_format(aBuffer, sizeof(aBuffer), "demos/%s_tmp.demo", m_pMap);
	Storage()->SavePath(aBuffer, aOldFilename, sizeof(aOldFilename));
	
	rename(aOldFilename, aFilename);
}
