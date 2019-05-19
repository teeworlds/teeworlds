/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>

#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/render.h>

#include "notifications.h"

CNotifications::CNotifications()
{
	m_SoundToggleTime = -99.0f;
}

void CNotifications::OnConsoleInit()
{
	IConsole* pConsole = Kernel()->RequestInterface<IConsole>();

	pConsole->Register("snd_toggle", "", CFGFLAG_CLIENT, Con_SndToggle, this, "Toggle sounds on and off");
}

void CNotifications::Con_SndToggle(IConsole::IResult *pResult, void *pUserData)
{
	CNotifications *pSelf = (CNotifications *)pUserData;

	g_Config.m_SndEnable ^= 1;
	pSelf->m_SoundToggleTime = pSelf->Client()->LocalTime();
}

void CNotifications::RenderSoundNotification()
{
	const float Height = 300.0f;
	const float Width = Height*Graphics()->ScreenAspect();
	const float ItemHeight = 20.f;
	const float ItemWidth = 20.f;
	const float DisplayTime = 1.5f; // includes FadeTime
	const float FadeTime = 0.6f;
	const float RemainingDisplayTime = max(0.0f, m_SoundToggleTime + DisplayTime - Client()->LocalTime());

	if(RemainingDisplayTime == 0.0f)
		return;

	Graphics()->MapScreen(0.0f, 0.0f, Width, Height);

	CUIRect Area;
	Area.x = (Width-ItemWidth)/2.f;
	Area.y = (Height/5.f) - (ItemHeight/2.f);
	Area.w = ItemWidth;
	Area.h = ItemHeight;

	const float Fade = min(1.0f, RemainingDisplayTime / FadeTime); // 0.0 ≤ Fade ≤ 1.0

	vec4 Color = (g_Config.m_SndEnable == 0) ? vec4(1.f/0xff*0xf9, 1.f/0xff*0x2b, 1.f/0xff*0x2b, 0.55f) : vec4(1.f/0xff*0x2b, 1.f/0xff*0xf9, 1.f/0xff*0x2b, 0.55f);
	Color = mix(vec4(Color.r, Color.g, Color.b, 0.0f), Color, 0.8*Fade);
	RenderTools()->DrawUIRect(&Area, Color, CUI::CORNER_ALL, 3.0f);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_SOUNDICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f*Fade, 1.0f*Fade, 1.0f*Fade, 1.0f*Fade);
	RenderTools()->SelectSprite(g_Config.m_SndEnable ? SPRITE_SOUNDICON_ON : SPRITE_SOUNDICON_MUTE);
	IGraphics::CQuadItem QuadItem(Area.x, Area.y, Area.w, Area.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CNotifications::OnRender()
{
	RenderSoundNotification();
}
