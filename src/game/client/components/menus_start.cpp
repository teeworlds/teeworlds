/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>

#include <engine/shared/config.h>

#include <game/localization.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include <game/generated/client_data.h>

#include "menus.h"

void CMenus::RenderStartMenu(CUIRect MainView)
{
	MainView.VMargin(160.0f, &MainView);
	MainView.HMargin(145.0f, &MainView);
	RenderTools()->DrawUIRect(&MainView, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);

	MainView.Margin(5.0f, &MainView);

	CUIRect Button;

	MainView.HSplitTop(40.0f, &Button, &MainView);
	static int s_PlayButton = 0;
	if(DoButton_MenuImage(&s_PlayButton, Localize("Play Game"), 0, &Button, "play_game", 15.0f, 0.5f))
		m_MenuPage = MENU_PAGE;

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(40.0f, &Button, &MainView);
	static int s_MapEditorButton = 0;
	if(DoButton_MenuImage(&s_MapEditorButton, Localize("Map Editor"), 0, &Button, "editor", 15.0f, 0.5f))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(40.0f, &Button, &MainView);
	static int s_DemoButton = 0;
	if(DoButton_MenuImage(&s_DemoButton, Localize("Demos"), 0, &Button, "demos", 15.0f, 0.5f))
	{
	}

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(40.0f, &Button, &MainView);
	static int s_LocalServerButton = 0;
	if(DoButton_MenuImage(&s_LocalServerButton, Localize("Create Local Server"), 0, &Button, "local_server", 15.0f, 0.5f))
	{
	}

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(40.0f, &Button, &MainView);
	static int s_SettingsButton = 0;
	if(DoButton_MenuImage(&s_SettingsButton, Localize("Settings"), 0, &Button, "settings", 15.0f, 0.5f))
	{
	}

	MainView.HSplitBottom(45.0f, &MainView, &Button);
	static int s_QuitButton = 0;
	if(DoButton_Menu(&s_QuitButton, Localize("Quit Game"), 0, &Button, 15.0f, 0.5f))
		m_Popup = POPUP_QUIT;
}

void CMenus::RenderLogo(CUIRect MainView)
{
	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(MainView.w/2-170, 70, 330, 50);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}