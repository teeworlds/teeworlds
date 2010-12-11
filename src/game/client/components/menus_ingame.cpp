/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/demo.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "menus.h"
#include "motd.h"
#include "voting.h"

void CMenus::RenderGame(CUIRect MainView)
{
	CUIRect Button;
	//CUIRect votearea;
	MainView.HSplitTop(45.0f, &MainView, 0);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(25.0f, &MainView, 0);
	MainView.VMargin(10.0f, &MainView);
	
	MainView.VSplitRight(120.0f, &MainView, &Button);
	static int s_DisconnectButton = 0;
	if(DoButton_Menu(&s_DisconnectButton, Localize("Disconnect"), 0, &Button))
		Client()->Disconnect();

	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pGameobj)
	{
		if(m_pClient->m_Snap.m_pLocalInfo->m_Team != -1)
		{
			MainView.VSplitLeft(10.0f, &Button, &MainView);
			MainView.VSplitLeft(120.0f, &Button, &MainView);
			static int s_SpectateButton = 0;
			if(DoButton_Menu(&s_SpectateButton, Localize("Spectate"), 0, &Button))
			{
				m_pClient->SendSwitchTeam(-1);
				SetActive(false);
			}
		}
		
		if(m_pClient->m_Snap.m_pGameobj->m_Flags & GAMEFLAG_TEAMS)
		{
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != 0)
			{
				MainView.VSplitLeft(10.0f, &Button, &MainView);
				MainView.VSplitLeft(120.0f, &Button, &MainView);
				static int s_SpectateButton = 0;
				if(DoButton_Menu(&s_SpectateButton, Localize("Join red"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(0);
					SetActive(false);
				}
			}

			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != 1)
			{
				MainView.VSplitLeft(10.0f, &Button, &MainView);
				MainView.VSplitLeft(120.0f, &Button, &MainView);
				static int s_SpectateButton = 0;
				if(DoButton_Menu(&s_SpectateButton, Localize("Join blue"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(1);
					SetActive(false);
				}
			}
		}
		else
		{
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != 0)
			{
				MainView.VSplitLeft(10.0f, &Button, &MainView);
				MainView.VSplitLeft(120.0f, &Button, &MainView);
				static int s_SpectateButton = 0;
				if(DoButton_Menu(&s_SpectateButton, Localize("Join game"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(0);
					SetActive(false);
				}
			}
		}
	}

	MainView.VSplitLeft(100.0f, &Button, &MainView);
	MainView.VSplitLeft(150.0f, &Button, &MainView);

	static int s_DemoButton = 0;
	bool Recording = DemoRecorder()->IsRecording();
	if(DoButton_Menu(&s_DemoButton, Localize(Recording ? "Stop record" : "Record demo"), 0, &Button))	// Localize("Stop record");Localize("Record demo");
	{
		if(!Recording)
			Client()->DemoRecorder_Start("demo", true);
		else
			Client()->DemoRecorder_Stop();
	}
	
	/*
	CUIRect bars;
	votearea.HSplitTop(10.0f, 0, &votearea);
	votearea.HSplitTop(25.0f + 10.0f*3 + 25.0f, &votearea, &bars);

	RenderTools()->DrawUIRect(&votearea, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	votearea.VMargin(20.0f, &votearea);
	votearea.HMargin(10.0f, &votearea);

	votearea.HSplitBottom(35.0f, &votearea, &bars);

	if(gameclient.voting->is_voting())
	{
		// do yes button
		votearea.VSplitLeft(50.0f, &button, &votearea);
		static int yes_button = 0;
		if(UI()->DoButton(&yes_button, "Yes", 0, &button, ui_draw_menu_button, 0))
			gameclient.voting->vote(1);

		// do no button
		votearea.VSplitLeft(5.0f, 0, &votearea);
		votearea.VSplitLeft(50.0f, &button, &votearea);
		static int no_button = 0;
		if(UI()->DoButton(&no_button, "No", 0, &button, ui_draw_menu_button, 0))
			gameclient.voting->vote(-1);
		
		// do time left
		votearea.VSplitRight(50.0f, &votearea, &button);
		char buf[256];
		str_format(buf, sizeof(buf), "%d", gameclient.voting->seconds_left());
		UI()->DoLabel(&button, buf, 24.0f, 0);

		// do description and command
		votearea.VSplitLeft(5.0f, 0, &votearea);
		UI()->DoLabel(&votearea, gameclient.voting->vote_description(), 14.0f, -1);
		votearea.HSplitTop(16.0f, 0, &votearea);
		UI()->DoLabel(&votearea, gameclient.voting->vote_command(), 10.0f, -1);

		// do bars
		bars.HSplitTop(10.0f, 0, &bars);
		bars.HMargin(5.0f, &bars);
		
		gameclient.voting->render_bars(bars, true);

	}		
	else
	{
		UI()->DoLabel(&votearea, "No vote in progress", 18.0f, -1);
	}*/
}

void CMenus::RenderServerInfo(CUIRect MainView)
{
	// fetch server info
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	
	if(!m_pClient->m_Snap.m_pLocalInfo)
		return;
	
	// count players for server info-box
	int NumPlayers = 0;
	for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			NumPlayers++;
		}
	}

	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	
	CUIRect View, ServerInfo, GameInfo, Motd;
	
	float x = 0.0f;
	float y = 0.0f;
	
	char aBuf[1024];
	
	// set view to use for all sub-modules
	MainView.Margin(10.0f, &View);
	
	// serverinfo
	View.HSplitTop(View.h/2-5.0f, &ServerInfo, &Motd);
	ServerInfo.VSplitLeft(View.w/2-5.0f, &ServerInfo, &GameInfo);
	RenderTools()->DrawUIRect(&ServerInfo, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
	
	ServerInfo.Margin(5.0f, &ServerInfo);
	
	x = 5.0f;
	y = 0.0f;
	
	TextRender()->Text(0, ServerInfo.x+x, ServerInfo.y+y, 32, Localize("Server info"), 250);
	y += 32.0f+5.0f;
	
	mem_zero(aBuf, sizeof(aBuf));
	str_format(
		aBuf,
		sizeof(aBuf),
		"%s\n\n"
		"%s: %s\n"
		"%s: %d\n"
		"%s: %s\n"
		"%s: %s\n",
		CurrentServerInfo.m_aName,
		Localize("Address"), g_Config.m_UiServerAddress,
		Localize("Ping"), m_pClient->m_Snap.m_pLocalInfo->m_Latency,
		Localize("Version"), CurrentServerInfo.m_aVersion,
		Localize("Password"), CurrentServerInfo.m_Flags &1 ? Localize("Yes") : Localize("No")
	);
	
	TextRender()->Text(0, ServerInfo.x+x, ServerInfo.y+y, 20, aBuf, 250);
	
	{
		CUIRect Button;
		int IsFavorite = ServerBrowser()->IsFavorite(CurrentServerInfo.m_NetAddr);
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		static int s_AddFavButton = 0;
		if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), IsFavorite, &Button))
		{
			if(IsFavorite)
				ServerBrowser()->RemoveFavorite(CurrentServerInfo.m_NetAddr);
			else
				ServerBrowser()->AddFavorite(CurrentServerInfo.m_NetAddr);
		}
	}
	
	// gameinfo
	GameInfo.VSplitLeft(10.0f, 0x0, &GameInfo);
	RenderTools()->DrawUIRect(&GameInfo, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
	
	GameInfo.Margin(5.0f, &GameInfo);
	
	x = 5.0f;
	y = 0.0f;
	
	TextRender()->Text(0, GameInfo.x+x, GameInfo.y+y, 32, Localize("Game info"), 250);
	y += 32.0f+5.0f;
	
	mem_zero(aBuf, sizeof(aBuf));
	str_format(
		aBuf,
		sizeof(aBuf),
		"\n\n"
		"%s: %s\n"
		"%s: %s\n"
		"%s: %d\n"
		"%s: %d\n"
		"\n"
		"%s: %d/%d\n",
		Localize("Game type"), CurrentServerInfo.m_aGameType,
		Localize("Map"), CurrentServerInfo.m_aMap,
		Localize("Score limit"), m_pClient->m_Snap.m_pGameobj->m_ScoreLimit,
		Localize("Time limit"), m_pClient->m_Snap.m_pGameobj->m_TimeLimit,
		Localize("Players"), m_pClient->m_Snap.m_NumPlayers, CurrentServerInfo.m_MaxPlayers
	);
	TextRender()->Text(0, GameInfo.x+x, GameInfo.y+y, 20, aBuf, 250);
	
	// motd
	Motd.HSplitTop(10.0f, 0, &Motd);
	RenderTools()->DrawUIRect(&Motd, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
	Motd.Margin(5.0f, &Motd);
	y = 0.0f;
	x = 5.0f;
	TextRender()->Text(0, Motd.x+x, Motd.y+y, 32, Localize("MOTD"), -1);
	y += 32.0f+5.0f;
	TextRender()->Text(0, Motd.x+x, Motd.y+y, 16, m_pClient->m_pMotd->m_aServerMotd, (int)Motd.w);
}

static const char *FormatCommand(const char *pCmd)
{
	return pCmd;
}

void CMenus::RenderServerControlServer(CUIRect MainView)
{
	int NumOptions = 0;
	for(CVoting::CVoteOption *pOption = m_pClient->m_pVoting->m_pFirst; pOption; pOption = pOption->m_pNext)
		NumOptions++;

	static int s_VoteList = 0;
	static float s_ScrollValue = 0;
	CUIRect List = MainView;
	UiDoListboxStart(&s_VoteList, &List, 24.0f, Localize("Settings"), "", NumOptions, 1, m_CallvoteSelectedOption, s_ScrollValue);
	
	for(CVoting::CVoteOption *pOption = m_pClient->m_pVoting->m_pFirst; pOption; pOption = pOption->m_pNext)
	{
		CListboxItem Item = UiDoListboxNextItem(pOption);
		
		if(Item.m_Visible)
			UI()->DoLabel(&Item.m_Rect, FormatCommand(pOption->m_aCommand), 16.0f, -1);
	}
	
	m_CallvoteSelectedOption = UiDoListboxEnd(&s_ScrollValue, 0);
}

void CMenus::RenderServerControlKick(CUIRect MainView)
{
	int NumOptions = 0;
	int Selected = -1;
	static int aPlayerIDs[MAX_CLIENTS];
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i])
			continue;
		if(m_CallvoteSelectedPlayer == i)
			Selected = NumOptions;
		aPlayerIDs[NumOptions++] = i;
	}

	static int s_VoteList = 0;
	static float s_ScrollValue = 0;
	CUIRect List = MainView;
	UiDoListboxStart(&s_VoteList, &List, 24.0f, Localize("Players"), "", NumOptions, 1, Selected, s_ScrollValue);
	
	for(int i = 0; i < NumOptions; i++)
	{
		CListboxItem Item = UiDoListboxNextItem(&aPlayerIDs[i]);
		
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info = m_pClient->m_aClients[aPlayerIDs[i]].m_RenderInfo;
			Info.m_Size = Item.m_Rect.h;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0), vec2(Item.m_Rect.x+Item.m_Rect.h/2, Item.m_Rect.y+Item.m_Rect.h/2));
			Item.m_Rect.x +=Info.m_Size;
			UI()->DoLabel(&Item.m_Rect, m_pClient->m_aClients[aPlayerIDs[i]].m_aName, 16.0f, -1);
		}
	}
	
	Selected = UiDoListboxEnd(&s_ScrollValue, 0);
	m_CallvoteSelectedPlayer = Selected != -1 ? aPlayerIDs[Selected] : -1;
}

void CMenus::RenderServerControl(CUIRect MainView)
{
	static int s_ControlPage = 0;
	
	// render background
	CUIRect Temp, TabBar;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 10.0f);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 10.0f);
	
	MainView.HSplitTop(10.0f, 0, &MainView);
	
	CUIRect Button;
	
	const char *paTabs[] = {
		Localize("Settings"),
		Localize("Kick")};
	int aNumTabs = (int)(sizeof(paTabs)/sizeof(*paTabs));
	
	for(int i = 0; i < aNumTabs; i++)
	{
		TabBar.HSplitTop(10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(DoButton_MenuTab(paTabs[i], paTabs[i], s_ControlPage == i, &Button, CUI::CORNER_R))
		{
			s_ControlPage = i;
			m_CallvoteSelectedPlayer = -1;
			m_CallvoteSelectedOption = -1;
		}
	}
		
	MainView.Margin(10.0f, &MainView);
	CUIRect Bottom;
	MainView.HSplitBottom(ms_ButtonHeight + 5*2, &MainView, &Bottom);
	Bottom.HMargin(5.0f, &Bottom);
	
	// render page		
	if(s_ControlPage == 0)
		RenderServerControlServer(MainView);
	else if(s_ControlPage == 1)
		RenderServerControlKick(MainView);
		

	{
		CUIRect Button;
		Bottom.VSplitRight(120.0f, &Bottom, &Button);
		
		static int s_CallVoteButton = 0;
		if(DoButton_Menu(&s_CallVoteButton, Localize("Call vote"), 0, &Button))
		{
			if(s_ControlPage == 0)
			{
				//
				m_pClient->m_pVoting->CallvoteOption(m_CallvoteSelectedOption);
				/*
				if(callvote_selectedmap >= 0 && callvote_selectedmap < gameclient.maplist->num())
					gameclient.voting->callvote_map(gameclient.maplist->name(callvote_selectedmap));*/
			}
			else if(s_ControlPage == 1)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					m_pClient->m_Snap.m_paPlayerInfos[m_CallvoteSelectedPlayer])
				{
					m_pClient->m_pVoting->CallvoteKick(m_CallvoteSelectedPlayer, m_aCallvoteReason);
					m_aCallvoteReason[0] = 0;
					SetActive(false);
				}
			}
		}
		
		// render kick reason
		if(s_ControlPage == 1)
		{
			CUIRect Reason;
			Bottom.VSplitRight(40.0f, &Bottom, 0);
			Bottom.VSplitRight(160.0f, &Bottom, &Reason);
			Reason.HSplitTop(5.0f, 0, &Reason);
			const char *pLabel = Localize("Reason:");
			UI()->DoLabel(&Reason, pLabel, 14.0f, -1);
			float w = TextRender()->TextWidth(0, 14.0f, pLabel, -1);
			Reason.VSplitLeft(w+10.0f, 0, &Reason);
			static float s_Offset = 0.0f;
			DoEditBox(&m_aCallvoteReason, &Reason, m_aCallvoteReason, sizeof(m_aCallvoteReason), 14.0f, &s_Offset, false, CUI::CORNER_ALL);
		}
		
		// force vote button (only available when authed in rcon)
		if(Client()->RconAuthed())
		{
			Bottom.VSplitLeft(120.0f, &Button, &Bottom);
			
			static int s_ForceVoteButton = 0;
			if(DoButton_Menu(&s_ForceVoteButton, Localize("Force vote"), 0, &Button))
			{
				if(s_ControlPage == 0)
				{
					m_pClient->m_pVoting->ForcevoteOption(m_CallvoteSelectedOption);
				}
				else if(s_ControlPage == 1)
				{
					if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
						m_pClient->m_Snap.m_paPlayerInfos[m_CallvoteSelectedPlayer])
					{
						m_pClient->m_pVoting->ForcevoteKick(m_CallvoteSelectedPlayer, m_aCallvoteReason);
						m_aCallvoteReason[0] = 0;
						SetActive(false);
					}
				}
			}
		}
	}		
}

void CMenus::RenderInGameBrowser(CUIRect MainView)
{
	CUIRect Box = MainView;
	CUIRect Button;

	static int PrevPage = g_Config.m_UiPage;
	static int LastServersPage = g_Config.m_UiPage;
	int ActivePage = g_Config.m_UiPage;
	int NewPage = -1;

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	Box.HSplitTop(5.0f, &MainView, &MainView);
	Box.HSplitTop(24.0f, &Box, &MainView);
	Box.VMargin(20.0f, &Box);

	Box.VSplitLeft(100.0f, &Button, &Box);
	static int s_InternetButton=0;
	if(DoButton_MenuTab(&s_InternetButton, Localize("Internet"), ActivePage==PAGE_INTERNET, &Button, CUI::CORNER_TL))
	{
		if (PrevPage != PAGE_SETTINGS || LastServersPage != PAGE_INTERNET) ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
		LastServersPage = PAGE_INTERNET;
		NewPage = PAGE_INTERNET;
	}

	//Box.VSplitLeft(4.0f, 0, &Box);
	Box.VSplitLeft(80.0f, &Button, &Box);
	static int s_LanButton=0;
	if(DoButton_MenuTab(&s_LanButton, Localize("LAN"), ActivePage==PAGE_LAN, &Button, 0))
	{
		if (PrevPage != PAGE_SETTINGS || LastServersPage != PAGE_LAN) ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
		LastServersPage = PAGE_LAN;
		NewPage = PAGE_LAN;
	}

	//box.VSplitLeft(4.0f, 0, &box);
	Box.VSplitLeft(110.0f, &Button, &Box);
	static int s_FavoritesButton=0;
	if(DoButton_MenuTab(&s_FavoritesButton, Localize("Favorites"), ActivePage==PAGE_FAVORITES, &Button, CUI::CORNER_TR))
	{
		if (PrevPage != PAGE_SETTINGS || LastServersPage != PAGE_FAVORITES) ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
		LastServersPage = PAGE_FAVORITES;
		NewPage  = PAGE_FAVORITES;
	}

	if(NewPage != -1)
	{
		if(Client()->State() != IClient::STATE_OFFLINE)
			g_Config.m_UiPage = NewPage;
	}

	PrevPage = g_Config.m_UiPage;

	RenderServerbrowser(MainView);
	return;
}
