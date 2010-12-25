/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdio.h>

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "timemessages.h"

void CTimeMessages::OnReset()
{
	m_TimemsgCurrent = 0;
	for(int i = 0; i < MAX_TIMEMSGS; i++)
		m_aTimemsgs[i].m_Tick = -100000;
}

void CTimeMessages::OnMessage(int MsgType, void *pRawMsg)
{
	if(!m_pClient->m_IsRace)
		return;
	
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_Cid == -1 && str_find(pMsg->m_pMessage, " finished in: "))
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
			
			CTimeMsg Time;
			Time.m_Diff = 0;
			
			// store the name
			char aName[MAX_NAME_LENGTH];
			str_copy(Time.m_aPlayerName, pMsg->m_pMessage, Num+1);
			
			// prepare values and state for saving
			if(sscanf(pMessage, " finished in: %d minute(s) %f", &Time.m_Minutes, &Time.m_Seconds) == 2)
			{
				int PlayerID = -1;
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(!str_comp(Time.m_aPlayerName, m_pClient->m_aClients[i].m_aName))
					{
						PlayerID = i;
						break;
					}
				
				// some proof
				if(PlayerID < 0)
					return;
				
				Time.m_PlayerRenderInfo = m_pClient->m_aClients[PlayerID].m_RenderInfo;
				Time.m_Tick = Client()->GameTick();
				
				m_TimemsgCurrent = (m_TimemsgCurrent+1)%MAX_TIMEMSGS;
				m_aTimemsgs[m_TimemsgCurrent] = Time;
			}
		}
		if(pMsg->m_Cid == -1 && !str_comp_num(pMsg->m_pMessage, "New record: ", 12))
		{
			const char* pMessage = pMsg->m_pMessage;
			
			if(sscanf(pMessage, "New record: %f", &m_aTimemsgs[m_TimemsgCurrent].m_Diff) != 1)
				m_aTimemsgs[m_TimemsgCurrent].m_Diff = 0;
		}
	}
}

void CTimeMessages::OnRender()
{
	if(!m_pClient->m_IsRace)
		return;
	
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;

	for(int i = 1; i <= MAX_TIMEMSGS; i++)
	{
		int r = (m_TimemsgCurrent+i)%MAX_TIMEMSGS;
		if(Client()->GameTick() > m_aTimemsgs[r].m_Tick+50*10)
			continue;

		float FontSize = 42.0f;
		float PlayerNameW = TextRender()->TextWidth(0, FontSize, m_aTimemsgs[r].m_aPlayerName, -1);
		
		// time
		char aTime[32];
		str_format(aTime, sizeof(aTime), "- Time: %d:%.2f", m_aTimemsgs[r].m_Minutes, m_aTimemsgs[r].m_Seconds);
		float TimeW = TextRender()->TextWidth(0, FontSize, aTime, -1);
		
		float x = StartX;
		
		// render diff
		if(m_aTimemsgs[r].m_Diff)
		{
			char aDiff[32];
			str_format(aDiff, sizeof(aDiff), "%+.2f", m_aTimemsgs[r].m_Diff);
			float DiffW = TextRender()->TextWidth(0, FontSize, aDiff, -1);
			
			x -= DiffW;
			TextRender()->TextColor(0.5f, 1.0f, 0.5f, 1.0f);
			TextRender()->Text(0, x, y, FontSize, aDiff, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			x -= 16.0f;
		}
			
		
		// render time
		x -= TimeW;
		TextRender()->Text(0, x, y, FontSize, aTime, -1);
		
		x -= 16.0f;
		
		// render player name
		x -= PlayerNameW;
		TextRender()->Text(0, x, y, FontSize, m_aTimemsgs[r].m_aPlayerName, -1);

		x -= 24.0f;
		
		// render player tee
		RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aTimemsgs[r].m_PlayerRenderInfo, EMOTE_HAPPY, vec2(-1,0), vec2(x, y+28));
		
		// new line
		y += 48;
	}
}
