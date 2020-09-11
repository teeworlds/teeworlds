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

	// TODO: ADDBACK: Net Corrections
}

void CDebugHud::RenderTuning()
{
	// render tuning debugging
	if(!Config()->m_DbgTuning)
		return;

	CTuningParams StandardTuning;

	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);

	float y = 50.0f;
	int Count = 0;
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

		float w;
		float x = 5.0f;

		// TODO: ADDBACK: render tunning

		Count++;
	}

	y = y+Count*6;

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
