/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>

#include <game/client/components/motd.h>
#include <game/client/components/scoreboard.h>

#include "broadcast.h"


CBroadcast::CBroadcast()
{
	OnReset();
}

void CBroadcast::DoBroadcast(const char *pText)
{
	str_copy(m_aBroadcastText, pText, sizeof(m_aBroadcastText));
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, 0, 0, 12.0f, TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = 300*Graphics()->ScreenAspect();
	TextRender()->TextEx(&Cursor, m_aBroadcastText, -1);
	m_BroadcastRenderOffset = 150*Graphics()->ScreenAspect()-Cursor.m_X/2;
	m_BroadcastTime = time_get()+time_freq()*10;
}

void CBroadcast::OnReset()
{
	m_BroadcastTime = 0;
}

void CBroadcast::OnRender()
{
	if(m_pClient->m_pScoreboard->Active() || m_pClient->m_pMotd->IsActive())
		return;

	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);

	if(time_get() < m_BroadcastTime)
	{
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, m_BroadcastRenderOffset, 40.0f, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 300*Graphics()->ScreenAspect()-m_BroadcastRenderOffset;
		TextRender()->TextEx(&Cursor, m_aBroadcastText, -1);
	}
}

