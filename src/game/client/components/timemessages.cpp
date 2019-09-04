/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

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
	if(MsgType == NETMSGTYPE_SV_RACEFINISH)
	{
		CNetMsg_Sv_RaceFinish *pMsg = (CNetMsg_Sv_RaceFinish *)pRawMsg;

		// unpack messages
		CTimeMsg Time;
		Time.m_PlayerID = pMsg->m_ClientID;
		Time.m_Time = pMsg->m_Time;
		Time.m_Diff = pMsg->m_Diff;
		Time.m_NewRecord = pMsg->m_NewRecord;
		str_format(Time.m_aPlayerName, sizeof(Time.m_aPlayerName), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[Time.m_PlayerID].m_aName : "");
		Time.m_PlayerRenderInfo = m_pClient->m_aClients[Time.m_PlayerID].m_RenderInfo;
		Time.m_Tick = Client()->GameTick();

		// add the message
		m_TimemsgCurrent = (m_TimemsgCurrent+1)%MAX_TIMEMSGS;
		m_aTimemsgs[m_TimemsgCurrent] = Time;
	}
}

void FormatTimeShort(char *pBuf, int Size, int Time, bool ForceMinutes = false)
{
	if(!ForceMinutes && Time < 60 * 1000)
		str_format(pBuf, Size, "%d.%03d", Time / 1000, Time % 1000);
	else
		str_format(pBuf, Size, "%02d:%02d.%03d", Time / (60 * 1000), (Time / 1000) % 60, Time % 1000);
}

void FormatTimeDiff(char *pBuf, int Size, int Time, bool Milli = true)
{
	int PosDiff = absolute(Time);
	char Sign = Time < 0 ? '-' : '+';
	if(Milli)
		str_format(pBuf, Size, "%c%d.%03d", Sign, PosDiff / 1000, PosDiff % 1000);
	else
		str_format(pBuf, Size, "%c%d.%02d", Sign, PosDiff / 1000, (PosDiff % 1000) / 10);
}

void CTimeMessages::OnRender()
{
	if(!(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_RACE))
		return;

	float Width = 350*3.0f*Graphics()->ScreenAspect();
	float Height = 350*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;

	for(int i = 1; i <= MAX_TIMEMSGS; i++)
	{
		int r = (m_TimemsgCurrent+i)%MAX_TIMEMSGS;
		if(Client()->GameTick() > m_aTimemsgs[r].m_Tick+50*20)
			continue;

		float FontSize = 36.0f;
		float PlayerNameW = TextRender()->TextWidth(0, FontSize, m_aTimemsgs[r].m_aPlayerName, -1, -1.0f) + RenderTools()->GetClientIdRectSize(FontSize);

		float x = StartX;

		// render diff
		if(m_aTimemsgs[r].m_Diff != 0)
		{
			char aBuf[32];
			char aDiff[32];
			FormatTimeDiff(aDiff, sizeof(aDiff), m_aTimemsgs[r].m_Diff);
			str_format(aBuf, sizeof(aBuf), "(%s)", aDiff);
			float DiffW = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);

			x -= DiffW;
			if(m_aTimemsgs[r].m_Diff < 0)
				TextRender()->TextColor(0.5f, 1.0f, 0.5f, 1.0f);
			else
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, 1.0f);
			TextRender()->Text(0, x, y, FontSize, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			x -= 16.0f;
		}

		// render time
		char aTime[32];
		FormatTimeShort(aTime, sizeof(aTime), m_aTimemsgs[r].m_Time, true);
		float TimeW = TextRender()->TextWidth(0, FontSize, aTime, -1, -1.0f);

		x -= TimeW;
		if(m_aTimemsgs[r].m_NewRecord)
			TextRender()->TextColor(0.0f, 0.5f, 1.0f, 1.0f);
		TextRender()->Text(0, x, y, FontSize, aTime, -1);

		x -= 52.0f;

		// render flag
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_RACEFLAG].m_Id);
		Graphics()->QuadsBegin();
		IGraphics::CQuadItem QuadItem(x, y, 52, 52);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		x -= 10.0f;

		// render player name
		x -= PlayerNameW;
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);

		RenderTools()->DrawClientID(TextRender(), &Cursor, m_aTimemsgs[r].m_PlayerID);
		TextRender()->TextEx(&Cursor, m_aTimemsgs[r].m_aPlayerName, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		x -= 28.0f;

		// render player tee
		int Emote = m_aTimemsgs[r].m_NewRecord ? EMOTE_HAPPY : EMOTE_NORMAL;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aTimemsgs[r].m_PlayerRenderInfo, Emote, vec2(-1,0), vec2(x, y+28));
		
		// new line
		y += 46;
	}
}
