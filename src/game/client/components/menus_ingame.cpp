/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/config.h>
#include <engine/demo.h>
#include <engine/friends.h>
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
	CSwitchTeamInfo Info = { 0 };
	GetSwitchTeamInfo(&Info);
	CUIRect Button, ButtonRow, Label;

	float Spacing = 3.0f;
	float ButtonWidth = (MainView.w/6.0f)-(Spacing*5.0)/6.0f;
	
	// cut view
	MainView.HSplitTop(20.0f, 0, &MainView);
	float NoteHeight = !Info.m_aNotification[0] ? 0.0f : 45.0f;
	MainView.HSplitTop(20.0f+20.0f+2*Spacing+ NoteHeight, &MainView, 0);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, ms_BackgroundAlpha), CUI::CORNER_ALL, 5.0f);

	// game options
	MainView.HSplitTop(20.0f, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Game options"), 20.0f*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);

	if(Info.m_aNotification[0] != 0)
	{
		// print notice
		CUIRect Bar;
		MainView.HSplitBottom(NoteHeight, &MainView, &Bar);
		Bar.HMargin(15.0f, &Bar);
		UI()->DoLabelScaled(&Bar, Info.m_aNotification, 14.0f, CUI::ALIGN_CENTER);
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
			if(DoButton_Menu(&s_RedButton, aBuf, Team == TEAM_RED, &Button, 0, CUI::CORNER_ALL, 5.0f, 0.0f, vec4(0.975f, 0.17f, 0.17f, 0.75f), false) && Team != TEAM_RED && !(Info.m_aNotification[0]) && !BlockRed)
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
			if(DoButton_Menu(&s_BlueButton, aBuf, Team == TEAM_BLUE, &Button, 0, CUI::CORNER_ALL, 5.0f, 0.0f, vec4(0.17f, 0.46f, 0.975f, 0.75f), false) && Team != TEAM_BLUE && !(Info.m_aNotification[0]) && !BlockBlue)
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
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, ms_BackgroundAlpha), CUI::CORNER_ALL, 5.0f);

	// player options
	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Player options"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);

	// scroll
	CUIRect Scroll;
	int NumClients = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(i != m_pClient->m_LocalClientID && m_pClient->m_aClients[i].m_Active)
			NumClients++;
	float Length = ButtonHeight * NumClients;
	static float s_ScrollValue = 0.0f;
	int ScrollNum = (int)((Length - MainView.h)/20.0f)+1;
	if(ScrollNum > 0)
	{
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP) && UI()->MouseInside(&MainView))
			s_ScrollValue = clamp(s_ScrollValue - 3.0f/ScrollNum, 0.0f, 1.0f);
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) && UI()->MouseInside(&MainView))
			s_ScrollValue = clamp(s_ScrollValue + 3.0f / ScrollNum, 0.0f, 1.0f);
	}
	MainView.VSplitRight(20.0f, &MainView, &Scroll);
	Scroll.HSplitTop(ButtonHeight, 0, &Scroll);
	RenderTools()->DrawUIRect(&Scroll, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);
	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollValue, &Scroll, s_ScrollValue);

	// headline
	MainView.HSplitTop(ButtonHeight, &Row, &MainView);
	Row.VSplitLeft(ButtonHeight+Spacing, 0, &Row);
	Row.VSplitLeft(NameWidth, &Label, &Row);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Player"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

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

	// background
	RenderTools()->DrawUIRect(&MainView, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);
	MainView.Margin(5.0f, &MainView);

	UI()->ClipEnable(&MainView);
	if(Length > MainView.h)
		MainView.y += (MainView.h - Length) * s_ScrollValue;

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
			if(Count++ % 2 == 0)
				RenderTools()->DrawUIRect(&Row, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

			// player info
			Row.VSplitLeft(ButtonHeight, &Label, &Row);
			Label.y += 2.0f;
			CTeeRenderInfo Info = m_pClient->m_aClients[i].m_RenderInfo;
			Info.m_Size = Label.h;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Label.x + Label.h / 2, Label.y + Label.h / 2));

			Row.VSplitLeft(2*Spacing, 0, &Row);
			Row.VSplitLeft(NameWidth, &Label, &Row);
			Label.y += 2.0f;
			if(g_Config.m_ClShowUserId)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Label.x, Label.y, ButtonHeight*ms_FontmodHeight*0.8f, TEXTFLAG_RENDER);
				RenderTools()->DrawClientID(TextRender(), &Cursor, i);
				Label.VSplitLeft(ButtonHeight, 0, &Label);
			}
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[i].m_aName : "");
			UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
			Row.VSplitLeft(Spacing, 0, &Row);
			Row.VSplitLeft(ClanWidth, &Label, &Row);
			Label.y += 2.0f;
			str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[i].m_aClan : "");
			UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

			// ignore button
			Row.VSplitRight(ButtonHeight/2, &Row, 0);
			Row.VSplitRight(ButtonHeight, &Row, &Label);
			if(g_Config.m_ClFilterchat == 2 || (g_Config.m_ClFilterchat == 1 && !m_pClient->m_aClients[i].m_Friend))
				DoButton_Toggle(&s_aPlayerIDs[i][0], 1, &Label, false);
			else
				if(DoButton_Toggle(&s_aPlayerIDs[i][0], m_pClient->m_aClients[i].m_ChatIgnore, &Label, true))
					m_pClient->m_aClients[i].m_ChatIgnore ^= 1;

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

	UI()->ClipDisable();
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
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, ms_BackgroundAlpha), CUI::CORNER_ALL, 5.0f);

	CUIRect ServerInfo, GameInfo, Motd, Label;

	char aBuf[128] = {0};
	const float ButtonHeight = 20.0f;

	// serverinfo
	MainView.HSplitBottom(250.0f, &ServerInfo, &Motd);
	ServerInfo.VSplitMid(&ServerInfo, &GameInfo);
	ServerInfo.VSplitRight(1.0f, &ServerInfo, 0);
	RenderTools()->DrawUIRect(&ServerInfo, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Server info"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	RenderTools()->DrawUIRect(&ServerInfo, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);
	ServerInfo.Margin(5.0f, &ServerInfo);
	
	ServerInfo.HSplitTop(2*ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, CurrentServerInfo.m_aName, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT, Label.w);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Address"), g_Config.m_UiServerAddress);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
	
	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Ping"), m_pClient->m_Snap.m_pLocalInfo->m_Latency);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Version"), CurrentServerInfo.m_aVersion);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	ServerInfo.HSplitTop(ButtonHeight, &Label, &ServerInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Password"), CurrentServerInfo.m_Flags&IServerBrowser::FLAG_PASSWORD ? Localize("Yes", "With") : Localize("No", "Without/None"));
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
	
	const bool IsFavorite = ServerBrowser()->IsFavorite(CurrentServerInfo.m_NetAddr);
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
		if(DoButton_CheckBox(&s_MuteBroadcast, Localize("Mute broadcasts"),
							 m_pClient->m_MuteServerBroadcast, &Button))
		{
			m_pClient->m_MuteServerBroadcast ^= 1;
		}
	}

	// gameinfo
	GameInfo.VSplitLeft(1.0f, 0, &GameInfo);
	RenderTools()->DrawUIRect(&GameInfo, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Game info"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	RenderTools()->DrawUIRect(&GameInfo, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);
	GameInfo.Margin(5.0f, &GameInfo);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Game type"), CurrentServerInfo.m_aGameType);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Map"), CurrentServerInfo.m_aMap);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Difficulty"), (CurrentServerInfo.m_ServerLevel == 0) ? Localize("Casual", "Server difficulty") : 
		(CurrentServerInfo.m_ServerLevel == 1 ? Localize("Normal", "Server difficulty") : Localize("Competitive", "Server difficulty")));
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_GameInfo.m_ScoreLimit);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	GameInfo.HSplitTop(ButtonHeight, &Label, &GameInfo);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Time limit"), m_pClient->m_GameInfo.m_TimeLimit);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	GameInfo.HSplitBottom(ButtonHeight, &GameInfo, &Label);
	Label.y += 2.0f;
	str_format(aBuf, sizeof(aBuf), "%s: %d/%d", Localize("Players"), m_pClient->m_GameInfo.m_NumPlayers, CurrentServerInfo.m_MaxClients);
	UI()->DoLabel(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	// motd
	Motd.HSplitTop(2.0f, 0, &Motd);
	RenderTools()->DrawUIRect(&Motd, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);

	Motd.HSplitTop(ButtonHeight, &Label, &Motd);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("MOTD"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	RenderTools()->DrawUIRect(&Motd, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 5.0f);
	Motd.Margin(5.0f, &Motd);

	TextRender()->Text(0, Motd.x, Motd.y, ButtonHeight*ms_FontmodHeight*0.8f, m_pClient->m_pMotd->GetMotd(), (int)Motd.w);
}

bool CMenus::RenderServerControlServer(CUIRect MainView)
{
	bool doCallVote = false;
	static int s_VoteList = 0;
	static CListBoxState s_ListBoxState;
	CUIRect List = MainView;
	UiDoListboxHeader(&s_ListBoxState, &List, Localize("Option"), 20.0f, 2.0f);
	UiDoListboxStart(&s_ListBoxState, &s_VoteList, 20.0f, 0, m_pClient->m_pVoting->m_NumVoteOptions, 1, m_CallvoteSelectedOption, 0, true);

	for(CVoteOptionClient *pOption = m_pClient->m_pVoting->m_pFirst; pOption; pOption = pOption->m_pNext)
	{
		CListboxItem Item = UiDoListboxNextItem(&s_ListBoxState, pOption);

		if(Item.m_Visible)
		{			
			Item.m_Rect.VMargin(5.0f, &Item.m_Rect);
			Item.m_Rect.y += 2.0f;
			UI()->DoLabel(&Item.m_Rect, pOption->m_aDescription, Item.m_Rect.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
		}
	}

	m_CallvoteSelectedOption = UiDoListboxEnd(&s_ListBoxState, &doCallVote);
	return doCallVote;
}

void CMenus::RenderServerControlKick(CUIRect MainView, bool FilterSpectators)
{
	int NumOptions = 0;
	int Selected = -1;
	static int aPlayerIDs[MAX_CLIENTS];
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
			aPlayerIDs[NumOptions++] = i;
		}
	}

	const float Spacing = 2.0f;
	const float NameWidth = 250.0f;
	const float ClanWidth = 250.0f;
	static int s_VoteList = 0;
	static CListBoxState s_ListBoxState;
	CUIRect List = MainView;
	UiDoListboxHeader(&s_ListBoxState, &List, Localize("Player"), 20.0f, 2.0f);
	UiDoListboxStart(&s_ListBoxState, &s_VoteList, 20.0f, 0, NumOptions, 1, Selected, 0, true);

	for(int i = 0; i < NumOptions; i++)
	{
		CListboxItem Item = UiDoListboxNextItem(&s_ListBoxState, &aPlayerIDs[i]);

		if(Item.m_Visible)
		{
			CUIRect Label, Row;
			Item.m_Rect.VMargin(5.0f, &Row);

			// player info
			Row.VSplitLeft(Row.h, &Label, &Row);
			Label.y += 2.0f;
			CTeeRenderInfo Info = m_pClient->m_aClients[aPlayerIDs[i]].m_RenderInfo;
			Info.m_Size = Label.h;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Label.x + Label.h / 2, Label.y + Label.h / 2));

			Row.VSplitLeft(2*Spacing, 0, &Row);
			if(g_Config.m_ClShowUserId)
			{
				Row.VSplitLeft(Row.h, &Label, &Row);
				Label.y += 2.0f;
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Label.x, Label.y, Label.h*ms_FontmodHeight*0.8f, TEXTFLAG_RENDER);
				RenderTools()->DrawClientID(TextRender(), &Cursor, aPlayerIDs[i]);
			}

			Row.VSplitLeft(Spacing, 0, &Row);
			Row.VSplitLeft(NameWidth, &Label, &Row);
			Label.y += 2.0f;
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[aPlayerIDs[i]].m_aName : "");
			UI()->DoLabel(&Label, aBuf, Label.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
			Row.VSplitLeft(Spacing, 0, &Row);
			Row.VSplitLeft(ClanWidth, &Label, &Row);
			Label.y += 2.0f;
			str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[aPlayerIDs[i]].m_aClan : "");
			UI()->DoLabel(&Label, aBuf, Label.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
		}
	}

	Selected = UiDoListboxEnd(&s_ListBoxState, 0);
	m_CallvoteSelectedPlayer = Selected != -1 ? aPlayerIDs[Selected] : -1;
}

void CMenus::HandleCallvote(int Page, bool Force)
{
	if(Page == 0)
		m_pClient->m_pVoting->CallvoteOption(m_CallvoteSelectedOption, m_aCallvoteReason, Force);
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

	if(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS)
		pNotification = Localize("Spectators aren't allowed to start a vote.");
	else if(m_pClient->m_pVoting->IsVoting())
		pNotification = Localize("Wait for current vote to end before calling a new one.");
	else if(m_pClient->m_pVoting->CallvoteBlockTime() != 0)
	{
		str_format(aBuf, sizeof(aBuf), Localize("You must wait %d seconds before making another vote"), m_pClient->m_pVoting->CallvoteBlockTime());
		pNotification = aBuf;
	}

	bool Authed = Client()->RconAuthed();
	MainView.HSplitBottom(80.0f, &MainView, 0);
	if(pNotification && !Authed)
	{
		MainView.HSplitTop(20.0f, 0, &MainView);
		// only print notice
		CUIRect Bar;
		MainView.HSplitTop(45.0f, &Bar, &MainView);
		RenderTools()->DrawUIRect(&Bar, vec4(0.0f, 0.0f, 0.0f, 0.25f+ms_BackgroundAlpha), CUI::CORNER_ALL, 5.0f);
		Bar.HMargin(15.0f, &Bar);
		UI()->DoLabelScaled(&Bar, pNotification, 14.0f, CUI::ALIGN_CENTER);
		return;
	}

	// tab bar
	const float NotActiveAlpha = 0.5f;
	CUIRect Bottom, Extended, Button, Row, Note;
	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(25.0f, &Row, &MainView);
	Row.VSplitLeft(Row.w/3-1.5f, &Button, &Row);
	static CButtonContainer s_Button0;
	if(DoButton_MenuTabTop(&s_Button0, Localize("Change settings"), false, &Button, s_ControlPage == 0 ? 1.0f : NotActiveAlpha, 1.0f, CUI::CORNER_T, 5.0f, 0.25f))
		s_ControlPage = 0;

	Row.VSplitLeft(1.5f, 0, &Row);
	Row.VSplitMid(&Button, &Row);
	Button.VMargin(1.5f, &Button);
	static CButtonContainer s_Button1;
	if(DoButton_MenuTabTop(&s_Button1, Localize("Kick player"), false, &Button, s_ControlPage == 1 ? 1.0f : NotActiveAlpha, 1.0f, CUI::CORNER_T, 5.0f, 0.25f))
		s_ControlPage = 1;

	Row.VSplitLeft(1.5f, 0, &Button);
	static CButtonContainer s_Button2;
	if(DoButton_MenuTabTop(&s_Button2, Localize("Move player to spectators"), false, &Button, s_ControlPage == 2 ? 1.0f : NotActiveAlpha, 1.0f, CUI::CORNER_T, 5.0f, 0.25f))
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
		MainView.HSplitTop(20.0f+45.0f, &MainView, 0);
	}
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, ms_BackgroundAlpha), CUI::CORNER_B, 5.0f);
	MainView.HSplitTop(20.0f, 0, &MainView);
	if(pNotification && !Authed)
	{
		// only print notice
		RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		MainView.HMargin(15.0f, &MainView);
		UI()->DoLabelScaled(&MainView, pNotification, 14.0f, CUI::ALIGN_CENTER);
		return;
	}

	// render background
	MainView.HSplitBottom(90.0f+2*20.0f, &MainView, &Extended);
	RenderTools()->DrawUIRect(&Extended, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	
	bool doCallVote = false;
	// render page
	if(s_ControlPage == 0)
		doCallVote = RenderServerControlServer(MainView); // double click triggers vote
	else if(s_ControlPage == 1)
		RenderServerControlKick(MainView, false);
	else if(s_ControlPage == 2)
		RenderServerControlKick(MainView, true);

	// vote menu
	Extended.Margin(5.0f, &Extended);
	Extended.HSplitTop(20.0f, &Note, &Extended);
	Extended.HSplitTop(20.0f, &Bottom, &Extended);
	{
		Bottom.VSplitRight(120.0f, &Bottom, &Button);

		// render kick reason
		CUIRect Reason, ClearButton, Label;
		Bottom.VSplitRight(40.0f, &Bottom, 0);
		Bottom.VSplitRight(160.0f, &Bottom, &Reason);
		Reason.VSplitRight(Reason.h, &Reason, &ClearButton);
		const char *pLabel = Localize("Reason:");
		float w = TextRender()->TextWidth(0, Reason.h*ms_FontmodHeight*0.8f, pLabel, -1);
		Reason.VSplitLeft(w + 10.0f, &Label, &Reason);
		Label.y += 2.0f;
		UI()->DoLabel(&Label, pLabel, Reason.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
		static float s_Offset = 0.0f;
		DoEditBox(&m_aCallvoteReason, &Reason, m_aCallvoteReason, sizeof(m_aCallvoteReason), Reason.h*ms_FontmodHeight*0.8f, &s_Offset, false, CUI::CORNER_L);

		// clear button
		{
			static CButtonContainer s_ClearButton;
			float Fade = ButtonFade(&s_ClearButton, 0.6f);
			RenderTools()->DrawUIRect(&ClearButton, vec4(1.0f, 1.0f, 1.0f, 0.33f+(Fade/0.6f)*0.165f), CUI::CORNER_R, 3.0f);
			Label = ClearButton;
			Label.y += 2.0f;
			UI()->DoLabel(&Label, "x", Label.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
			if(UI()->DoButtonLogic(s_ClearButton.GetID(), "x", 0, &ClearButton))
				m_aCallvoteReason[0] = 0;
		}

		if(pNotification == 0)
		{
			// call vote
			static CButtonContainer s_CallVoteButton;
			if(DoButton_Menu(&s_CallVoteButton, Localize("Call vote"), 0, &Button) || doCallVote)
				HandleCallvote(s_ControlPage, false);
		}
		else
		{
			// print notice
			UI()->DoLabel(&Note, pNotification, Note.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT, Note.w);
		}

		// extended features (only available when authed in rcon)
		if(Authed)
		{
			// background
			Extended.Margin(10.0f, &Extended);
			Extended.HSplitTop(20.0f, &Bottom, &Extended);
			Extended.HSplitTop(5.0f, 0, &Extended);

			// force vote
			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(120.0f, &Button, &Bottom);
			static CButtonContainer s_ForceVoteButton;
			if(DoButton_Menu(&s_ForceVoteButton, Localize("Force vote"), 0, &Button))
				HandleCallvote(s_ControlPage, true);

			if(s_ControlPage == 0)
			{
				// remove vote
				Bottom.VSplitRight(10.0f, &Bottom, 0);
				Bottom.VSplitRight(120.0f, 0, &Button);
				static CButtonContainer s_RemoveVoteButton;
				if(DoButton_Menu(&s_RemoveVoteButton, Localize("Remove"), 0, &Button))
					m_pClient->m_pVoting->RemovevoteOption(m_CallvoteSelectedOption);


				// add vote
				Extended.HSplitTop(20.0f, &Bottom, &Extended);
				Bottom.VSplitLeft(5.0f, 0, &Bottom);
				Bottom.VSplitLeft(250.0f, &Button, &Bottom);
				UI()->DoLabelScaled(&Button, Localize("Vote description:"), 14.0f, CUI::ALIGN_LEFT);

				Bottom.VSplitLeft(20.0f, 0, &Button);
				UI()->DoLabelScaled(&Button, Localize("Vote command:"), 14.0f, CUI::ALIGN_LEFT);

				static char s_aVoteDescription[64] = {0};
				static char s_aVoteCommand[512] = {0};
				Extended.HSplitTop(20.0f, &Bottom, &Extended);
				Bottom.VSplitRight(10.0f, &Bottom, 0);
				Bottom.VSplitRight(120.0f, &Bottom, &Button);
				static CButtonContainer s_AddVoteButton;
				if(DoButton_Menu(&s_AddVoteButton, Localize("Add"), 0, &Button))
					if(s_aVoteDescription[0] != 0 && s_aVoteCommand[0] != 0)
						m_pClient->m_pVoting->AddvoteOption(s_aVoteDescription, s_aVoteCommand);

				Bottom.VSplitLeft(5.0f, 0, &Bottom);
				Bottom.VSplitLeft(250.0f, &Button, &Bottom);
				static float s_OffsetDesc = 0.0f;
				DoEditBox(&s_aVoteDescription, &Button, s_aVoteDescription, sizeof(s_aVoteDescription), 14.0f, &s_OffsetDesc, false, CUI::CORNER_ALL);

				Bottom.VMargin(20.0f, &Button);
				static float s_OffsetCmd = 0.0f;
				DoEditBox(&s_aVoteCommand, &Button, s_aVoteCommand, sizeof(s_aVoteCommand), 14.0f, &s_OffsetCmd, false, CUI::CORNER_ALL);
			}
		}
	}
}

