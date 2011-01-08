/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdio.h>

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
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
			Time.m_ServerDiff = 0;
			Time.m_LocalDiff = 0;
			
			// store the name
			str_copy(Time.m_aPlayerName, pMsg->m_pMessage, Num+1);
			
			// prepare values and state for saving
			if(sscanf(pMessage, " finished in: %d minute(s) %f", &Time.m_Minutes, &Time.m_Seconds) == 2)
			{
				Time.m_PlayerID = -1;
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(!str_comp(Time.m_aPlayerName, m_pClient->m_aClients[i].m_aName))
					{
						Time.m_PlayerID = i;
						break;
					}
				
				// some proof
				if(Time.m_PlayerID < 0)
					return;
				
				float FinishTime = (float)(Time.m_Minutes*60) + Time.m_Seconds;
				if(m_pClient->m_aClients[Time.m_PlayerID].m_Score > 0)
					Time.m_LocalDiff = FinishTime - m_pClient->m_aClients[Time.m_PlayerID].m_Score;
				Time.m_PlayerRenderInfo = m_pClient->m_aClients[Time.m_PlayerID].m_RenderInfo;
				Time.m_Tick = time_get();
				
				m_TimemsgCurrent = (m_TimemsgCurrent+1)%MAX_TIMEMSGS;
				m_aTimemsgs[m_TimemsgCurrent] = Time;
			}
		}
		if(pMsg->m_Cid == -1 && !str_comp_num(pMsg->m_pMessage, "New record: ", 12))
		{
			const char* pMessage = pMsg->m_pMessage;
			
			if(sscanf(pMessage, "New record: %f", &m_aTimemsgs[m_TimemsgCurrent].m_ServerDiff) != 1)
				m_aTimemsgs[m_TimemsgCurrent].m_ServerDiff = 0;
		}
	}
}

void CTimeMessages::OnRender()
{
	if(!m_pClient->m_IsRace)
		return;
	
	float Width = 330*3.0f*Graphics()->ScreenAspect();
	float Height = 330*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;
	if(g_Config.m_ClShowfps)
		y = 60.0f;

	int64 Now = time_get();
	for(int i = 1; i <= MAX_TIMEMSGS; i++)
	{
		int r = (m_TimemsgCurrent+i)%MAX_TIMEMSGS;
		if(Now > m_aTimemsgs[r].m_Tick+10*time_freq())
			continue;

		float Blend = Now > m_aTimemsgs[r].m_Tick+8*time_freq() ? 1.0f-(Now-m_aTimemsgs[r].m_Tick-8*time_freq())/(2.0f*time_freq()) : 1.0f;
		
		float FontSize = 36.0f;
		float PlayerNameW = TextRender()->TextWidth(0, FontSize, m_aTimemsgs[r].m_aPlayerName, -1);
		
		// time
		char aTime[32];
		str_format(aTime, sizeof(aTime), "%02d:%06.3f", m_aTimemsgs[r].m_Minutes, m_aTimemsgs[r].m_Seconds);
		float TimeW = TextRender()->TextWidth(0, FontSize, aTime, -1);
		
		float x = StartX;
		
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);
		
		// render local diff
		if(m_aTimemsgs[r].m_LocalDiff && !m_aTimemsgs[r].m_ServerDiff)
		{
			char aDiff[32];
			str_format(aDiff, sizeof(aDiff), "(%+5.3f)", m_aTimemsgs[r].m_LocalDiff);
			float DiffW = TextRender()->TextWidth(0, FontSize, aDiff, -1);
			
			x -= DiffW;
			if(m_aTimemsgs[r].m_LocalDiff < 0)
				TextRender()->TextColor(0.5f, 1.0f, 0.5f, Blend);
			else
				TextRender()->TextColor(0.7f, 0.7f, 0.7f, Blend);
			TextRender()->Text(0, x, y, FontSize, aDiff, -1);
			if(m_aTimemsgs[r].m_PlayerID != m_pClient->m_Snap.m_LocalCid)
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);
			
			x -= 16.0f;
		}
		
		// render server diff
		if(m_aTimemsgs[r].m_ServerDiff)
		{
			char aDiff[32];
			str_format(aDiff, sizeof(aDiff), "(%+5.3f)", m_aTimemsgs[r].m_ServerDiff);
			float DiffW = TextRender()->TextWidth(0, FontSize, aDiff, -1);
			
			x -= DiffW;
			TextRender()->TextColor(0.0f, 0.5f, 1.0f, Blend);
			TextRender()->Text(0, x, y, FontSize, aDiff, -1);
			if(m_aTimemsgs[r].m_PlayerID != m_pClient->m_Snap.m_LocalCid)
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);
			
			x -= 16.0f;
		}
			
		
		// render time
		x -= TimeW;
		TextRender()->Text(0, x, y, FontSize, aTime, -1);
		
		x -= 52.0f;
		
		// render flag
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_RACEFLAG].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, Blend);
		IGraphics::CQuadItem QuadItem(x, y, 52, 52);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		
		x -= 10.0f;
		
		// render player name
		x -= PlayerNameW;
		TextRender()->Text(0, x, y, FontSize, m_aTimemsgs[r].m_aPlayerName, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f); // reset color
		
		x -= 28.0f;
		
		// render player tee
		m_aTimemsgs[r].m_PlayerRenderInfo.m_ColorBody.a = Blend;
		m_aTimemsgs[r].m_PlayerRenderInfo.m_ColorFeet.a = Blend;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aTimemsgs[r].m_PlayerRenderInfo, EMOTE_HAPPY, vec2(-1,0), vec2(x, y+28), true);
		
		// new line
		y += 46;
	}
}
