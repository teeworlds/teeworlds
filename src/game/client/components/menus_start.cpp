/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/textrender.h>

#include <engine/shared/config.h>

#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include <generated/client_data.h>

#include "menus.h"

void CMenus::RenderStartMenu(CUIRect MainView)
{
	// render logo
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(MainView.w/2-140, 60, 280, 70);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	CUIRect TopMenu, BottomMenu;
	MainView.VMargin(MainView.w/2-190.0f, &TopMenu);
	TopMenu.HSplitTop(365.0f, &TopMenu, &BottomMenu);
	//TopMenu.HSplitBottom(145.0f, &TopMenu, 0);
	RenderTools()->DrawUIRect4(&TopMenu, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), CUI::CORNER_B, 10.0f);

	TopMenu.HSplitTop(145.0f, 0, &TopMenu);

	CUIRect Button;
	int NewPage = -1;

	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static CButtonContainer s_SettingsButton;
	if(DoButton_Menu(&s_SettingsButton, Localize("Settings"), 0, &Button, g_Config.m_ClShowStartMenuImages ? "settings" : 0, CUI::CORNER_ALL, 10.0f, 0.5f) || CheckHotKey(KEY_S))
		NewPage = PAGE_SETTINGS;
	
	/*TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Bottom);
	static int s_LocalServerButton = 0;
	if(g_Config.m_ClShowStartMenuImages)
	{
		if(DoButton_MenuImage(&s_LocalServerButton, Localize("Local server"), 0, &Button, "local_server", 10.0f, 0.5f))
		{
		}
	}
	else
	{
		if(DoButton_Menu(&s_LocalServerButton, Localize("Local server"), 0, &Button, CUI::CORNER_ALL, 10.0f, 0.5f))
		{
		}
	}*/

	TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static CButtonContainer s_DemoButton;
	if(DoButton_Menu(&s_DemoButton, Localize("Demos"), 0, &Button, g_Config.m_ClShowStartMenuImages ? "demos" : 0, CUI::CORNER_ALL, 10.0f, 0.5f) || CheckHotKey(KEY_D))
	{
		NewPage = PAGE_DEMOS;
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	static bool EditorHotkeyWasPressed = true;
	static float EditorHotKeyChecktime = 0;
	TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static CButtonContainer s_MapEditorButton;
	if(DoButton_Menu(&s_MapEditorButton, Localize("Editor"), 0, &Button, g_Config.m_ClShowStartMenuImages ? "editor" : 0, CUI::CORNER_ALL, 10.0f, 0.5f) || (!EditorHotkeyWasPressed && Client()->LocalTime() - EditorHotKeyChecktime < 0.1f && CheckHotKey(KEY_E)))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
		EditorHotkeyWasPressed = true;
	}
	if(!Input()->KeyIsPressed(KEY_E))
	{
		EditorHotkeyWasPressed = false;
		EditorHotKeyChecktime = Client()->LocalTime();
	}

	TopMenu.HSplitBottom(5.0f, &TopMenu, 0); // little space
	TopMenu.HSplitBottom(40.0f, &TopMenu, &Button);
	static CButtonContainer s_PlayButton;
	if(DoButton_Menu(&s_PlayButton, Localize("Play"), 0, &Button, g_Config.m_ClShowStartMenuImages ? "play_game" : 0, CUI::CORNER_ALL, 10.0f, 0.5f) || m_EnterPressed || CheckHotKey(KEY_P))
		NewPage = g_Config.m_UiBrowserPage;
	
	BottomMenu.HSplitTop(90.0f, 0, &BottomMenu);
	RenderTools()->DrawUIRect4(&BottomMenu, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 10.0f);

	BottomMenu.HSplitTop(40.0f, &Button, &TopMenu);
	static CButtonContainer s_QuitButton;
	if(DoButton_Menu(&s_QuitButton, Localize("Quit"), 0, &Button, 0, CUI::CORNER_ALL, 10.0f, 0.5f) || m_EscapePressed || CheckHotKey(KEY_Q))
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
		UI()->DoLabel(&Version, aBuf, 14.0f, CUI::ALIGN_CENTER);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	UI()->DoLabel(&Version, GAME_RELEASE_VERSION, 14.0f, CUI::ALIGN_RIGHT);

	if(NewPage != -1)
		SetMenuPage(NewPage);
}

void CMenus::RenderLogo(CUIRect MainView)
{
	
}
