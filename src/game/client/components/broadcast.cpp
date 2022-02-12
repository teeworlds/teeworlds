/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>

#include "broadcast.h"
#include "chat.h"
#include "scoreboard.h"
#include "motd.h"

static const float BROADCAST_FONTSIZE_BIG = 11.0f;
static const float BROADCAST_FONTSIZE_SMALL = 6.5f;

inline bool IsCharANum(char c)
{
	return c >= '0' && c <= '9';
}

inline bool IsCharWhitespace(char c)
{
	return c == '\n' || c == '\t' || c == ' ';
}

inline int WordLengthBack(const char *pText, int MaxChars)
{
	int s = 0;
	while(MaxChars--)
	{
		if(IsCharWhitespace(*pText))
			return s;
		pText--;
		s++;
	}
	return 0;
}

void CBroadcast::RenderServerBroadcast()
{
	if(!Config()->m_ClShowServerBroadcast || m_MuteServerBroadcast || !m_ServerBroadcastCursor.Rendered())
		return;
	if(m_pClient->m_pChat->IsActive())
		return;

	const float DisplayDuration = 10.0f;
	const float DisplayStartFade = 9.0f;
	const float DeltaTime = Client()->LocalTime() - m_ServerBroadcastReceivedTime;

	if(DeltaTime > DisplayDuration)
		return;

	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, Height);

	const float Fade = 1.0f - maximum(0.0f, (DeltaTime - DisplayStartFade) / (DisplayDuration - DisplayStartFade));
	const float TextHeight = m_ServerBroadcastCursor.BoundingBox().h;
	const float Rounding = 5.0f;

	CUIRect BroadcastView;
	BroadcastView.w = Width * 0.5f;
	BroadcastView.h = maximum(TextHeight, 2*Rounding) + 2.0f;
	BroadcastView.x = BroadcastView.w * 0.5f;
	BroadcastView.y = Height - BroadcastView.h;

	BroadcastView.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f * Fade), Rounding, CUIRect::CORNER_T);

	// draw lines
	const vec2 ShadowOffset(1.0f, 2.0f);
	m_ServerBroadcastCursor.MoveTo(BroadcastView.x + BroadcastView.w/2, BroadcastView.y + BroadcastView.h/2 - TextHeight/2);

	TextRender()->DrawTextShadowed(&m_ServerBroadcastCursor, ShadowOffset, Fade, 0, m_aServerBroadcastSegments[0].m_GlyphPos);
	for(int i = 0; i < m_NumSegments; ++i)
	{
		CBroadcastSegment *pSegment = &m_aServerBroadcastSegments[i];
		if(m_aServerBroadcastSegments[i].m_IsHighContrast)
			TextRender()->DrawTextOutlined(&m_ServerBroadcastCursor, Fade, pSegment->m_GlyphPos, (pSegment+1)->m_GlyphPos);
		else
			TextRender()->DrawTextShadowed(&m_ServerBroadcastCursor, ShadowOffset, Fade, pSegment->m_GlyphPos, (pSegment+1)->m_GlyphPos);
	}
}

void CBroadcast::RenderClientBroadcast()
{
	if(m_pClient->m_pScoreboard->IsActive() || m_pClient->m_pMotd->IsActive() || !m_ClientBroadcastCursor.Rendered())
		return;
	if(Client()->LocalTime() >= m_BroadcastTime)
		return;

	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();

	Graphics()->MapScreen(0, 0, Width, Height);

	m_ClientBroadcastCursor.MoveTo(300*Graphics()->ScreenAspect() / 2, 40.0f);
	TextRender()->DrawTextOutlined(&m_ClientBroadcastCursor);
}

CBroadcast::CBroadcast()
{
	OnReset();
}

void CBroadcast::DoClientBroadcast(const char *pText)
{
	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, Height);

	m_ClientBroadcastCursor.Reset();
	m_ClientBroadcastCursor.m_FontSize = 12.0f;
	m_ClientBroadcastCursor.m_Align = TEXTALIGN_TC;
	m_ClientBroadcastCursor.m_MaxWidth = Width;

	TextRender()->TextDeferred(&m_ClientBroadcastCursor, pText, -1);
	m_BroadcastTime = Client()->LocalTime() + 10.0f;
}

void CBroadcast::OnReset()
{
	m_BroadcastTime = 0;
	m_ServerBroadcastReceivedTime = 0;
	m_MuteServerBroadcast = false;
	m_NumSegments = -1;
}

void CBroadcast::OnMessage(int MsgType, void* pRawMsg)
{
	// process server broadcast message
	if(MsgType == NETMSGTYPE_SV_BROADCAST)
	{
		OnBroadcastMessage((CNetMsg_Sv_Broadcast *)pRawMsg);
	}
}

void CBroadcast::OnBroadcastMessage(const CNetMsg_Sv_Broadcast *pMsg)
{
	if(!Config()->m_ClShowServerBroadcast || m_MuteServerBroadcast)
		return;

	// new broadcast message
	int MsgLength = str_length(pMsg->m_pMessage);
	m_ServerBroadcastReceivedTime = Client()->LocalTime();

	char aBuf[MAX_BROADCAST_MSG_SIZE];
	vec4 SegmentColors[MAX_BROADCAST_MSG_SIZE];
	int SegmentIndices[MAX_BROADCAST_MSG_SIZE];
	m_NumSegments = 0;
	int ServerMsgLen = 0;
	int UserLineCount = 1;

	// parse colors and newline
	for(int i = 0; i < MsgLength && ServerMsgLen < MAX_BROADCAST_MSG_SIZE - 1; i++)
	{
		const char *c = pMsg->m_pMessage + i;
		if(*c == '^' && i+3 < MsgLength && IsCharANum(c[1]) && IsCharANum(c[2])  && IsCharANum(c[3]))
		{
			SegmentColors[m_NumSegments] = vec4((c[1] - '0') / 9.0f, (c[2] - '0') / 9.0f, (c[3] - '0') / 9.0f, 1.0f);
			SegmentIndices[m_NumSegments] = ServerMsgLen;
			m_NumSegments++;
			i += 3; // skip color code
			continue;
		}

		if(*c == '\\' && c[1] == 'n')
		{
			if(UserLineCount >= MAX_BROADCAST_LINES)
				break;
			UserLineCount++;
			aBuf[ServerMsgLen++] = '\n';
			i++; // skip 'n'
			continue;
		}

		aBuf[ServerMsgLen++] = *c;
	}
	SegmentIndices[m_NumSegments] = ServerMsgLen;
	aBuf[ServerMsgLen] = 0;

	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();
	const float LineMaxWidth = Width * 0.5f - 10.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	// one line == big font
	// 2+ lines == small font
	float FontSize = BROADCAST_FONTSIZE_BIG;

	m_ServerBroadcastCursor.Reset();
	m_ServerBroadcastCursor.m_FontSize = FontSize;
	m_ServerBroadcastCursor.m_Align = TEXTALIGN_TC;
	m_ServerBroadcastCursor.m_MaxWidth = LineMaxWidth;
	m_ServerBroadcastCursor.m_MaxLines = MAX_BROADCAST_LINES;
	m_ServerBroadcastCursor.m_Flags = TEXTFLAG_ALLOW_NEWLINE | TEXTFLAG_WORD_WRAP;

	if(UserLineCount <= 1) // auto mode
	{
		m_ServerBroadcastCursor.m_FontSize = FontSize;
		m_ServerBroadcastCursor.m_Flags |= TEXTFLAG_NO_RENDER;

		TextRender()->TextOutlined(&m_ServerBroadcastCursor, aBuf, ServerMsgLen);

		// can't fit on one line, reduce size
		int NumLines = m_ServerBroadcastCursor.LineCount();
		FontSize = mix(BROADCAST_FONTSIZE_BIG, BROADCAST_FONTSIZE_SMALL,
					   NumLines/(float)MAX_BROADCAST_LINES);
	}
	else // user defined lines mode
	{
		FontSize = mix(BROADCAST_FONTSIZE_BIG, BROADCAST_FONTSIZE_SMALL,
					   UserLineCount/(float)MAX_BROADCAST_LINES);
	}

	m_ServerBroadcastCursor.Reset();
	m_ServerBroadcastCursor.m_Flags &= ~TEXTFLAG_NO_RENDER;
	m_ServerBroadcastCursor.m_FontSize = FontSize;

	vec4 OldColor = TextRender()->GetColor();
	vec4 OldSecondary = TextRender()->GetSecondaryColor();

	TextRender()->TextColor(1, 1, 1, 1);
	TextRender()->TextSecondaryColor(0, 0, 0, 1);
	TextRender()->TextDeferred(&m_ServerBroadcastCursor, aBuf, SegmentIndices[0]);
	for(int i = 0; i < m_NumSegments; i++)
	{
		if(Config()->m_ClColoredBroadcast)
		{
			float AvgLum = 0.2126*SegmentColors[i].r + 0.7152*SegmentColors[i].g + 0.0722*SegmentColors[i].b;
			TextRender()->TextColor(SegmentColors[i]);
			if(AvgLum < 0.25f)
			{
				m_aServerBroadcastSegments[i].m_IsHighContrast = true;
				TextRender()->TextSecondaryColor(1, 1, 1, 0.6f);
			}
			else
			{
				m_aServerBroadcastSegments[i].m_IsHighContrast = false;
				TextRender()->TextSecondaryColor(0, 0, 0, 1);
			}
		}
		else
		{
			TextRender()->TextSecondaryColor(0, 0, 0, 1);
			m_aServerBroadcastSegments[i].m_IsHighContrast = false;
		}
		m_aServerBroadcastSegments[i].m_GlyphPos = m_ServerBroadcastCursor.GlyphCount();
		TextRender()->TextDeferred(&m_ServerBroadcastCursor, aBuf + SegmentIndices[i], SegmentIndices[i+1] - SegmentIndices[i]);
	}
	m_aServerBroadcastSegments[m_NumSegments].m_GlyphPos = m_ServerBroadcastCursor.GlyphCount();
	TextRender()->TextColor(OldColor);
	TextRender()->TextSecondaryColor(OldSecondary);
}

void CBroadcast::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	RenderServerBroadcast();
	RenderClientBroadcast();
}
