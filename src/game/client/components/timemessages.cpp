/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/teerace.h>
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
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if (!IsRace(&ServerInfo))
		return;
	
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID == -1)
		{
			CTimeMsg Time;
			Time.m_ServerDiff = 0;
			Time.m_LocalDiff = 0;

			int MsgTime = IRace::TimeFromFinishMessage(pMsg->m_pMessage, Time.m_aPlayerName, sizeof(Time.m_aPlayerName));
			if(MsgTime)
			{
				Time.m_Time = MsgTime;
				Time.m_PlayerID = -1;
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(str_comp(Time.m_aPlayerName, m_pClient->m_aClients[i].m_aName) == 0)
					{
						Time.m_PlayerID = i;
						break;
					}
				
				// some proof
				if(Time.m_PlayerID < 0)
					return;
				
				if(m_pClient->m_aClients[Time.m_PlayerID].m_RaceTime > 0)
					Time.m_LocalDiff = Time.m_Time - m_pClient->m_aClients[Time.m_PlayerID].m_RaceTime;
				Time.m_PlayerRenderInfo = m_pClient->m_aClients[Time.m_PlayerID].m_RenderInfo;
				Time.m_Tick = time_get();
				
				m_TimemsgCurrent = (m_TimemsgCurrent+1)%MAX_TIMEMSGS;
				m_aTimemsgs[m_TimemsgCurrent] = Time;

				char aTime[32];
				IRace::FormatTimeShort(aTime, sizeof(aTime), Time.m_Time);

				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "%s finished in: %s", Time.m_aPlayerName, aTime);
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "race", aBuf);
			}
			else
			{
				static const char *pRecordStr = "New record: ";
				const char *pRecord = str_find(pMsg->m_pMessage, pRecordStr);
				if (pRecord)
				{
					pRecord += str_length(pRecordStr);
					if (*pRecord == '-')
						pRecord++;
					int MsgTime = IRace::TimeFromStr(pRecord);
					if (MsgTime)
						m_aTimemsgs[m_TimemsgCurrent].m_ServerDiff = -MsgTime;
				}
			}
		}
	}
}

void CTimeMessages::OnRender()
{
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo))
		return;
	
	float Width = 330*3.0f*Graphics()->ScreenAspect();
	float Height = 330*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;
	if(g_Config.m_ClShowfps)
		y = 60.0f;
	
	TextRender()->BatchBegin();

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
		IRace::FormatTimeShort(aTime, sizeof(aTime), m_aTimemsgs[r].m_Time, true);
		float TimeW = TextRender()->TextWidth(0, FontSize, aTime, -1);
		
		float x = StartX;
		
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);
		
		// render local diff
		if(m_aTimemsgs[r].m_LocalDiff && !m_aTimemsgs[r].m_ServerDiff)
		{
			char aBuf[32];
			char aDiff[32];
			IRace::FormatTimeDiff(aDiff, sizeof(aDiff), m_aTimemsgs[r].m_LocalDiff);
			str_format(aBuf, sizeof(aBuf), "(%s)", aDiff);
			float DiffW = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			
			x -= DiffW;
			if(m_aTimemsgs[r].m_LocalDiff < 0)
				TextRender()->TextColor(0.5f, 1.0f, 0.5f, Blend);
			else
				TextRender()->TextColor(0.7f, 0.7f, 0.7f, Blend);
			TextRender()->Text(0, x, y, FontSize, aDiff, -1);
			if(m_aTimemsgs[r].m_PlayerID != m_pClient->m_Snap.m_LocalClientID)
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);
			
			x -= 16.0f;
		}
		
		// render server diff
		if(m_aTimemsgs[r].m_ServerDiff)
		{
			char aBuf[32];
			char aDiff[32];
			IRace::FormatTimeDiff(aDiff, sizeof(aDiff), m_aTimemsgs[r].m_ServerDiff);
			str_format(aBuf, sizeof(aBuf), "(%s)", aDiff);
			float DiffW = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			
			x -= DiffW;
			TextRender()->TextColor(0.0f, 0.5f, 1.0f, Blend);
			TextRender()->Text(0, x, y, FontSize, aDiff, -1);
			if(m_aTimemsgs[r].m_PlayerID != m_pClient->m_Snap.m_LocalClientID)
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
		if(m_aTimemsgs[r].m_LocalDiff < 0)
			RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aTimemsgs[r].m_PlayerRenderInfo, EMOTE_HAPPY, vec2(-1,0), vec2(x, y+28));
		else
			RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aTimemsgs[r].m_PlayerRenderInfo, EMOTE_NORMAL, vec2(-1,0), vec2(x, y+28));
		
		// new line
		y += 46;
	}
	
	TextRender()->BatchEnd();
}
