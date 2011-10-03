#include <string.h>

#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/generated/client_data.h>

#include <game/version.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>

#include "binds.h"
#include "menus.h"
#include "skins.h"
#include "items.h"

#include <game/client/teecomp.h>

void CMenus::RenderRgbSliders(CUIRect* pMainView, CUIRect* pButton, int &r, int &g, int &b, bool Enabled)
{
	const char *pLabels[] = {"R.", "G.", "B."};
	int *pColorSlider[3] = {&r, &g, &b};
	for(int i=0; i<3; i++)
	{
		CUIRect Text;
		pMainView->HSplitTop(19.0f, pButton, pMainView);
		pButton->VMargin(15.0f, pButton);
		pButton->VSplitLeft(30.0f, &Text, pButton);
		pButton->VSplitRight(5.0f, pButton, 0);
		pButton->HSplitTop(4.0f, 0, pButton);

		if(Enabled)
		{
			float k = (*pColorSlider[i]) / 255.0f;
			k = DoScrollbarH(pColorSlider[i], pButton, k);
			*pColorSlider[i] = (int)(k*255.0f);
		}
		else
			DoScrollbarH(pColorSlider[i], pButton, 0);
		UI()->DoLabel(&Text, pLabels[i], 15.0f, -1);
	}
}

void CMenus::RenderSettingsTeecomp(CUIRect MainView)
{
	CUIRect Button;
	static int s_SettingsPage = 0;

	if(s_SettingsPage != 3)
	{
		MainView.HSplitBottom(20.0f, 0, &Button);
		Button.VSplitLeft(MainView.w/3, &Button, 0);
		static int s_DefaultButton = 0;
		if(DoButton_Menu((void*)&s_DefaultButton, Localize("Reset to defaults"), 0, &Button))
			CTeecompUtils::ResetConfig();

		MainView.HSplitBottom(10.0f, &MainView, &Button);
		UI()->DoLabel(&Button, "http://spl0k.unreal-design.com/teeworlds/", 10.0f, 1);
		MainView.HSplitBottom(10.0f, &MainView, &Button);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf),  Localize("Teeworlds %s with TeeComp %s by spl0k"), GAME_VERSION, TEECOMP_VERSION);
		UI()->DoLabel(&Button, aBuf, 10.0f, 1);
		MainView.HSplitBottom(10.0f, &MainView, 0);
	}

	// render background
	CUIRect Tabbar;
	MainView.HSplitTop(24.0f, &Tabbar, &MainView);
	//RenderTools()->DrawUIRect(&MainView, color_Tabbar_active, CORNER_ALL, 10.0f);

	const char *pTabs[] = { Localize("Skins"), Localize("Stats"), Localize("Scores"), Localize("Misc"), Localize("About") };
	int NumTabs = (int)(sizeof(pTabs)/sizeof(*pTabs));

	for(int i=0; i<NumTabs; i++)
	{
		Tabbar.VSplitLeft(10.0f, &Button, &Tabbar);
		Tabbar.VSplitLeft(80.0f, &Button, &Tabbar);
		
		if(DoButton_Menu(pTabs[i], pTabs[i], s_SettingsPage == i, &Button))
			s_SettingsPage = i;
	}

	if(s_SettingsPage != 4)
		MainView.Margin(10.0f, &MainView);
	
	if(s_SettingsPage == 0)
		RenderSettingsTeecompSkins(MainView);
	else if(s_SettingsPage == 1)
		RenderSettingsTeecompStats(MainView);
	else if(s_SettingsPage == 2)
		RenderSettingsTeecompScores(MainView);
	else if(s_SettingsPage == 3)
		RenderSettingsTeecompMisc(MainView);
	else if(s_SettingsPage == 4)
		RenderSettingsTeecompAbout(MainView);
}

void CMenus::RenderSettingsTeecompSkins(CUIRect MainView)
{
	CUIRect Button, LeftView, RightView;
	MainView.VSplitLeft(MainView.w/2, &LeftView, &RightView);

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcColoredTeesMethod, Localize("Enemy based colors"), g_Config.m_TcColoredTeesMethod, &Button))
		g_Config.m_TcColoredTeesMethod ^= 1;

	// Colors team 1

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("Use DM colors for team %s"), (g_Config.m_TcColoredTeesMethod)?Localize("mates"):"1");
	if(DoButton_CheckBox(&g_Config.m_TcDmColorsTeam1, aBuf, g_Config.m_TcDmColorsTeam1, &Button))
		g_Config.m_TcDmColorsTeam1 ^= 1;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	UI()->DoLabel(&Button, (g_Config.m_TcColoredTeesMethod)?Localize("Team mates"):Localize("Team 1"), 14.0f, -1);
	int r1, g1, b1, r2, g2, b2;
	r1 = g_Config.m_TcColoredTeesTeam1>>16;
	g1 = (g_Config.m_TcColoredTeesTeam1>>8)&0xff;
	b1 = g_Config.m_TcColoredTeesTeam1&0xff;
	RenderRgbSliders(&LeftView, &Button, r1, g1, b1, !g_Config.m_TcDmColorsTeam1);
	g_Config.m_TcColoredTeesTeam1 = (r1<<16) + (g1<<8) + b1;

	const CSkins::CSkin *s = m_pClient->m_pSkins->Get(max(0, m_pClient->m_pSkins->Find(g_Config.m_TcForcedSkin1)));
	CTeeRenderInfo Info;
	if(!g_Config.m_TcDmColorsTeam1)
	{
		Info.m_Texture = s->m_ColorTexture;
		Info.m_ColorBody = vec4(r1/255.0f, g1/255.0f, b1/255.0f, 1.0f);
		Info.m_ColorFeet = vec4(r1/255.0f, g1/255.0f, b1/255.0f, 1.0f);
	}
	else
	{
		Info.m_Texture = s->m_OrgTexture;
		Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	Info.m_Size = UI()->Scale()*50.0f;

	Button.HSplitTop(70.0f, 0, &Button);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1, 0), vec2(Button.x, Button.y+Button.h/2));
	LeftView.HSplitTop(50.0f, 0, &LeftView);

	// Colors team 2

	RightView.HSplitTop(20.0f, 0, &RightView);
	RightView.HSplitTop(20.0f, &Button, &RightView);
	str_format(aBuf, sizeof(aBuf), Localize("Use DM colors for %s"), (g_Config.m_TcColoredTeesMethod)?Localize("enemies"):Localize("team 2"));
	if(DoButton_CheckBox(&g_Config.m_TcDmColorsTeam2, aBuf, g_Config.m_TcDmColorsTeam2, &Button))
		g_Config.m_TcDmColorsTeam2 ^= 1;

	RightView.HSplitTop(20.0f, &Button, &RightView);
	UI()->DoLabel(&Button, (g_Config.m_TcColoredTeesMethod)?Localize("Enemies"):Localize("Team 2"), 14.0f, -1);
	r2 = g_Config.m_TcColoredTeesTeam2>>16;
	g2 = (g_Config.m_TcColoredTeesTeam2>>8)&0xff;
	b2 = g_Config.m_TcColoredTeesTeam2&0xff;
	RenderRgbSliders(&RightView, &Button, r2, g2, b2, !g_Config.m_TcDmColorsTeam2);
	g_Config.m_TcColoredTeesTeam2 = (r2<<16) + (g2<<8) + b2;

	s = m_pClient->m_pSkins->Get(max(0, m_pClient->m_pSkins->Find(g_Config.m_TcForcedSkin2)));
	if(!g_Config.m_TcDmColorsTeam2)
	{
		Info.m_Texture = s->m_ColorTexture;
		Info.m_ColorBody = vec4(r2/255.0f, g2/255.0f, b2/255.0f, 1.0f);
		Info.m_ColorFeet = vec4(r2/255.0f, g2/255.0f, b2/255.0f, 1.0f);
	}
	else
	{
		Info.m_Texture = s->m_OrgTexture;
		Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	Button.HSplitTop(70.0f, 0, &Button);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1, 0), vec2(Button.x, Button.y+Button.h/2));
	RightView.HSplitTop(50.0f, 0, &RightView);

	// Force skins team 1

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcForcedSkinsMethod, Localize("Enemy based skins"), g_Config.m_TcForcedSkinsMethod, &Button))
		g_Config.m_TcForcedSkinsMethod ^= 1;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	str_format(aBuf, sizeof(aBuf), Localize("Force team %s/FFA skins"), (g_Config.m_TcForcedSkinsMethod)?Localize("mates"):"1");
	if(DoButton_CheckBox(&g_Config.m_TcForceSkinTeam1, aBuf, g_Config.m_TcForceSkinTeam1, &Button))
		g_Config.m_TcForceSkinTeam1 ^= 1;

	CUIRect SkinSelection, Scroll;
	LeftView.Margin(10.0f, &SkinSelection);

	SkinSelection.HSplitTop(20, &Button, &SkinSelection);
	RenderTools()->DrawUIRect(&Button, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f); 
	UI()->DoLabel(&Button, Localize("Forced skin"), 14.0f, 0);

	RenderTools()->DrawUIRect(&SkinSelection, vec4(0,0,0,0.15f), 0, 0);
	SkinSelection.VSplitRight(15, &SkinSelection, &Scroll);

	CUIRect List = SkinSelection;
	List.HSplitTop(20, &Button, &List);
	
	int Num = (int)(SkinSelection.h/Button.h);
	static float Scrollvalue = 0;
	static int Scrollbar = 0;
	Scroll.HMargin(5.0f, &Scroll);
	Scrollvalue = DoScrollbarV(&Scrollbar, &Scroll, Scrollvalue);

	int Start = (int)((m_pClient->m_pSkins->Num()-Num)*Scrollvalue);
	if(Start < 0)
		Start = 0;

	for(int i=Start; i<Start+Num && i<m_pClient->m_pSkins->Num(); i++)
	{
		const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);

		str_format(aBuf, sizeof(aBuf), "%s", s->m_aName);
		int Selected = 0;
		if(str_comp(s->m_aName, g_Config.m_TcForcedSkin1) == 0)
			Selected = 1;

		if(DoButton_ListRow(s+m_pClient->m_pSkins->Num(), "", Selected, &Button))
			str_copy(g_Config.m_TcForcedSkin1, s->m_aName, sizeof(g_Config.m_TcForcedSkin1));

		Button.VMargin(5.0f, &Button);
		Button.HSplitTop(1.0f, 0, &Button);
		UI()->DoLabel(&Button, aBuf, 14.0f, -1);

		List.HSplitTop(20.0f, &Button, &List);
	}

	// Forced skin team 2

	RightView.HSplitTop(20.0f, 0, &RightView);
	RightView.HSplitTop(20.0f, &Button, &RightView);
	str_format(aBuf, sizeof(aBuf), Localize("Force %s skins"), (g_Config.m_TcForcedSkinsMethod)?Localize("enemies"):Localize("team 2"));
	if(DoButton_CheckBox(&g_Config.m_TcForceSkinTeam2, aBuf, g_Config.m_TcForceSkinTeam2, &Button))
		g_Config.m_TcForceSkinTeam2 ^= 1;

	RightView.Margin(10.0f, &SkinSelection);

	SkinSelection.HSplitTop(20, &Button, &SkinSelection);
	RenderTools()->DrawUIRect(&Button, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f); 
	UI()->DoLabel(&Button, Localize("Forced skin"), 14.0f, 0);

	RenderTools()->DrawUIRect(&SkinSelection, vec4(0,0,0,0.15f), 0, 0);
	SkinSelection.VSplitRight(15, &SkinSelection, &Scroll);

	List = SkinSelection;
	List.HSplitTop(20, &Button, &List);
	
	Num = (int)(SkinSelection.h/Button.h);
	static float Scrollvalue2 = 0;
	static int Scrollbar2 = 0;
	Scroll.HMargin(5.0f, &Scroll);
	Scrollvalue2 = DoScrollbarV(&Scrollbar2, &Scroll, Scrollvalue2);

	Start = (int)((m_pClient->m_pSkins->Num()-Num)*Scrollvalue2);
	if(Start < 0)
		Start = 0;

	for(int i=Start; i<Start+Num && i<m_pClient->m_pSkins->Num(); i++)
	{
		const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);

		str_format(aBuf, sizeof(aBuf), "%s", s->m_aName);
		int Selected = 0;
		if(str_comp(s->m_aName, g_Config.m_TcForcedSkin2) == 0)
			Selected = 1;

		if(DoButton_ListRow(s+m_pClient->m_pSkins->Num(), "", Selected, &Button))
			str_copy(g_Config.m_TcForcedSkin2, s->m_aName, sizeof(g_Config.m_TcForcedSkin2));

		Button.VMargin(5.0f, &Button);
		Button.HSplitTop(1.0f, 0, &Button);
		UI()->DoLabel(&Button, aBuf, 14.0f, -1);

		List.HSplitTop(20.0f, &Button, &List);
	}
}

void CMenus::RenderSettingsTeecompStats(CUIRect MainView)
{
	CUIRect Button, LeftView;

	MainView.VSplitLeft(MainView.w/2, &LeftView, &MainView);

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%s:",  Localize("Show in global statboard"));
	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	UI()->DoLabel(&Button, aBuf, 16.0f, -1);

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos, Localize("Frags"), g_Config.m_TcStatboardInfos & TC_STATS_FRAGS, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_FRAGS;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+1, Localize("Deaths"), g_Config.m_TcStatboardInfos & TC_STATS_DEATHS, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_DEATHS;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+2, Localize("Suicides"), g_Config.m_TcStatboardInfos & TC_STATS_SUICIDES, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_SUICIDES;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+3, Localize("Ratio"), g_Config.m_TcStatboardInfos & TC_STATS_RATIO, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_RATIO;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+4, Localize("Net score"), g_Config.m_TcStatboardInfos & TC_STATS_NET, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_NET;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+5, Localize("Frags per minute"), g_Config.m_TcStatboardInfos & TC_STATS_FPM, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_FPM;
		
	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+6, Localize("Current spree"), g_Config.m_TcStatboardInfos & TC_STATS_SPREE, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_SPREE;
		
	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+7, Localize("Best spree"), g_Config.m_TcStatboardInfos & TC_STATS_BESTSPREE, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_BESTSPREE;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+9, Localize("Weapons stats"), g_Config.m_TcStatboardInfos & TC_STATS_WEAPS, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_WEAPS;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+8, Localize("Flag grabs"), g_Config.m_TcStatboardInfos & TC_STATS_FLAGGRABS, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_FLAGGRABS;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcStatboardInfos+10, Localize("Flag captures"), g_Config.m_TcStatboardInfos & TC_STATS_FLAGCAPTURES, &Button))
		g_Config.m_TcStatboardInfos ^= TC_STATS_FLAGCAPTURES;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, Localize("Key bindings"), 16.0f, -1);
	char aaBuf[3][32];
	str_format(aaBuf[0], sizeof(aaBuf[0]), "%s:", Localize("Global statboard"));
	str_format(aaBuf[1], sizeof(aaBuf[1]), "%s:", Localize("Player board"));
	str_format(aaBuf[2], sizeof(aaBuf[2]), "%s:", Localize("Next player"));
	CKeyInfo pKeys[] = {{ aaBuf[0], "+stats 1", 0},
		{ aaBuf[1], "+stats 2", 0},
		{ aaBuf[2], "+next_stats", 0}};

	for(int pKeyid=0; pKeyid < KEY_LAST; pKeyid++)
	{
		const char *Bind = m_pClient->m_pBinds->Get(pKeyid);
		if(!Bind[0])
			continue;

		for(unsigned int i=0; i<sizeof(pKeys)/sizeof(CKeyInfo); i++)
			if(strcmp(Bind, pKeys[i].m_pCommand) == 0)
			{
				pKeys[i].m_KeyId = pKeyid;
				break;
			}
	}

	for(unsigned int i=0; i<sizeof(pKeys)/sizeof(CKeyInfo); i++)
		UiDoKeybinder(pKeys[i], &MainView);
}

void CMenus::RenderSettingsTeecompScores(CUIRect MainView)
{
	CUIRect Button, LeftView;

	MainView.VSplitLeft(MainView.w/2, &LeftView, &MainView);

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%s:",  Localize("Show in Scoreboard"));
	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	UI()->DoLabel(&Button, aBuf, 16.0f, -1);

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcScoreboardInfos, Localize("Column Titles"), g_Config.m_TcScoreboardInfos & TC_SCORE_TITLE, &Button))
		g_Config.m_TcScoreboardInfos ^= TC_SCORE_TITLE;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcScoreboardInfos+1, Localize("Country"), g_Config.m_TcScoreboardInfos & TC_SCORE_COUNTRY, &Button))
		g_Config.m_TcScoreboardInfos ^= TC_SCORE_COUNTRY;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcScoreboardInfos+2, Localize("Clan"), g_Config.m_TcScoreboardInfos & TC_SCORE_CLAN, &Button))
		g_Config.m_TcScoreboardInfos ^= TC_SCORE_CLAN;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcScoreboardInfos+3, Localize("Ping"), g_Config.m_TcScoreboardInfos & TC_SCORE_PING, &Button))
		g_Config.m_TcScoreboardInfos ^= TC_SCORE_PING;
}

void CMenus::RenderSettingsTeecompMisc(CUIRect MainView)
{
	CUIRect LeftView, RightView, Button, Lable;

	MainView.VSplitLeft(MainView.w/2, &LeftView, &RightView);

	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, Localize("Name plates"), 16.0f, 0);
	LeftView.HSplitTop(20.0f, 0, &LeftView);
	RightView.HSplitTop(20.0f, 0, &RightView);

	RightView.HSplitTop(20.0f, &Button, &RightView);
	if(DoButton_CheckBox(&g_Config.m_TcNameplateScore, Localize("Show score in name plate"), g_Config.m_TcNameplateScore, &Button))
		g_Config.m_TcNameplateScore ^= 1;

	LeftView.HSplitTop(20.0f, 0, &LeftView);
	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	UI()->DoLabel(&Button, Localize("HUD/Flag"), 16.0f, -1);

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcHudMatch, Localize("Make HUD match tees colors"), g_Config.m_TcHudMatch, &Button))
		g_Config.m_TcHudMatch ^= 1;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcColoredFlags, Localize("Make flags match tees colors"), g_Config.m_TcColoredFlags, &Button))
		g_Config.m_TcColoredFlags ^= 1;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	if(DoButton_CheckBox(&g_Config.m_TcSpeedmeter, Localize("Display speed meter"), g_Config.m_TcSpeedmeter, &Button))
		g_Config.m_TcSpeedmeter ^= 1;

	LeftView.HSplitTop(20.0f, &Button, &LeftView);
	Button.VSplitLeft(15.0f, 0, &Button);
	if(DoButton_CheckBox(&g_Config.m_TcSpeedmeterAccel, Localize("Speed meter show acceleration"), g_Config.m_TcSpeedmeterAccel, &Button))
		g_Config.m_TcSpeedmeterAccel ^= 1;

	RightView.HSplitTop(20.0f, 0, &RightView);
	RightView.HSplitTop(20.0f, &Button, &RightView);
	UI()->DoLabel(&Button, Localize("Others"), 16.0f, -1);

	RightView.HSplitTop(20.0f, &Button, &RightView);
	if(DoButton_CheckBox(&g_Config.m_TcHideCarrying, Localize("Hide flag while carrying it"), g_Config.m_TcHideCarrying, &Button))
		g_Config.m_TcHideCarrying ^= 1;

	RightView.HSplitTop(20.0f, &Button, &RightView);
	if(DoButton_CheckBox(&g_Config.m_TcStatScreenshot, Localize("Automatically take game over stat screenshot"), g_Config.m_TcStatScreenshot, &Button))
		g_Config.m_TcStatScreenshot ^= 1;

	char aBuf[64];
	RightView.HSplitTop(10.0f, 0, &RightView);
	RightView.VSplitLeft(20.0f, 0, &RightView);
	RightView.HSplitTop(20.0f, &Lable, &Button);
	Button.VSplitRight(20.0f, &Button, 0);
	Button.HSplitTop(20.0f, &Button, 0);
	if(g_Config.m_TcStatScreenshotMax)
		str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_TcStatScreenshotMax);
	else
		str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), Localize("no limit"));
	UI()->DoLabelScaled(&Lable, aBuf, 13.0f, -1);
	g_Config.m_TcStatScreenshotMax = static_cast<int>(DoScrollbarH(&g_Config.m_TcStatScreenshotMax, &Button, g_Config.m_TcStatScreenshotMax/1000.0f)*1000.0f+0.1f);
}

void CMenus::RenderSettingsTeecompAbout(CUIRect MainView)
{
	CUIRect Button;
	
	char aBuf[32];
	MainView.HSplitTop(52.0f, &Button, &MainView);
	UI()->DoLabel(&Button, "TeeComp", 48.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitRight(Button.w/3, 0, &Button);
	str_format(aBuf, sizeof(aBuf), "%s %s", Localize("Version"), TEECOMP_VERSION);
	UI()->DoLabel(&Button, aBuf, 14.0f, -1);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitRight(Button.w/3, 0, &Button);
	str_format(aBuf, sizeof(aBuf), "%s %s", Localize("For Teeworlds"), GAME_VERSION);
	UI()->DoLabel(&Button, aBuf, 14.0f, -1);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitRight(Button.w/3, 0, &Button);
	str_format(aBuf, sizeof(aBuf), "%s %s %s", Localize("Compiled"), __DATE__, __TIME__);
	UI()->DoLabel(&Button, aBuf, 14.0f, -1);

	MainView.HSplitTop(40.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, Localize("By Alban 'spl0k' FERON"), 14.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, "http://spl0k.unreal-design.com/", 14.0f, 0);

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(22.0f, &Button, &MainView);
	UI()->DoLabel(&Button, Localize("Special thanks to:"), 16.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, "Sd`", 14.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, "Tho", 14.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, "Eve", 14.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, Localize("some other MonkeyStyle members"), 14.0f, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabel(&Button, Localize("and the Teeworlds.com community"), 14.0f, 0);

	MainView.HSplitBottom(10.0f, &MainView, &Button);
	UI()->DoLabel(&Button, Localize("so you can set while u set while u set options"), 10.0f, -1);
	MainView.HSplitBottom(10.0f, &MainView, &Button);
	UI()->DoLabel(&Button, Localize("Yo dawg I herd you like tabs so we put tabs in yo tabs in yo tabs"), 10.0f, -1);
}
