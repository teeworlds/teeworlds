#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/layers.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>

//#include "controls.h"
//#include "camera.h"
#include "debughud.h"

void CDebugHud::RenderNetCorrections()
{
	if(!g_Config.m_Debug || !m_pClient->m_Snap.m_pLocalCharacter || !m_pClient->m_Snap.m_pLocalPrevCharacter)
		return;

	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);
	
	/*float speed = distance(vec2(netobjects.local_prev_character->x, netobjects.local_prev_character->y),
		vec2(netobjects.local_character->x, netobjects.local_character->y));*/

/*
	float velspeed = length(vec2(gameclient.snap.local_character->m_VelX/256.0f, gameclient.snap.local_character->m_VelY/256.0f))*50;
	
	float ramp = velocity_ramp(velspeed, gameclient.tuning.velramp_start, gameclient.tuning.velramp_range, gameclient.tuning.velramp_curvature);
	
	char buf[512];
	str_format(buf, sizeof(buf), "%.0f\n%.0f\n%.2f\n%d %s\n%d %d",
		velspeed, velspeed*ramp, ramp,
		netobj_num_corrections(), netobj_corrected_on(),
		gameclient.snap.local_character->m_X,
		gameclient.snap.local_character->m_Y
	);
	TextRender()->Text(0, 150, 50, 12, buf, -1);*/
}

void CDebugHud::RenderTuning()
{
	// render tuning debugging
	if(!g_Config.m_DbgTuning)
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
		
		str_format(aBuf, sizeof(aBuf), "%.2f", Standard);
		x += 20.0f;
		w = TextRender()->TextWidth(0, 5, aBuf, -1);
		TextRender()->Text(0x0, x-w, y+Count*6, 5, aBuf, -1);

		str_format(aBuf, sizeof(aBuf), "%.2f", Current);
		x += 20.0f;
		w = TextRender()->TextWidth(0, 5, aBuf, -1);
		TextRender()->Text(0x0, x-w, y+Count*6, 5, aBuf, -1);

		x += 5.0f;
		TextRender()->Text(0x0, x, y+Count*6, 5, m_pClient->m_Tuning.m_apNames[i], -1);
		
		Count++;
	}
	
	y = y+Count*6;
	
	Graphics()->TextureSet(-1);
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
