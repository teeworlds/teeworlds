/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/config.h>
#include <engine/demo.h>
#include <engine/contacts.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include "menus.h"
#include "motd.h"
#include "voting.h"
#include "broadcast.h"

void CMenus::GetSwitchTeamInfo(CSwitchTeamInfo *pInfo)
{
	pInfo->m_aNotification[0] = 0;
	int TeamMod = m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS ? -1 : 0;
	pInfo->m_AllowSpec = true;
	pInfo->m_TimeLeft = 0;

	if(m_pClient->m_ServerSettings.m_TeamLock)
	{
		str_copy(pInfo->m_aNotification, Localize("Teams are locked"), sizeof(pInfo->m_aNotification));
		pInfo->m_AllowSpec = false;
	}
	else if(TeamMod + m_pClient->m_GameInfo.m_aTeamSize[TEAM_RED] + m_pClient->m_GameInfo.m_aTeamSize[TEAM_BLUE] >= m_pClient->m_ServerSettings.m_PlayerSlots)
	{
		str_format(pInfo->m_aNotification, sizeof(pInfo->m_aNotification), Localize("Only %d active players are allowed"), m_pClient->m_ServerSettings.m_PlayerSlots);
	}
	else if(m_pClient->m_TeamCooldownTick + 1 >= Client()->GameTick())
	{
		pInfo->m_TimeLeft = (m_pClient->m_TeamCooldownTick - Client()->GameTick()) / Client()->GameTickSpeed() + 1;
		str_format(pInfo->m_aNotification, sizeof(pInfo->m_aNotification), Localize("Teams are locked. Time to wait before changing team: %02d:%02d"), pInfo->m_TimeLeft / 60, pInfo->m_TimeLeft % 60);
		pInfo->m_AllowSpec = false;
	}
}

void CMenus::RenderGame(CUIRect MainView)
{
	if(m_pClient->m_LocalClientID == -1)
		return;

	char aBuf[128];
	CSwitchTeamInfo Info = {{0}};
	GetSwitchTeamInfo(&Info);
	CUIRect Button, ButtonRow, Label;

	float Spacing = 3.0f;
	float ButtonWidth = (MainView.w/6.0f)-(Spacing*5.0)/6.0f;

	// cut view
	MainView.HSplitTop(20.0f, 0, &MainView);
	float NoteHeight = !Info.m_aNotification[0] ? 0.0f : 45.0f;
	MainView.HSplitTop(20.0f+20.0f+2*Spacing+ NoteHeight, &MainView, 0);
	MainView.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f));

	// game options
	MainView.HSplitTop(20.0f, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Game options"), 20.0f*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);
	MainView.Draw(vec4(0.0, 0.0, 0.0, 0.25f));

	if(Info.m_aNotification[0] != 0)
	{
		// print notice
		CUIRect Bar;
		MainView.HSplitBottom(NoteHeight, &MainView, &Bar);
		Bar.HMargin(15.0f, &Bar);
		UI()->DoLabel(&Bar, Info.m_aNotification, 14.0f, TEXTALIGN_CENTER);
	}

	// buttons
	{
		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(20.0f, &ButtonRow, 0);
		ButtonRow.VMargin(Spacing, &ButtonRow);

		// specator button
		int Team = m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team;
		if(!Info.m_AllowSpec && Team != TEAM_SPECTATORS)
		{
			if(Info.m_TimeLeft)
				str_format(aBuf, sizeof(aBuf), "(%d)", Info.m_TimeLeft);
			else
				str_copy(aBuf, Localize("locked"), sizeof(aBuf));
		}
		else
			str_copy(aBuf, Localize(Team != TEAM_SPECTATORS ? "Spectate" : "Spectating"), sizeof(aBuf)); // Localize("Spectating");

		ButtonRow.VSplitLeft(ButtonWidth, &Button, &ButtonRow);
		ButtonRow.VSplitLeft(Spacing, 0, &ButtonRow);
		static CButtonContainer s_SpectateButton;
		if(DoButton_Menu(&s_SpectateButton, aBuf, Team == TEAM_SPECTATORS, &Button) && Team != TEAM_SPECTATORS && Info.m_AllowSpec)
		{
			m_pClient->SendSwitchTeam(TEAM_SPECTATORS);
			SetActive(false);
		}

		// team button
		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		{
			int RedTeamSizeNew = m_pClient->m_GameInfo.m_aTeamSize[TEAM_RED];
			if(Team != TEAM_RED)
				++RedTeamSizeNew;
			int BlueTeamSizeNew = m_pClient->m_GameInfo.m_aTeamSize[TEAM_BLUE];
			if(Team == TEAM_BLUE)
				--BlueTeamSizeNew;
			bool BlockRed = m_pClient->m_ServerSettings.m_TeamBalance && (RedTeamSizeNew - BlueTeamSizeNew >= NUM_TEAMS);
			if((Info.m_aNotification[0] && Team != TEAM_RED) || BlockRed)
			{
				if(Info.m_TimeLeft)
					str_format(aBuf, sizeof(aBuf), "(%d)", Info.m_TimeLeft);
				else
					str_copy(aBuf, Localize("locked"), sizeof(aBuf));
			}
			else
				str_copy(aBuf, Localize(Team != TEAM_RED ? "Join red" : "Joined red"), sizeof(aBuf)); // Localize("Join red");Localize("Joined red");

			ButtonRow.VSplitLeft(ButtonWidth, &Button, &ButtonRow);
			ButtonRow.VSplitLeft(Spacing, 0, &ButtonRow);
			static CButtonContainer s_RedButton;
			if(DoButton_Menu(&s_RedButton, aBuf, Team == TEAM_RED, &Button, 0, CUIRect::CORNER_ALL, 5.0f, 0.0f, vec4(0.975f, 0.17f, 0.17f, 0.75f), false) && Team != TEAM_RED && !(Info.m_aNotification[0]) && !BlockRed)
			{
				m_pClient->SendSwitchTeam(TEAM_RED);
				SetActive(false);
			}

			RedTeamSizeNew = m_pClient->m_GameInfo.m_aTeamSize[TEAM_RED];
			if(Team == TEAM_RED)
				--RedTeamSizeNew;
			BlueTeamSizeNew = m_pClient->m_GameInfo.m_aTeamSize[TEAM_BLUE];
			if(Team != TEAM_BLUE)
				++BlueTeamSizeNew;
			bool BlockBlue = m_pClient->m_ServerSettings.m_TeamBalance && (BlueTeamSizeNew - RedTeamSizeNew >= NUM_TEAMS);
			if((Info.m_aNotification[0] && Team != TEAM_BLUE) || BlockBlue)
			{
				if(Info.m_TimeLeft)
					str_format(aBuf, sizeof(aBuf), "(%d)", Info.m_TimeLeft);
				else
					str_copy(aBuf, Localize("locked"), sizeof(aBuf));
			}
			else
				str_copy(aBuf, Localize(Team != TEAM_BLUE ? "Join blue" : "Joined blue"), sizeof(aBuf)); // Localize("Join blue");Localize("Joined blue");

			ButtonRow.VSplitLeft(ButtonWidth, &Button, &ButtonRow);
			ButtonRow.VSplitLeft(Spacing, 0, &ButtonRow);
			static CButtonContainer s_BlueButton;
			if(DoButton_Menu(&s_BlueButton, aBuf, Team == TEAM_BLUE, &Button, 0, CUIRect::CORNER_ALL, 5.0f, 0.0f, vec4(0.17f, 0.46f, 0.975f, 0.75f), false) && Team != TEAM_BLUE && !(Info.m_aNotification[0]) && !BlockBlue)
			{
				m_pClient->SendSwitchTeam(TEAM_BLUE);
				SetActive(false);
			}
		}
		else
		{
			if(Info.m_aNotification[0] && Team != TEAM_RED)
			{
				if(Info.m_TimeLeft)
					str_format(aBuf, sizeof(aBuf), "(%d)", Info.m_TimeLeft);
				else
					str_copy(aBuf, Localize("locked"), sizeof(aBuf));
			}
			else
				str_copy(aBuf, Localize(Team != TEAM_RED ? "Join" : "Joined"), sizeof(aBuf)); //Localize("Join");Localize("Joined");

			ButtonRow.VSplitLeft(ButtonWidth, &Button, &ButtonRow);
			static CButtonContainer s_JoinButton;
			if(DoButton_Menu(&s_JoinButton, aBuf, Team == TEAM_RED, &Button) && Team != TEAM_RED && !(Info.m_aNotification[0]))
			{
				m_pClient->SendSwitchTeam(TEAM_RED);
				SetActive(false);
			}
		}

		// disconnect button
		ButtonRow.VSplitRight(ButtonWidth, &ButtonRow, &Button);
		static CButtonContainer s_DisconnectButton;
		if(DoButton_Menu(&s_DisconnectButton, Localize("Disconnect"), 0, &Button))
			Client()->Disconnect();

		// Record button
		ButtonRow.VSplitRight(50.0f, &ButtonRow, 0);
		ButtonRow.VSplitRight(ButtonWidth, &ButtonRow, &Button);
		static CButtonContainer s_DemoButton;
		bool Recording = DemoRecorder()->IsRecording();
		if(DoButton_Menu(&s_DemoButton, Localize(Recording ? "Stop record" : "Record"), Recording, &Button))	// Localize("Stop record");Localize("Record");
		{
			if(!Recording)
				Client()->DemoRecorder_Start("demo", true);
			else
				Client()->DemoRecorder_Stop();
		}
	}
}

void CMenus::RenderPlayers(CUIRect MainView)
{
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;
	const float NameWidth = 250.0f;
	const float ClanWidth = 250.0f;
	CUIRect Label, Row;
	MainView.HSplitBottom(80.0f, &MainView, 0);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f));

	// player options
	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Player options"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);
	MainView.Draw(vec4(0.0, 0.0, 0.0, 0.25f));

	// prepare headline
	MainView.HSplitTop(ButtonHeight, &Row, &MainView);

	// background
	MainView.Draw(vec4(0.0, 0.0, 0.0, 0.25f));
	MainView.Margin(5.0f, &MainView);

	// prepare scroll
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollUnit = ButtonHeight * 3; // 3 players per scroll
	if(s_ScrollRegion.IsScrollbarShown())
		Row.VSplitRight(ScrollParams.m_ScrollbarWidth, &Row, 0);

	// headline
	Row.VSplitLeft(ButtonHeight+Spacing, 0, &Row);
	Row.VSplitLeft(NameWidth, &Label, &Row);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Player"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	Row.VSplitRight(2*ButtonHeight, &Row, &Label);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_GUIICON_MUTE);
	IGraphics::CQuadItem QuadItem(Label.x, Label.y, Label.w, Label.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	Row.VSplitRight(2*Spacing, &Row, 0);
	Row.VSplitRight(2*ButtonHeight, &Row, &Label);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_GUIICON_FRIEND);
	QuadItem = IGraphics::CQuadItem(Label.x, Label.y, Label.w, Label.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// scroll, ignore margins
	MainView.Margin(-5.0f, &MainView);
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.Margin(5.0f, &MainView);
	MainView.y += ScrollOffset.y;

	// options
	static int s_aPlayerIDs[MAX_CLIENTS][2] = {{0}};
	int Teams[3] = { TEAM_RED, TEAM_BLUE, TEAM_SPECTATORS };
	for(int Team = 0, Count = 0; Team < 3; ++Team)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(i == m_pClient->m_LocalClientID || !m_pClient->m_aClients[i].m_Active || m_pClient->m_aClients[i].m_Team != Teams[Team])
				continue;

			MainView.HSplitTop(ButtonHeight, &Row, &MainView);
			s_ScrollRegion.AddRect(Row);

			Count++;
			if(s_ScrollRegion.IsRectClipped(Row))
				continue;

			if(Count % 2 == 1)
				Row.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f));

			// player info
			Row.VSplitLeft(ButtonHeight, &Label, &Row);
			Label.y += 2.0f;
			CTeeRenderInfo Info = m_pClient->m_aClients[i].m_RenderInfo;
			Info.m_Size = Label.h;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Label.x + Label.h / 2, Label.y + Label.h / 2));

			Row.VSplitLeft(2*Spacing, 0, &Row);
			Row.VSplitLeft(NameWidth, &Label, &Row);
			Label.y += 2.0f;
			if(Config()->m_ClShowUserId)
			{
				UI()->DrawClientID(ButtonHeight*CUI::ms_FontmodHeight*0.8f, vec2(Label.x, Label.y), i);
				Label.VSplitLeft(ButtonHeight, 0, &Label);
			}
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[i].m_aName : "");
			UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);
			Row.VSplitLeft(Spacing, 0, &Row);
			Row.VSplitLeft(ClanWidth, &Label, &Row);
			Label.y += 2.0f;
			str_format(aBuf, sizeof(aBuf), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[i].m_aClan : "");
			UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

			// ignore button
			Row.VSplitRight(ButtonHeight/2, &Row, 0);
			Row.VSplitRight(ButtonHeight, &Row, &Label);
			if(Config()->m_ClFilterchat == 2 || (Config()->m_ClFilterchat == 1 && !m_pClient->m_aClients[i].m_Friend))
				DoButton_Toggle(&s_aPlayerIDs[i][0], 1, &Label, false);
			else
				if(DoButton_Toggle(&s_aPlayerIDs[i][0], m_pClient->m_aClients[i].m_ChatIgnore, &Label, true))
				{
					if(m_pClient->m_aClients[i].m_ChatIgnore)
						m_pClient->Blacklist()->RemoveIgnoredPlayer(m_pClient->m_aClients[i].m_aName, m_pClient->m_aClients[i].m_aClan);
					else
						m_pClient->Blacklist()->AddIgnoredPlayer(m_pClient->m_aClients[i].m_aName, m_pClient->m_aClients[i].m_aClan);

					m_pClient->m_aClients[i].m_ChatIgnore ^= 1;
				}

			// friend button
			Row.VSplitRight(2*Spacing+ButtonHeight,&Row, 0);
			Row.VSplitRight(ButtonHeight, &Row, &Label);
			if(DoButton_Toggle(&s_aPlayerIDs[i][1], m_pClient->m_aClients[i].m_Friend, &Label, true))
			{
				if(m_pClient->m_aClients[i].m_Friend)
					m_pClient->Friends()->RemoveFriend(m_pClient->m_aClients[i].m_aName, m_pClient->m_aClients[i].m_aClan);
				else
					m_pClient->Friends()->AddFriend(m_pClient->m_aClients[i].m_aName, m_pClient->m_aClients[i].m_aClan);

				m_pClient->m_aClients[i].m_Friend ^= 1;
			}
		}
	}
	s_ScrollRegion.End();
}

void CMenus::RenderServerInfo(CUIRect MainView)
{
	if(!m_pClient->m_Snap.m_pLocalInfo)
		return;

	// fetch server info
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	// render background
	MainView.HSplitBottom(80.0f, &MainView, 0);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f));

	CUIRect ServerInfo, GameInfo, Motd, Label;

	char aBuf[128] = {0};
	const float ButtonHeight = 20.0f;

	// serverinfo
	MainView.HSplitBottom(250.0f, &ServerInfo, &Motd);
	ServerInfo.VSplitMid(&ServerInfo, &GameInfo, 2.0f);
	ServerInfo.Draw(vec4(0.0, 0.0, 0.0, 0.25f));

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Server info"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);
	ServerInfo.Draw(vec4(0.0, 0.0, 0.0, 0.25f));
	ServerInfo.Margin(5.0f, &ServerInfo);

	ServerInfo.HSplitTop(2*ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, CurrentServerInfo.m_aName, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT, Label.w);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Address"), CurrentServerInfo.m_aHostname);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Ping"), m_pClient->m_Snap.m_pLocalInfo->m_Latency);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Version"), CurrentServerInfo.m_aVersion);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Password"), CurrentServerInfo.m_Flags&IServerBrowser::FLAG_PASSWORD ? Localize("Yes", "With") : Localize("No", "Without/None"));
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	const bool IsFavorite = CurrentServerInfo.m_Favorite;
	ServerInfo.HSplitBottom(ButtonHeight, &ServerInfo, &Label);
	static int s_AddFavButton = 0;
	if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), IsFavorite, &Label))
	{
		if(IsFavorite)
			ServerBrowser()->RemoveFavorite(&CurrentServerInfo);
		else
			ServerBrowser()->AddFavorite(&CurrentServerInfo);
	}

	{
		CUIRect Button;
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		static int s_MuteBroadcast = 0;
		if(DoButton_CheckBox(&s_MuteBroadcast, Localize("Mute broadcasts"), m_pClient->m_pBroadcast->IsMuteServerBroadcast(), &Button))
		{
			m_pClient->m_pBroadcast->ToggleMuteServerBroadcast();
		}
	}

	// gameinfo
	GameInfo.Draw(vec4(0.0, 0.0, 0.0, 0.25f));

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Game info"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);
	GameInfo.Draw(vec4(0.0, 0.0, 0.0, 0.25f));
	GameInfo.Margin(5.0f, &GameInfo);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Game type"), CurrentServerInfo.m_aGameType);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Map"), CurrentServerInfo.m_aMap);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	const char *pLevelName = "";
	switch(CurrentServerInfo.m_ServerLevel)
	{
		case CServerInfo::LEVEL_CASUAL:
			pLevelName = Localize("Casual", "Server difficulty");
			break;
		case CServerInfo::LEVEL_NORMAL:
			pLevelName = Localize("Normal", "Server difficulty");
			break;
		case CServerInfo::LEVEL_COMPETITIVE:
			pLevelName = Localize("Competitive", "Server difficulty");
			break;
	}
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Difficulty"), pLevelName);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_GameInfo.m_ScoreLimit);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Time limit"), m_pClient->m_GameInfo.m_TimeLimit);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	GameInfo.HSplitBottom(ButtonHeight, &GameInfo, &Label);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d/%d", Localize("Players"), m_pClient->m_GameInfo.m_NumPlayers, CurrentServerInfo.m_MaxClients);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);

	// motd
	Motd.HSplitTop(2.0f, 0, &Motd);
	Motd.Draw(vec4(0.0, 0.0, 0.0, 0.25f));

	Motd.HSplitTop(ButtonHeight, &Label, &Motd);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("MOTD"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);
	Motd.Draw(vec4(0.0, 0.0, 0.0, 0.25f));
	Motd.Margin(5.0f, &Motd);

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);

	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = ButtonHeight*CUI::ms_FontmodHeight*0.8f*3; // 3 rows per scroll

	s_ScrollRegion.Begin(&Motd, &ScrollOffset, &ScrollParams);
	Motd.y += ScrollOffset.y;

	static CTextCursor s_MenuMotdCursor;
	s_MenuMotdCursor.m_FontSize = ButtonHeight*CUI::ms_FontmodHeight*0.8f;
	s_MenuMotdCursor.MoveTo(Motd.x, Motd.y);
	s_MenuMotdCursor.m_MaxWidth = Motd.w;
	s_MenuMotdCursor.m_MaxLines = -1;
	s_MenuMotdCursor.m_Flags = TEXTFLAG_ALLOW_NEWLINE | TEXTFLAG_WORD_WRAP;
	s_MenuMotdCursor.Reset();
	TextRender()->TextOutlined(&s_MenuMotdCursor, m_pClient->m_pMotd->GetMotd(), -1);

	// define the MOTD text area and make it scrollable
	CUIRect MotdTextArea;
	Motd.HSplitTop(s_MenuMotdCursor.BoundingBox().Bottom()-Motd.y+ButtonHeight*CUI::ms_FontmodHeight*0.8f+5.0f, &MotdTextArea, &Motd);
	s_ScrollRegion.AddRect(MotdTextArea);

	s_ScrollRegion.End();
}

bool CMenus::RenderServerControlServer(CUIRect MainView)
{
	static CListBox s_ListBox;
	CUIRect List = MainView;
	s_ListBox.DoHeader(&List, Localize("Option"), UI()->GetListHeaderHeight());
	s_ListBox.DoStart(20.0f, m_pClient->m_pVoting->NumVoteOptions(), 1, 3, m_CallvoteSelectedOption, 0, true, 0, CUIRect::CORNER_NONE);

	for(const CVoteOptionClient *pOption = m_pClient->m_pVoting->FirstVoteOption(); pOption; pOption = pOption->m_pNext)
	{
		if(m_aFilterString[0] && !pOption->m_IsSubheader && !str_find_nocase(pOption->m_aDescription, m_aFilterString))
			continue; // no match found

		if(!pOption->m_aDescription[0])
			continue; // depth resets

		CListboxItem Item = pOption->m_IsSubheader ? s_ListBox.DoSubheader() : s_ListBox.DoNextItem(pOption);

		if(Item.m_Visible)
		{
			Item.m_Rect.VMargin(5.0f, &Item.m_Rect);
			Item.m_Rect.y += 2.0f;
			for(int i = pOption->m_IsSubheader ? 1 : 0; i < pOption->m_Depth; i++)
				Item.m_Rect.VSplitLeft(10.0f, 0, &Item.m_Rect);

			UI()->DoLabel(&Item.m_Rect, pOption->m_aDescription, Item.m_Rect.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);
		}
	}

	m_CallvoteSelectedOption = s_ListBox.DoEnd();
	return s_ListBox.WasItemActivated();
}

void CMenus::RenderServerControlKick(CUIRect MainView, bool FilterSpectators)
{
	int NumOptions = 0;
	int Selected = -1;
	static int s_aPlayerIDs[MAX_CLIENTS];
	int Teams[3] = { TEAM_RED, TEAM_BLUE, TEAM_SPECTATORS };
	for(int Team = 0; Team < 3; ++Team)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(i == m_pClient->m_LocalClientID || !m_pClient->m_aClients[i].m_Active || m_pClient->m_aClients[i].m_Team != Teams[Team] ||
				(FilterSpectators && m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS) ||
				(!FilterSpectators && m_pClient->m_Snap.m_paPlayerInfos[i] && (m_pClient->m_Snap.m_paPlayerInfos[i]->m_PlayerFlags&PLAYERFLAG_ADMIN)))
				continue;
			if(m_CallvoteSelectedPlayer == i)
				Selected = NumOptions;
			s_aPlayerIDs[NumOptions++] = i;
		}
	}

	const float Spacing = 2.0f;
	const float NameWidth = 250.0f;
	const float ClanWidth = 250.0f;

	static CListBox s_ListBox;
	CUIRect List = MainView;
	s_ListBox.DoHeader(&List, Localize("Player"), UI()->GetListHeaderHeight());
	s_ListBox.DoStart(20.0f, NumOptions, 1, 3, Selected, 0, true, 0, CUIRect::CORNER_NONE);

	for(int i = 0; i < NumOptions; i++)
	{
		CListboxItem Item = s_ListBox.DoNextItem(&s_aPlayerIDs[i]);
		if(Item.m_Visible)
		{
			CUIRect Label, Row;
			Item.m_Rect.VMargin(5.0f, &Row);

			// player info
			Row.VSplitLeft(Row.h, &Label, &Row);
			Label.y += 2.0f;
			CTeeRenderInfo Info = m_pClient->m_aClients[s_aPlayerIDs[i]].m_RenderInfo;
			Info.m_Size = Label.h;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Label.x + Label.h / 2, Label.y + Label.h / 2));

			Row.VSplitLeft(2*Spacing, 0, &Row);
			if(Config()->m_ClShowUserId)
			{
				Row.VSplitLeft(Row.h, &Label, &Row);
				Label.y += 2.0f;
				UI()->DrawClientID(Label.h*CUI::ms_FontmodHeight*0.8f, vec2(Label.x, Label.y), s_aPlayerIDs[i]);
			}

			Row.VSplitLeft(Spacing, 0, &Row);
			Row.VSplitLeft(NameWidth, &Label, &Row);
			Label.y += 2.0f;
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[s_aPlayerIDs[i]].m_aName : "");
			UI()->DoLabel(&Label, aBuf, Label.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);
			Row.VSplitLeft(Spacing, 0, &Row);
			Row.VSplitLeft(ClanWidth, &Label, &Row);
			Label.y += 2.0f;
			str_format(aBuf, sizeof(aBuf), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[s_aPlayerIDs[i]].m_aClan : "");
			UI()->DoLabel(&Label, aBuf, Label.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);
		}
	}

	Selected = s_ListBox.DoEnd();
	m_CallvoteSelectedPlayer = Selected != -1 ? s_aPlayerIDs[Selected] : -1;
}

void CMenus::HandleCallvote(int Page, bool Force)
{
	if(Page == 0)
	{
		// find the correct index within the filtered list
		int RealIndex = 0, FilteredIndex = 0;
		for(const CVoteOptionClient *pOption = m_pClient->m_pVoting->FirstVoteOption(); pOption; pOption = pOption->m_pNext, RealIndex++)
		{
			if(m_aFilterString[0] && !pOption->m_IsSubheader && !str_find_nocase(pOption->m_aDescription, m_aFilterString))
				continue; // no match found

			if(!pOption->m_aDescription[0])
				continue; // depth reset

			if(FilteredIndex == m_CallvoteSelectedOption)
				break;

			FilteredIndex++;
		}
		m_pClient->m_pVoting->CallvoteOption(RealIndex, m_aCallvoteReason, Force);
	}
	else if(Page == 1)
	{
		if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
			m_pClient->m_aClients[m_CallvoteSelectedPlayer].m_Active)
		{
			m_pClient->m_pVoting->CallvoteKick(m_CallvoteSelectedPlayer, m_aCallvoteReason, Force);
			SetActive(false);
		}
	}
	else if(Page == 2)
	{
		if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
			m_pClient->m_aClients[m_CallvoteSelectedPlayer].m_Active)
		{
			m_pClient->m_pVoting->CallvoteSpectate(m_CallvoteSelectedPlayer, m_aCallvoteReason, Force);
			SetActive(false);
		}
	}
}

void CMenus::RenderServerControl(CUIRect MainView)
{
	if(m_pClient->m_LocalClientID == -1)
		return;

	static int s_ControlPage = 0;
	const char *pNotification = 0;
	char aBuf[64];
	const bool Authed = Client()->RconAuthed();

	if(m_pClient->m_pVoting->IsVoting())
		pNotification = Localize("Wait for current vote to end before calling a new one.");
	else if(m_pClient->m_pVoting->CallvoteBlockTime() != 0)
	{
		str_format(aBuf, sizeof(aBuf), Localize("You must wait %d seconds before making another vote"), m_pClient->m_pVoting->CallvoteBlockTime());
		pNotification = aBuf;
	}
	else if(!Authed && m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS)
		pNotification = Localize("Spectators aren't allowed to start a vote.");

	MainView.HSplitBottom(80.0f, &MainView, 0);
	if(pNotification && !Authed)
	{
		MainView.HSplitTop(20.0f, 0, &MainView);
		// only print notice
		CUIRect Bar;
		MainView.HSplitTop(45.0f, &Bar, &MainView);
		Bar.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f+Config()->m_ClMenuAlpha/100.0f));
		Bar.HMargin(15.0f, &Bar);
		UI()->DoLabel(&Bar, pNotification, 14.0f, TEXTALIGN_CENTER);
		return;
	}

	// tab bar
	const float NotActiveAlpha = 0.5f;
	CUIRect Bottom, Extended, Button, Row;
	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(25.0f, &Row, &MainView);
	Row.VSplitLeft(Row.w/3-1.5f, &Button, &Row);
	static CButtonContainer s_Button0;
	if(DoButton_MenuTabTop(&s_Button0, Localize("Change settings"), false, &Button, s_ControlPage == 0 ? 1.0f : NotActiveAlpha, 1.0f, CUIRect::CORNER_T, 5.0f, 0.25f))
		s_ControlPage = 0;

	Row.VSplitLeft(1.5f, 0, &Row);
	Row.VSplitMid(&Button, &Row);
	Button.VMargin(1.5f, &Button);
	static CButtonContainer s_Button1;
	if(DoButton_MenuTabTop(&s_Button1, Localize("Kick player"), false, &Button, s_ControlPage == 1 ? 1.0f : NotActiveAlpha, 1.0f, CUIRect::CORNER_T, 5.0f, 0.25f))
		s_ControlPage = 1;

	Row.VSplitLeft(1.5f, 0, &Button);
	static CButtonContainer s_Button2;
	if(DoButton_MenuTabTop(&s_Button2, Localize("Move player to spectators"), false, &Button, s_ControlPage == 2 ? 1.0f : NotActiveAlpha, 1.0f, CUIRect::CORNER_T, 5.0f, 0.25f))
		s_ControlPage = 2;

	if(s_ControlPage == 1)
	{
		if(!m_pClient->m_ServerSettings.m_KickVote)
			pNotification = Localize("Server does not allow voting to kick players");
		else if(m_pClient->m_GameInfo.m_aTeamSize[TEAM_RED]+m_pClient->m_GameInfo.m_aTeamSize[TEAM_BLUE] < m_pClient->m_ServerSettings.m_KickMin)
		{
			str_format(aBuf, sizeof(aBuf), Localize("Kick voting requires %d players on the server"), m_pClient->m_ServerSettings.m_KickMin);
			pNotification = aBuf;
		}
	}
	else if(s_ControlPage == 2 && !m_pClient->m_ServerSettings.m_SpecVote)
		pNotification = Localize("Server does not allow voting to move players to spectators");

	if(pNotification && !Authed)
	{
		// only print notice
		MainView.HSplitTop(45.0f, &MainView, 0);
		MainView.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, CUIRect::CORNER_B);
		MainView.HMargin(15.0f, &MainView);
		UI()->DoLabel(&MainView, pNotification, 14.0f, TEXTALIGN_CENTER);
		return;
	}

	// render background
	const float Spacing = 10.0f;
	const float LineHeight = 20.0f;
	const float ColumnWidth = 120.0f;
	MainView.HSplitBottom(LineHeight + 2*Spacing + (Authed ? (3*LineHeight + 2*Spacing) : 0.0f), &MainView, &Extended);
	Extended.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, CUIRect::CORNER_B);

	bool DoCallVote = false;
	// render page
	if(s_ControlPage == 0)
		// double click triggers vote if not spectating
		DoCallVote = RenderServerControlServer(MainView) && m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS;
	else if(s_ControlPage == 1)
		RenderServerControlKick(MainView, false);
	else if(s_ControlPage == 2)
		RenderServerControlKick(MainView, true);

	// vote menu
	Extended.Margin(Spacing, &Extended);
	Extended.HSplitTop(LineHeight, &Bottom, &Extended);
	{
		if(Authed || m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS)
		{
			CUIRect Search;
			// render search
			Bottom.VSplitLeft(260.0f, &Search, &Bottom);

			if(s_ControlPage == 0)
			{
				const char *pSearchLabel = Localize("Search:");
				const float FontSize = Search.h*CUI::ms_FontmodHeight*0.8f;
				CUIRect Label;
				Search.VSplitLeft(TextRender()->TextWidth(FontSize, pSearchLabel, -1) + 10.0f, &Label, &Search);
				Label.y += 2.0f;
				UI()->DoLabel(&Label, pSearchLabel, FontSize, TEXTALIGN_LEFT);
				static CLineInput s_FilterInput(m_aFilterString, sizeof(m_aFilterString));
				if(UI()->DoEditBox(&s_FilterInput, &Search, FontSize))
					m_CallvoteSelectedOption = 0;
			}

			if(pNotification)
			{
				Bottom.y += 2.0f;
				UI()->DoLabel(&Bottom, pNotification, Bottom.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_LEFT);
			}
			else
			{
				CUIRect Reason, CallVoteButton, ClearButton, Label;
				Bottom.VSplitRight(ColumnWidth, &Bottom, &CallVoteButton);
				Bottom.VSplitRight(2*Spacing, &Bottom, 0);
				Bottom.VSplitRight(200.0f, &Bottom, &Reason);
				Reason.VSplitRight(Reason.h, &Reason, &ClearButton);

				// render reason
				const char *pReasonLabel = Localize("Reason:");
				const float FontSize = Reason.h*CUI::ms_FontmodHeight*0.8f;
				Reason.VSplitLeft(TextRender()->TextWidth(FontSize, pReasonLabel, -1) + 10.0f, &Label, &Reason);
				Label.y += 2.0f;
				UI()->DoLabel(&Label, pReasonLabel, FontSize, TEXTALIGN_LEFT);
				static CLineInput s_ReasonInput(m_aCallvoteReason, sizeof(m_aCallvoteReason));
				UI()->DoEditBox(&s_ReasonInput, &Reason, FontSize, CUIRect::CORNER_L);

				// clear button
				static CButtonContainer s_ClearButton;
				if(DoButton_SpriteID(&s_ClearButton, IMAGE_TOOLICONS, SPRITE_TOOL_X_A, false, &ClearButton, CUIRect::CORNER_R, 5.0f, true))
					m_aCallvoteReason[0] = 0;

				// call vote button
				static CButtonContainer s_CallVoteButton;
				if(DoButton_Menu(&s_CallVoteButton, Localize("Call vote"), 0, &CallVoteButton) || DoCallVote)
				{
					HandleCallvote(s_ControlPage, false);
					m_aCallvoteReason[0] = 0;
				}
			}
		}

		// extended features (only available when authed in rcon)
		if(Authed)
		{
			// background
			Extended.HSplitTop(Spacing, 0, &Extended);
			Extended.HSplitTop(LineHeight, &Bottom, &Extended);
			Extended.HSplitTop(Spacing, 0, &Extended);

			// force vote
			Bottom.VSplitLeft(180.0f, &Button, &Bottom);
			static CButtonContainer s_ForceVoteButton;
			if(DoButton_Menu(&s_ForceVoteButton, Localize("Force vote"), 0, &Button))
			{
				HandleCallvote(s_ControlPage, true);
				m_aCallvoteReason[0] = 0;
			}

			if(s_ControlPage == 0)
			{
				const float FontSize = 14.0f;

				// remove vote
				Bottom.VSplitRight(ColumnWidth, 0, &Button);
				static CButtonContainer s_RemoveVoteButton;
				if(DoButton_Menu(&s_RemoveVoteButton, Localize("Remove"), 0, &Button))
					m_pClient->m_pVoting->RconRemoveVoteOption(m_CallvoteSelectedOption);

				// add vote
				Extended.HSplitTop(LineHeight, &Bottom, &Extended);
				Bottom.VSplitLeft(2*ColumnWidth+Spacing, &Button, &Bottom);
				UI()->DoLabel(&Button, Localize("Vote description:"), FontSize, TEXTALIGN_LEFT);

				Bottom.VSplitLeft(2*Spacing, 0, &Button);
				UI()->DoLabel(&Button, Localize("Vote command:"), FontSize, TEXTALIGN_LEFT);

				static char s_aVoteDescription[VOTE_DESC_LENGTH] = {0};
				static char s_aVoteCommand[VOTE_CMD_LENGTH] = {0};
				Extended.HSplitTop(LineHeight, &Bottom, &Extended);
				Bottom.VSplitRight(ColumnWidth, &Bottom, &Button);
				static CButtonContainer s_AddVoteButton;
				if(DoButton_Menu(&s_AddVoteButton, Localize("Add"), 0, &Button))
					if(s_aVoteDescription[0] != 0 && s_aVoteCommand[0] != 0)
						m_pClient->m_pVoting->RconAddVoteOption(s_aVoteDescription, s_aVoteCommand);

				Bottom.VSplitLeft(2*ColumnWidth+Spacing, &Button, &Bottom);
				static CLineInput s_DescriptionInput(s_aVoteDescription, sizeof(s_aVoteDescription));
				UI()->DoEditBox(&s_DescriptionInput, &Button, FontSize);

				Bottom.VMargin(2*Spacing, &Button);
				static CLineInput s_CommandInput(s_aVoteCommand, sizeof(s_aVoteCommand));
				UI()->DoEditBox(&s_CommandInput, &Button, FontSize);
			}
		}
	}
}
