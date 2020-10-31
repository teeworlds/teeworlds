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

	const float Fade = 1.0f - max(0.0f, (DeltaTime - DisplayStartFade) / (DisplayDuration - DisplayStartFade));
	const int FontSize = m_ServerBroadcastCursor.m_FontSize;
	const int LineCount = m_ServerBroadcastCursor.LineCount();

	CUIRect BroadcastView;
	BroadcastView.w = Width * 0.5f;
	BroadcastView.h = 1.5f * LineCount * FontSize + 4.0f;
	BroadcastView.x = BroadcastView.w * 0.5f;
	BroadcastView.y = Height - BroadcastView.h;

	CUIRect BackgroundRect;
	BroadcastView.HSplitBottom(1.5f * LineCount * FontSize, 0, &BackgroundRect);
	BroadcastView.HSplitBottom(2.0f, &BroadcastView, 0);

	// draw bottom bar
	const float CornerWidth = 10.0f;
	const float CornerHeight = BackgroundRect.h;
	BackgroundRect.VMargin(CornerWidth, &BackgroundRect);

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// make round corners
	enum { CORNER_MAX_QUADS=4 };
	IGraphics::CFreeformItem LeftCornerQuads[CORNER_MAX_QUADS];
	IGraphics::CFreeformItem RightCornerQuads[CORNER_MAX_QUADS];
	const float AngleStep = (pi * 0.5f)/(CORNER_MAX_QUADS * 2);

	for(int q = 0; q < CORNER_MAX_QUADS; q++)
	{
		const float Angle = AngleStep * q * 2;
		const float ca = cosf(Angle);
		const float ca1 = cosf(Angle + AngleStep);
		const float ca2 = cosf(Angle + AngleStep * 2);
		const float sa = sinf(Angle);
		const float sa1 = sinf(Angle + AngleStep);
		const float sa2 = sinf(Angle + AngleStep * 2);

		IGraphics::CFreeformItem LQuad(
			BackgroundRect.x + ca * -CornerWidth,
			BackgroundRect.y + CornerHeight + sa * -CornerHeight,

			BackgroundRect.x, BackgroundRect.y + CornerHeight,

			BackgroundRect.x + ca1 * -CornerWidth,
			BackgroundRect.y + CornerHeight + sa1 * -CornerHeight,

			BackgroundRect.x + ca2 * -CornerWidth,
			BackgroundRect.y + CornerHeight + sa2 *- CornerHeight
		);
		LeftCornerQuads[q] = LQuad;

		IGraphics::CFreeformItem RQuad(
			BackgroundRect.x + BackgroundRect.w + ca * CornerWidth,
			BackgroundRect.y + CornerHeight + sa * -CornerHeight,

			BackgroundRect.x + BackgroundRect.w, BackgroundRect.y + CornerHeight,

			BackgroundRect.x + BackgroundRect.w + ca1 * CornerWidth,
			BackgroundRect.y + CornerHeight + sa1 * -CornerHeight,

			BackgroundRect.x + BackgroundRect.w + ca2 * CornerWidth,
			BackgroundRect.y + CornerHeight + sa2 *- CornerHeight
		);
		RightCornerQuads[q] = RQuad;
	}

	IGraphics::CColorVertex aColorVert[4] = {
		IGraphics::CColorVertex(0, 0, 0, 0, 0.0f),
		IGraphics::CColorVertex(1, 0, 0, 0, 0.7f * Fade),
		IGraphics::CColorVertex(2, 0, 0, 0, 0.0f),
		IGraphics::CColorVertex(3, 0, 0, 0, 0.0f)
	};

	Graphics()->SetColorVertex(aColorVert, 4);
	Graphics()->QuadsDrawFreeform(LeftCornerQuads, CORNER_MAX_QUADS);
	Graphics()->QuadsDrawFreeform(RightCornerQuads, CORNER_MAX_QUADS);

	Graphics()->QuadsEnd();

	{
		vec4 ColorTop(0, 0, 0, 0.01f * Fade);
		vec4 ColorBot(0, 0, 0, 0.7f * Fade);
		RenderTools()->DrawUIRect4(&BackgroundRect, ColorTop, ColorTop, ColorBot, ColorBot, 0, 0.0f);
	}

	BroadcastView.VMargin(5.0f, &BroadcastView);
	BroadcastView.HSplitBottom(2.0f, &BroadcastView, 0);

	// draw lines
	const vec2 ShadowOffset(1.0f, 2.0f);
	float y = BroadcastView.y + BroadcastView.h - LineCount * FontSize;
	m_ServerBroadcastCursor.MoveTo(BroadcastView.x+BroadcastView.w/2, y);

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
	m_ClientBroadcastCursor.Reset();
	m_ClientBroadcastCursor.m_FontSize = 12.0f;
	m_ClientBroadcastCursor.m_Align = TEXTALIGN_TC;
	m_ClientBroadcastCursor.m_MaxWidth = 300*Graphics()->ScreenAspect();

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
	int UserLineCount = 0;

	// parse colors and newline
	for(int i = 0; i < MsgLength && ServerMsgLen < MAX_BROADCAST_MSG_SIZE - 1; i++)
	{
		const char *c = pMsg->m_pMessage + i;
		if(*c == '^' && i+3 < MsgLength && IsCharANum(c[1]) && IsCharANum(c[2])  && IsCharANum(c[3]))
		{
			SegmentColors[m_NumSegments] = vec4((c[1] - '0') / 9.0f, (c[2] - '0') / 9.0f, (c[3] - '0') / 9.0f, 1.0f);
			SegmentIndices[m_NumSegments] = ServerMsgLen;
			m_NumSegments++;
			i += 3;
			continue;
		}

		if(*c == '\\' && c[1] == 'n')
		{
			aBuf[ServerMsgLen++] = '\n';
			UserLineCount += 1;
			i++;
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
