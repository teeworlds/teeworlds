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

#define BROADCAST_FONTSIZE_BIG 11.0f
#define BROADCAST_FONTSIZE_SMALL 6.5f

inline bool IsCharANum(char c)
{
	return c >= '0' && c <= '9';
}

inline int WordLengthBack(const char *pText, int MaxChars)
{
	int s = 0;
	while(MaxChars--)
	{
		if((*pText == '\n' || *pText == '\t' || *pText == ' '))
			return s;
		pText--;
		s++;
	}
	return 0;
}

inline bool IsCharWhitespace(char c)
{
	return c == '\n' || c == '\t' || c == ' ';
}

void CBroadcast::RenderServerBroadcast()
{
	if(!g_Config.m_ClShowServerBroadcast || m_pClient->m_MuteServerBroadcast)
		return;

	const bool ColoredBroadcastEnabled = g_Config.m_ClColoredBroadcast;
	const float Height = 300;
	const float Width = Height*Graphics()->ScreenAspect();

	const float DisplayDuration = 10.0f;
	const float DisplayStartFade = 9.0f;
	const float DeltaTime = Client()->LocalTime() - m_SrvBroadcastReceivedTime;

	if(m_aSrvBroadcastMsg[0] == 0 || DeltaTime > DisplayDuration)
		return;

	if(m_pClient->m_pChat->IsActive() || m_pClient->Client()->State() != IClient::STATE_ONLINE)
		return;

	Graphics()->MapScreen(0, 0, Width, Height);

	const float Fade = 1.0f - max(0.0f, (DeltaTime - DisplayStartFade) / (DisplayDuration - DisplayStartFade));

	CUIRect ScreenRect = {0, 0, Width, Height};

	CUIRect BcView = ScreenRect;
	BcView.x += Width * 0.25f;
	BcView.y += Height * 0.8f;
	BcView.w *= 0.5f;
	BcView.h *= 0.2f;

	vec4 ColorTop(0, 0, 0, 0);
	vec4 ColorBot(0, 0, 0, 0.4f * Fade);
	CUIRect BgRect;
	BcView.HSplitBottom(10.0f, 0, &BgRect);
	BcView.HSplitBottom(6.0f, &BcView, 0);

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
		IGraphics::CColorVertex(0, 0,0,0, 0.0f),
		IGraphics::CColorVertex(1, 0,0,0, 0.4f * Fade),
		IGraphics::CColorVertex(2, 0,0,0, 0.0f),
		IGraphics::CColorVertex(3, 0,0,0, 0.0f)
	};

	Graphics()->SetColorVertex(aColorVert, 4);
	Graphics()->QuadsDrawFreeform(LeftCornerQuads, CORNER_MAX_QUADS);
	Graphics()->QuadsDrawFreeform(RightCornerQuads, CORNER_MAX_QUADS);

	Graphics()->QuadsEnd();

	RenderTools()->DrawUIRect4(&BgRect, ColorTop, ColorTop,
							   ColorBot, ColorBot, 0, 0);


	BcView.VMargin(5.0f, &BcView);
	BcView.HSplitBottom(2.0f, &BcView, 0);

	// draw lines
	const float FontSize = m_SrvBroadcastFontSize;
	const int LineCount = m_SrvBroadcastLineCount;
	const char* pBroadcastMsg = m_aSrvBroadcastMsg;
	CTextCursor Cursor;

	const vec2 ShadowOff(1.0f, 2.0f);
	const vec4 ShadowColorBlack(0, 0, 0, 0.9f * Fade);
	const vec4 TextColorWhite(1, 1, 1, Fade);
	float y = BcView.y + BcView.h - LineCount * FontSize;

	for(int l = 0; l < LineCount; l++)
	{
		const CBcLineInfo& Line = m_aSrvBroadcastLines[l];
		TextRender()->SetCursor(&Cursor, BcView.x + (BcView.w - Line.m_Width) * 0.5f, y,
								FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = BcView.w;

		// draw colored text
		if(ColoredBroadcastEnabled)
		{
			int DrawnStrLen = 0;
			int ThisCharPos = Line.m_pStrStart - pBroadcastMsg;
			while(DrawnStrLen < Line.m_StrLen)
			{
				int StartColorID = -1;
				int ColorStrLen = -1;

				// find color
				for(int j = 0; j < m_SrvBroadcastColorCount; j++)
				{
					if((ThisCharPos+DrawnStrLen) >= m_aSrvBroadcastColorList[j].m_CharPos)
					{
						StartColorID = j;
					}
					else if(StartColorID >= 0)
					{
						ColorStrLen = m_aSrvBroadcastColorList[j].m_CharPos -
									  max(m_aSrvBroadcastColorList[StartColorID].m_CharPos, ThisCharPos);
						break;
					}
				}

				dbg_assert(StartColorID >= 0, "This should not be -1, color not found");

				if(ColorStrLen < 0)
					ColorStrLen = Line.m_StrLen-DrawnStrLen;
				ColorStrLen = min(ColorStrLen, Line.m_StrLen-DrawnStrLen);

				const CBcColor& TextColor = m_aSrvBroadcastColorList[StartColorID];
				float r = TextColor.m_R/255.f;
				float g = TextColor.m_G/255.f;
				float b = TextColor.m_B/255.f;
				float AvgLum = 0.2126*r + 0.7152*g + 0.0722*b;

				if(AvgLum < 0.25f)
				{
					TextRender()->TextOutlineColor(1, 1, 1, 0.6f);
					TextRender()->TextColor(r, g, b, Fade);
					TextRender()->TextEx(&Cursor, Line.m_pStrStart+DrawnStrLen, ColorStrLen);
				}
				else
				{
					vec4 TextColor(r, g, b, Fade);
					vec4 ShadowColor(r * 0.15f, g * 0.15f, b * 0.15f, 0.9f * Fade);
					TextRender()->TextShadowed(&Cursor, Line.m_pStrStart+DrawnStrLen, ColorStrLen,
											   ShadowOff, ShadowColor, TextColor);
				}

				DrawnStrLen += ColorStrLen;
			}
		}
		else
		{
			TextRender()->TextShadowed(&Cursor, Line.m_pStrStart, Line.m_StrLen,
									   ShadowOff, ShadowColorBlack, TextColorWhite);
		}

		y += FontSize;
	}

	TextRender()->TextColor(1, 1, 1, 1);
	TextRender()->TextOutlineColor(0, 0, 0, 0.3f);
}

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
	m_BroadcastTime = Client()->LocalTime() + 10.0f;
}

void CBroadcast::OnReset()
{
	m_BroadcastTime = 0;
	m_SrvBroadcastReceivedTime = 0;
}

void CBroadcast::OnMessage(int MsgType, void* pRawMsg)
{
	// process server broadcast message
	if(MsgType == NETMSGTYPE_SV_BROADCAST && g_Config.m_ClShowServerBroadcast &&
	   !m_pClient->m_MuteServerBroadcast)
	{
		CNetMsg_Sv_Broadcast *pMsg = (CNetMsg_Sv_Broadcast *)pRawMsg;

		// new broadcast message
		int RcvMsgLen = str_length(pMsg->m_pMessage);
		mem_zero(m_aSrvBroadcastMsg, sizeof(m_aSrvBroadcastMsg));
		m_aSrvBroadcastMsgLen = 0;
		m_SrvBroadcastReceivedTime = Client()->LocalTime();

		const CBcColor White = { 255, 255, 255, 0 };
		m_aSrvBroadcastColorList[0] = White;
		m_SrvBroadcastColorCount = 1;

		CBcLineInfo UserLines[MAX_BROADCAST_LINES];
		int UserLineCount = 0;
		int LastUserLineStartPoint = 0;

		// parse colors
		for(int i = 0; i < RcvMsgLen; i++)
		{
			const char* c = pMsg->m_pMessage + i;
			const char* pTmp = c;
			int CharUtf8 = str_utf8_decode(&pTmp);
			const int Utf8Len = pTmp-c;

			if(*c == CharUtf8 && *c == '^')
			{
				if(i+3 < RcvMsgLen && IsCharANum(c[1]) && IsCharANum(c[2])  && IsCharANum(c[3]))
				{
					u8 r = (c[1] - '0') * 24 + 39;
					u8 g = (c[2] - '0') * 24 + 39;
					u8 b = (c[3] - '0') * 24 + 39;
					CBcColor Color = { r, g, b, m_aSrvBroadcastMsgLen };
					if(m_SrvBroadcastColorCount < MAX_BROADCAST_COLORS)
						m_aSrvBroadcastColorList[m_SrvBroadcastColorCount++] = Color;
					i += 3;
					continue;
				}
			}

			if(*c == CharUtf8 && *c == '\\')
			{
				if(i+1 < RcvMsgLen && c[1] == 'n' && UserLineCount < MAX_BROADCAST_LINES)
				{
					CBcLineInfo Line = { m_aSrvBroadcastMsg+LastUserLineStartPoint,
										 m_aSrvBroadcastMsgLen-LastUserLineStartPoint, 0 };
					if(Line.m_StrLen > 0)
						UserLines[UserLineCount++] = Line;
					LastUserLineStartPoint = m_aSrvBroadcastMsgLen;
					i++;
					continue;
				}
			}

			if(*c == '\n')
			{
				CBcLineInfo Line = { m_aSrvBroadcastMsg+LastUserLineStartPoint,
									 m_aSrvBroadcastMsgLen-LastUserLineStartPoint, 0 };
				if(Line.m_StrLen > 0)
					UserLines[UserLineCount++] = Line;
				LastUserLineStartPoint = m_aSrvBroadcastMsgLen;
				continue;
			}

			if(m_aSrvBroadcastMsgLen+Utf8Len < MAX_BROADCAST_MSG_LENGTH)
				m_aSrvBroadcastMsg[m_aSrvBroadcastMsgLen++] = *c;
		}

		// last user defined line
		if(LastUserLineStartPoint > 0 && UserLineCount < 3)
		{
			CBcLineInfo Line = { m_aSrvBroadcastMsg+LastUserLineStartPoint,
								 m_aSrvBroadcastMsgLen-LastUserLineStartPoint, 0 };
			if(Line.m_StrLen > 0)
				UserLines[UserLineCount++] = Line;
		}

		const float Height = 300;
		const float Width = Height*Graphics()->ScreenAspect();
		const float LineMaxWidth = Width * 0.5f - 10.0f;

		// process boradcast message
		const char* pBroadcastMsg = m_aSrvBroadcastMsg;
		const int MsgLen = m_aSrvBroadcastMsgLen;

		CTextCursor Cursor;
		Graphics()->MapScreen(0, 0, Width, Height);

		// one line == big font
		// 2+ lines == small font
		m_SrvBroadcastLineCount = 0;
		float FontSize = BROADCAST_FONTSIZE_BIG;

		if(UserLineCount <= 1) // auto mode
		{
			TextRender()->SetCursor(&Cursor, 0, 0, FontSize, 0);
			Cursor.m_LineWidth = LineMaxWidth;
			TextRender()->TextEx(&Cursor, pBroadcastMsg, MsgLen);

			// can't fit on one line, reduce size
			Cursor.m_LineCount = min(Cursor.m_LineCount, (int)MAX_BROADCAST_LINES);
			FontSize = mix(BROADCAST_FONTSIZE_BIG, BROADCAST_FONTSIZE_SMALL,
						   Cursor.m_LineCount/(float)MAX_BROADCAST_LINES);

			// make lines
			int CurCharCount = 0;
			while(CurCharCount < MsgLen && m_SrvBroadcastLineCount < MAX_BROADCAST_LINES)
			{
				const char* RemainingMsg = pBroadcastMsg + CurCharCount;

				TextRender()->SetCursor(&Cursor, 0, 0, FontSize, TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = LineMaxWidth;

				TextRender()->TextEx(&Cursor, RemainingMsg, -1);
				int StrLen = Cursor.m_CharCount;

				// don't cut words
				if(CurCharCount + StrLen < MsgLen)
				{
					const int WorldLen = WordLengthBack(RemainingMsg + StrLen, StrLen);
					if(WorldLen > 0 && WorldLen < StrLen)
					{
						StrLen -= WorldLen;
						TextRender()->SetCursor(&Cursor, 0, 0, FontSize, TEXTFLAG_STOP_AT_END);
						Cursor.m_LineWidth = LineMaxWidth;
						TextRender()->TextEx(&Cursor, RemainingMsg, StrLen);
					}
				}

				const float TextWidth = Cursor.m_X-Cursor.m_StartX;

				CBcLineInfo Line = { RemainingMsg, StrLen, TextWidth };
				m_aSrvBroadcastLines[m_SrvBroadcastLineCount++] = Line;
				CurCharCount += StrLen;
			}
		}
		else // user defined lines mode
		{
			FontSize = mix(BROADCAST_FONTSIZE_BIG, BROADCAST_FONTSIZE_SMALL,
						   UserLineCount/(float)MAX_BROADCAST_LINES);

			for(int i = 0; i < UserLineCount && m_SrvBroadcastLineCount < MAX_BROADCAST_LINES; i++)
			{
				TextRender()->SetCursor(&Cursor, 0, 0, FontSize, TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = LineMaxWidth;
				TextRender()->TextEx(&Cursor, UserLines[i].m_pStrStart, UserLines[i].m_StrLen);

				const float TextWidth = Cursor.m_X-Cursor.m_StartX;
				const int StrLen = Cursor.m_CharCount;

				CBcLineInfo Line = { UserLines[i].m_pStrStart, StrLen, TextWidth };
				m_aSrvBroadcastLines[m_SrvBroadcastLineCount++] = Line;
			}
		}

		m_SrvBroadcastFontSize = FontSize;
	}
}

void CBroadcast::OnRender()
{
	// server broadcast
	RenderServerBroadcast();

	// client broadcast
	if(m_pClient->m_pScoreboard->Active() || m_pClient->m_pMotd->IsActive())
		return;

	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);

	if(Client()->LocalTime() < m_BroadcastTime)
	{
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, m_BroadcastRenderOffset, 40.0f, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 300*Graphics()->ScreenAspect()-m_BroadcastRenderOffset;
		TextRender()->TextEx(&Cursor, m_aBroadcastText, -1);
	}
}

