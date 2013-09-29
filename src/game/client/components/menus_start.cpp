/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <engine/shared/config.h>

#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include <game/generated/client_data.h>

#include "menus.h"

void CMenus::RenderStartMenu(CUIRect MainView)
{
	CUIRect TopMenu, BottomMenu;
	MainView.VMargin(MainView.w/2-190.0f, &TopMenu);
	TopMenu.HSplitTop(365.0f, &TopMenu, &BottomMenu);
	//TopMenu.HSplitBottom(145.0f, &TopMenu, 0);
	RenderTools()->DrawUIRect4(&TopMenu, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 10.0f);

	TopMenu.HSplitTop(145.0f, 0, &TopMenu);

	CUIRect Button;

	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static int s_SettingsButton = 0;
	if(g_Config.m_ClShowStartMenuImages)
	{
		if(DoButtonStart(&s_SettingsButton, Localize("Settings"), &Button, "settings", 10.0f, 0.5f))
		{
			m_MenuPage = PAGE_SETTINGS;
			m_ResetFades = FADE_ALL;
		}
	}
	else
	{
		if(DoButtonDefault(&s_SettingsButton, Localize("Settings"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		{
			m_MenuPage = PAGE_SETTINGS;
			m_ResetFades = FADE_ALL;
		}
	}

	/*TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Bottom);
	static int s_LocalServerButton = 0;
	if(g_Config.m_ClShowStartMenuImages)
	{
		if(DoButtonStart(&s_LocalServerButton, Localize("Local server"), &Button, "local_server", 10.0f, 0.5f))
		{
		}
	}
	else
	{
		if(DoButtonDefault(&s_LocalServerButton, Localize("Local server"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		{
		}
	}*/

	TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static int s_DemoButton = 0;
	if(g_Config.m_ClShowStartMenuImages)
	{
		if(DoButtonStart(&s_DemoButton, Localize("Demos"), &Button, "demos", 10.0f, 0.5f))
		{
			m_MenuPage = PAGE_DEMOS;
			m_ResetFades = FADE_ALL;
			DemolistPopulate();
			DemolistOnUpdate(false);
		}
	}
	else
	{
		if(DoButtonDefault(&s_DemoButton, Localize("Demos"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		{
			m_MenuPage = PAGE_DEMOS;
			m_ResetFades = FADE_ALL;
			DemolistPopulate();
			DemolistOnUpdate(false);
		}
	}

	TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static int s_MapEditorButton = 0;
	if(g_Config.m_ClShowStartMenuImages)
	{
		if(DoButtonStart(&s_MapEditorButton, Localize("Editor"), &Button, "editor", 10.0f, 0.5f))
		{
			g_Config.m_ClEditor = 1;
			Input()->MouseModeRelative();
		}
	}
	else
	{
		if(DoButtonDefault(&s_MapEditorButton, Localize("Editor"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		{
			g_Config.m_ClEditor = 1;
			Input()->MouseModeRelative();
		}
	}

	TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static int s_PlayButton = 0;
	if(g_Config.m_ClShowStartMenuImages)
	{
		if(DoButtonStart(&s_PlayButton, Localize("Play"), &Button, "play_game", 10.0f, 0.5f))
		{
			m_MenuPage = g_Config.m_UiBrowserPage;
			m_ResetFades = FADE_ALL;
		}
	}
	else
	{
		if(DoButtonDefault(&s_PlayButton, Localize("Play"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		{
			m_MenuPage = g_Config.m_UiBrowserPage;
			m_ResetFades = FADE_ALL;
		}
	}

	BottomMenu.HSplitTop(90.0f, 0, &BottomMenu);
	RenderTools()->DrawUIRect4(&BottomMenu, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 10.0f);

	BottomMenu.HSplitTop(40.0f, &Button, &TopMenu);
	static int s_QuitButton = 0;
	if(DoButtonDefault(&s_QuitButton, Localize("Quit"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		m_Popup = POPUP_QUIT;

	// render version
	CUIRect Version;
	MainView.HSplitBottom(50.0f, 0, &Version);
	Version.VMargin(50.0f, &Version);
	char aBuf[64];
	if(str_comp(Client()->LatestVersion(), "0") != 0)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Teeworlds %s is out! Download it at www.teeworlds.com!"), Client()->LatestVersion());
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		UI()->DoLabelScaled(&Version, aBuf, 14.0f, 0);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	UI()->DoLabelScaled(&Version, GAME_VERSION, 14.0f, 1);
}

void CMenus::RenderLogo(CUIRect MainView)
{
	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(MainView.w/2-140, 60, 280, 70);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}
