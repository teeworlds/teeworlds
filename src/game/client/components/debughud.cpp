/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/layers.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>

//#include "controls.h"
//#include "camera.h"
#include "debughud.h"

void CDebugHud::RenderNetCorrections()
{
	if(!Config()->m_Debug || Config()->m_DbgGraphs || !m_pClient->m_Snap.m_pLocalCharacter || !m_pClient->m_Snap.m_pLocalPrevCharacter)
		return;

	float Width = 300*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, 300);

	/*float speed = distance(vec2(netobjects.local_prev_character->x, netobjects.local_prev_character->y),
		vec2(netobjects.local_character->x, netobjects.local_character->y));*/

	float Velspeed = length(vec2(m_pClient->m_Snap.m_pLocalCharacter->m_VelX/256.0f, m_pClient->m_Snap.m_pLocalCharacter->m_VelY/256.0f))*50;
	float Ramp = VelocityRamp(Velspeed, m_pClient->m_Tuning.m_VelrampStart, m_pClient->m_Tuning.m_VelrampRange, m_pClient->m_Tuning.m_VelrampCurvature);

	const char *paStrings[] = {"velspeed:", "velspeed*ramp:", "ramp:", "Pos", " x:", " y:", "netmsg failed on:", "netobj num failures:", "netobj failed on:"};
	const int Num = sizeof(paStrings)/sizeof(char *);

	static CTextCursor s_CursorLabels(5.0f);
	s_CursorLabels.MoveTo(Width-100.0f, 50.0f);
	s_CursorLabels.m_MaxLines = -1;
	s_CursorLabels.m_LineSpacing = 1.0f;
	s_CursorLabels.Reset(0);

	static CTextCursor s_CursorValues(5.0f);
	s_CursorValues.MoveTo(Width-10.0f, 50.0f);
	s_CursorValues.m_MaxLines = -1;
	s_CursorValues.m_LineSpacing = 1.0f;
	s_CursorValues.m_Align = TEXTALIGN_TR;
	s_CursorValues.Reset();

	for(int i = 0; i < Num; ++i)
	{
		TextRender()->TextDeferred(&s_CursorLabels, paStrings[i], -1);
		TextRender()->TextNewline(&s_CursorLabels);
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%.0f", Velspeed/32);
	TextRender()->TextDeferred(&s_CursorValues, aBuf, -1);
	TextRender()->TextNewline(&s_CursorValues);

	str_format(aBuf, sizeof(aBuf), "%.0f", Velspeed/32*Ramp);
	TextRender()->TextDeferred(&s_CursorValues, aBuf, -1);
	TextRender()->TextNewline(&s_CursorValues);

	str_format(aBuf, sizeof(aBuf), "%.2f", Ramp);
	TextRender()->TextDeferred(&s_CursorValues, aBuf, -1);
	TextRender()->TextNewline(&s_CursorValues);
	TextRender()->TextNewline(&s_CursorValues);

	str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pLocalCharacter->m_X/32);
	TextRender()->TextDeferred(&s_CursorValues, aBuf, -1);
	TextRender()->TextNewline(&s_CursorValues);

	str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pLocalCharacter->m_Y/32);
	TextRender()->TextDeferred(&s_CursorValues, aBuf, -1);
	TextRender()->TextNewline(&s_CursorValues);
	

	TextRender()->TextDeferred(&s_CursorValues, m_pClient->NetmsgFailedOn(), -1);
	TextRender()->TextNewline(&s_CursorValues);

	str_format(aBuf, sizeof(aBuf), "%d", m_pClient->NetobjNumFailures());
	TextRender()->TextDeferred(&s_CursorValues, aBuf, -1);
	TextRender()->TextNewline(&s_CursorValues);

	TextRender()->TextDeferred(&s_CursorValues, m_pClient->NetobjFailedOn(), -1);

	TextRender()->DrawTextOutlined(&s_CursorLabels);
	TextRender()->DrawTextOutlined(&s_CursorValues);
}

void CDebugHud::RenderTuning()
{
	// render tuning debugging
	if(!Config()->m_DbgTuning)
		return;

	CTuningParams StandardTuning;

	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);

	static CTextCursor s_CursorStandard(5.0f, 25.0f, 50.0f);
	static CTextCursor s_CursorCurrent(5.0f, 50.0f, 50.0f);
	static CTextCursor s_CursorLabels(5.0f, 55.0f, 50.0f);
	s_CursorStandard.m_MaxLines = -1;
	s_CursorStandard.m_LineSpacing = 1.0f;
	s_CursorStandard.m_Align = TEXTALIGN_TR;
	s_CursorStandard.Reset(0);
	s_CursorCurrent.m_MaxLines = -1;
	s_CursorCurrent.m_LineSpacing = 1.0f;
	s_CursorCurrent.m_Align = TEXTALIGN_TR;
	s_CursorCurrent.Reset();
	s_CursorLabels.m_MaxLines = -1;
	s_CursorLabels.m_LineSpacing = 1.0f;
	s_CursorLabels.Reset(0);

	for(int i = 0; i < m_pClient->m_Tuning.Num(); i++)
	{
		char aBuf[128];
		float Current, Standard;
		m_pClient->m_Tuning.Get(i, &Current);
		StandardTuning.Get(i, &Standard);

		if(Standard == Current)
			TextRender()->TextColor(1,1,1,1.0f);
		else
			TextRender()->TextColor(1,0.25f,0.25f,1.0f);

		str_format(aBuf, sizeof(aBuf), "%.2f", Standard);
		TextRender()->TextDeferred(&s_CursorStandard, aBuf, -1);

		str_format(aBuf, sizeof(aBuf), "%.2f", Current);
		TextRender()->TextDeferred(&s_CursorCurrent, aBuf, -1);

		TextRender()->TextDeferred(&s_CursorLabels, m_pClient->m_Tuning.GetName(i), -1);

		TextRender()->TextNewline(&s_CursorStandard);
		TextRender()->TextNewline(&s_CursorCurrent);
		TextRender()->TextNewline(&s_CursorLabels);
	}

	TextRender()->DrawTextOutlined(&s_CursorStandard);
	TextRender()->DrawTextOutlined(&s_CursorCurrent);
	TextRender()->DrawTextOutlined(&s_CursorLabels);

	float y = 50.0f+m_pClient->m_Tuning.Num()*6;

	Graphics()->TextureClear();
	Graphics()->BlendNormal();
	Graphics()->LinesBegin();
	float Height = 50.0f;
	float pv = 1;
	IGraphics::CLineItem Array[100];
	for(int i = 0; i < 100; i++)
	{
		float Speed = i/100.0f * 3000;
		float Ramp = VelocityRamp(Speed, m_pClient->m_Tuning.m_VelrampStart, m_pClient->m_Tuning.m_VelrampRange, m_pClient->m_Tuning.m_VelrampCurvature);
		float RampedSpeed = (Speed * Ramp)/1000.0f;
		Array[i] = IGraphics::CLineItem((i-1)*2, y+Height-pv*Height, i*2, y+Height-RampedSpeed*Height);
		//Graphics()->LinesDraw((i-1)*2, 200, i*2, 200);
		pv = RampedSpeed;
	}
	Graphics()->LinesDraw(Array, 100);
	Graphics()->LinesEnd();
	TextRender()->TextColor(1,1,1,1);
}

void CDebugHud::OnRender()
{
	RenderTuning();
	RenderNetCorrections();
}
