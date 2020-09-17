/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>

#include "broadcast.h"
#include "chat.h"
#include "scoreboard.h"
#include "motd.h"

#define BROADCAST_FONTSIZE_BIG 9.5f
#define BROADCAST_FONTSIZE_MEDIUM 7.5f
#define BROADCAST_FONTSIZE_SMALL 6.5f

inline bool IsCharANum(char c)
{
	return c >= '0' && c <= '9';
}

void CBroadcast::RenderServerBroadcast()
{
	if(!Config()->m_ClShowServerBroadcast || m_MuteServerBroadcast || m_aSrvBroadcastMsg[0] == 0)
		return;
	if(m_pClient->m_pChat->IsActive())
		return;

	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();

	const float DisplayDuration = 10.0f;
	const float DisplayStartFade = 9.0f;
	const float DeltaTime = Client()->LocalTime() - m_SrvBroadcastReceivedTime;

	if(m_NumSegments < 0 || !m_SrvBroadcastCursor.Rendered() || DeltaTime > DisplayDuration)
		return;

	if(m_pClient->m_pChat->IsActive() || m_pClient->Client()->State() != IClient::STATE_ONLINE)
		return;

	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, Height);

	const float Fade = 1.0f - max(0.0f, (DeltaTime - DisplayStartFade) / (DisplayDuration - DisplayStartFade));
	const float FontSize = m_SrvBroadcastFontSize;
	const int LineCount = m_SrvBroadcastLineCount;

	CUIRect BcView;
	BcView.w = Width * 0.5f;
	BcView.h = 1.5f * LineCount * FontSize + 4.0f;
	BcView.x = BcView.w * 0.5f;
	BcView.y = Height - BcView.h;

	CUIRect BgRect;
	BcView.HSplitBottom(1.5f * LineCount * FontSize, 0, &BgRect);
	BcView.HSplitBottom(2.0f, &BcView, 0);

	// draw bottom bar
	const float CornerWidth = 10.0f;
	const float CornerHeight = BgRect.h;
	BgRect.VMargin(CornerWidth, &BgRect);

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
			BgRect.x + ca * -CornerWidth,
			BgRect.y + CornerHeight + sa * -CornerHeight,

			BgRect.x, BgRect.y + CornerHeight,

			BgRect.x + ca1 * -CornerWidth,
			BgRect.y + CornerHeight + sa1 * -CornerHeight,

			BgRect.x + ca2 * -CornerWidth,
			BgRect.y + CornerHeight + sa2 *- CornerHeight
		);
		LeftCornerQuads[q] = LQuad;

		IGraphics::CFreeformItem RQuad(
			BgRect.x + BgRect.w + ca * CornerWidth,
			BgRect.y + CornerHeight + sa * -CornerHeight,

			BgRect.x + BgRect.w, BgRect.y + CornerHeight,

			BgRect.x + BgRect.w + ca1 * CornerWidth,
			BgRect.y + CornerHeight + sa1 * -CornerHeight,

			BgRect.x + BgRect.w + ca2 * CornerWidth,
			BgRect.y + CornerHeight + sa2 *- CornerHeight
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
		RenderTools()->DrawUIRect4(&BgRect, ColorTop, ColorTop, ColorBot, ColorBot, 0, 0.0f);
	}

	BcView.VMargin(5.0f, &BcView);
	BcView.HSplitBottom(2.0f, &BcView, 0);

	const vec2 ShadowOffset(2.0f, 2.0f);
	m_SrvBroadcastCursor.MoveTo(BcView.x+BcView.w/2, BcView.y+BcView.h+3.0f);

	TextRender()->DrawTextShadowed(&m_SrvBroadcastCursor, ShadowOffset, Fade, 0, m_aSrvBroadcastSegments[0].m_GlyphPos);
	for(int i = 0; i < m_NumSegments; ++i)
	{
		CBroadcast::CBcSegment *pSegment = &m_aSrvBroadcastSegments[i];
		if(m_aSrvBroadcastSegments[i].m_IsHighContrast)
			TextRender()->DrawTextOutlined(&m_SrvBroadcastCursor, Fade, pSegment->m_GlyphPos, (pSegment+1)->m_GlyphPos);
		else
			TextRender()->DrawTextShadowed(&m_SrvBroadcastCursor, ShadowOffset, Fade, pSegment->m_GlyphPos, (pSegment+1)->m_GlyphPos);
	}
}

void CBroadcast::RenderClientBroadcast()
{
	if(m_pClient->m_pScoreboard->IsActive() || m_pClient->m_pMotd->IsActive() || m_aBroadcastText[0] == 0)
		return;
	if(Client()->LocalTime() >= m_BroadcastTime)
		return;

	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();

	Graphics()->MapScreen(0, 0, Width, Height);

	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, m_BroadcastRenderOffset, 40.0f, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = Width - m_BroadcastRenderOffset;
	TextRender()->TextEx(&Cursor, m_aBroadcastText, -1);
}

CBroadcast::CBroadcast()
{
	OnReset();
}

void CBroadcast::DoClientBroadcast(const char *pText)
{
	m_BroadcastCursor.Reset();
	m_BroadcastCursor.m_FontSize = 12.0f;
	m_BroadcastCursor.m_Align = TEXTALIGN_TC;
	m_BroadcastCursor.m_MaxWidth = 300*Graphics()->ScreenAspect();

	TextRender()->TextDeferred(&m_BroadcastCursor, pText, -1);
	m_BroadcastTime = Client()->LocalTime() + 10.0f;
}

void CBroadcast::OnReset()
{
	m_BroadcastTime = 0;
	m_SrvBroadcastReceivedTime = 0;
	m_NumSegments = -1;
}

void CBroadcast::OnMessage(int MsgType, void* pRawMsg)
{
	// process server broadcast message
	if(MsgType == NETMSGTYPE_SV_BROADCAST)
	{

		CNetMsg_Sv_Broadcast *pMsg = (CNetMsg_Sv_Broadcast *)pRawMsg;

		// new broadcast message
		int RcvMsgLen = str_length(pMsg->m_pMessage);
		m_SrvBroadcastReceivedTime = Client()->LocalTime();

		char aBuf[MAX_BROADCAST_MSG_LENGTH];
		vec4 SegmentColors[MAX_BROADCAST_MSG_LENGTH];
		int SegmentIndices[MAX_BROADCAST_MSG_LENGTH];
		m_NumSegments = 0;
		int SrvMsgLen = 0;

		// parse colors and newline
		for(int i = 0; i < RcvMsgLen && SrvMsgLen < MAX_BROADCAST_MSG_LENGTH - 1; i++)
		{
			const char *c = pMsg->m_pMessage + i;
			if(*c == '^' && i+3 < RcvMsgLen && IsCharANum(c[1]) && IsCharANum(c[2])  && IsCharANum(c[3]))
			{
				SegmentColors[m_NumSegments] = vec4((c[1] - '0') / 9.0f, (c[2] - '0') / 9.0f, (c[3] - '0') / 9.0f, 1.0f);
				SegmentIndices[m_NumSegments] = SrvMsgLen;
				m_NumSegments++;
				i += 3;
				continue;
			}
		}

			if(*c == '\\' && c[1] == 'n')
			{
				aBuf[SrvMsgLen++] = '\n';
				i++;
				continue;
			}

			aBuf[SrvMsgLen++] = *c;
		}
		SegmentIndices[m_NumSegments] = SrvMsgLen;
		aBuf[SrvMsgLen] = 0;

		const float Height = 300;
		const float Width = Height*Graphics()->ScreenAspect();
		const float MaxWidth = Width * 0.5f - 10.0f;
		const float MaxHeight = BROADCAST_FONTSIZE_SMALL * MAX_BROADCAST_LINES;

		Graphics()->MapScreen(0, 0, Width, Height);

		float FontSize = BROADCAST_FONTSIZE_BIG;

		m_SrvBroadcastFontSize = FontSize;

		// find approprate font size in first pass
		m_SrvBroadcastCursor.Reset();
		m_SrvBroadcastCursor.m_FontSize = FontSize;
		m_SrvBroadcastCursor.m_Align = TEXTALIGN_BC;
		m_SrvBroadcastCursor.m_Flags = TEXTFLAG_ALLOW_NEWLINE | TEXTFLAG_NO_RENDER | TEXTFLAG_WORD_WRAP;
		m_SrvBroadcastCursor.m_MaxWidth = -1;
		m_SrvBroadcastCursor.m_MaxLines = MAX_BROADCAST_LINES;
		TextRender()->TextDeferred(&m_SrvBroadcastCursor, aBuf, SegmentIndices[0]);
		for(int i = 0; i < m_NumSegments; i++)
			TextRender()->TextDeferred(&m_SrvBroadcastCursor, aBuf + SegmentIndices[i], SegmentIndices[i+1] - SegmentIndices[i]);

		if(m_SrvBroadcastCursor.Width() > MaxWidth)
			FontSize = max(BROADCAST_FONTSIZE_MEDIUM, FontSize * (MaxWidth / m_SrvBroadcastCursor.Width()));
		
		if(m_SrvBroadcastCursor.Height() > MaxHeight)
			FontSize = max(BROADCAST_FONTSIZE_SMALL, FontSize * (MaxHeight / m_SrvBroadcastCursor.Height()));
		
		m_SrvBroadcastCursor.m_MaxWidth = MaxWidth;
		m_SrvBroadcastCursor.m_Flags &= ~TEXTFLAG_NO_RENDER;
		m_SrvBroadcastCursor.m_FontSize = FontSize;
		m_SrvBroadcastCursor.Reset();

		vec4 OldColor = TextRender()->GetColor();
		vec4 OldSecondary = TextRender()->GetSecondaryColor();

		TextRender()->TextColor(1,1,1,1);
		TextRender()->TextSecondaryColor(0, 0, 0, 1);
		TextRender()->TextDeferred(&m_SrvBroadcastCursor, aBuf, SegmentIndices[0]);
		for(int i = 0; i < m_NumSegments; i++)
		{
			if(Config()->m_ClColoredBroadcast)
			{
				float AvgLum = 0.2126*SegmentColors[i].r + 0.7152*SegmentColors[i].g + 0.0722*SegmentColors[i].b;
				TextRender()->TextColor(SegmentColors[i]);
				if(AvgLum < 0.25f)
				{
					m_aSrvBroadcastSegments[i].m_IsHighContrast = true;
					TextRender()->TextSecondaryColor(1, 1, 1, 0.6f);
				}
				else
				{
					m_aSrvBroadcastSegments[i].m_IsHighContrast = false;
					TextRender()->TextSecondaryColor(0, 0, 0, 1);
				}
			}
			else
			{
				TextRender()->TextSecondaryColor(0, 0, 0, 1);
				m_aSrvBroadcastSegments[i].m_IsHighContrast = false;
			}
			m_aSrvBroadcastSegments[i].m_GlyphPos = m_SrvBroadcastCursor.GlyphCount();
			TextRender()->TextDeferred(&m_SrvBroadcastCursor, aBuf + SegmentIndices[i], SegmentIndices[i+1] - SegmentIndices[i]);
		}
		m_aSrvBroadcastSegments[m_NumSegments].m_GlyphPos = m_SrvBroadcastCursor.GlyphCount();
		TextRender()->TextColor(OldColor);
		TextRender()->TextSecondaryColor(OldSecondary);
	}

	m_SrvBroadcastFontSize = FontSize;
}

void CBroadcast::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	RenderServerBroadcast();
	RenderClientBroadcast();
}
