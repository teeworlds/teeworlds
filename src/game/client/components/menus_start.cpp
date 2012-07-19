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
	CUIRect Menu;
	MainView.VMargin(MainView.w/2-190.0f, &Menu);
	Menu.HMargin(145.0f, &Menu);
	RenderTools()->DrawUIRect(&Menu, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 10.0f);

	Menu.Margin(5.0f, &Menu);

	CUIRect Button;

	Menu.HSplitTop(40.0f, &Button, &Menu);
	static int s_PlayButton = 0;
	if(DoButton_MenuImage(&s_PlayButton, Localize("Play Game"), 0, &Button, "play_game", 10.0f, 0.5f))
		m_MenuPage = g_Config.m_UiBrowserPage;

	Menu.HSplitTop(5.0f, 0, &Menu); // little space
	Menu.HSplitTop(40.0f, &Button, &Menu);
	static int s_MapEditorButton = 0;
	if(DoButton_MenuImage(&s_MapEditorButton, Localize("Map Editor"), 0, &Button, "editor", 10.0f, 0.5f))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}

	Menu.HSplitTop(5.0f, 0, &Menu); // little space
	Menu.HSplitTop(40.0f, &Button, &Menu);
	static int s_DemoButton = 0;
	if(DoButton_MenuImage(&s_DemoButton, Localize("Demos"), 0, &Button, "demos", 10.0f, 0.5f))
	{
		m_MenuPage = PAGE_DEMOS;
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	Menu.HSplitTop(5.0f, 0, &Menu); // little space
	Menu.HSplitTop(40.0f, &Button, &Menu);
	static int s_LocalServerButton = 0;
	if(DoButton_MenuImage(&s_LocalServerButton, Localize("Create Local Server"), 0, &Button, "local_server", 10.0f, 0.5f))
	{
	}

	Menu.HSplitTop(5.0f, 0, &Menu); // little space
	Menu.HSplitTop(40.0f, &Button, &Menu);
	static int s_SettingsButton = 0;
	if(DoButton_MenuImage(&s_SettingsButton, Localize("Settings"), 0, &Button, "settings", 10.0f, 0.5f))
	{
		m_MenuPage = PAGE_SETTINGS;
	}

	Menu.HSplitBottom(45.0f, &Menu, &Button);
	static int s_QuitButton = 0;
	if(DoButton_Menu(&s_QuitButton, Localize("Quit Game"), 0, &Button, 15.0f, 0.5f))
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
	IGraphics::CQuadItem QuadItem(MainView.w/2-140, 70, 280, 70);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}
