/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/config.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/localization.h>
#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>

#include "menus.h"


void CMenus::RenderServerbrowserServerList(CUIRect View)
{
	CUIRect Headers;
	CUIRect Status;

	View.HSplitTop(ms_ListheaderHeight, &Headers, &View);
	View.HSplitBottom(28.0f, &View, &Status);

	// split of the scrollbar
	RenderTools()->DrawUIRect(&Headers, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f);
	Headers.VSplitRight(20.0f, &Headers, 0);

	struct CColumn
	{
		int m_ID;
		int m_Sort;
		CLocConstString m_Caption;
		int m_Direction;
		float m_Width;
		int m_Flags;
		CUIRect m_Rect;
		CUIRect m_Spacer;
	};

	enum
	{
		FIXED=1,
		SPACER=2,

		COL_FLAG_LOCK=0,
		COL_FLAG_PURE,
		COL_FLAG_FAV,
		COL_NAME,
		COL_GAMETYPE,
		COL_MAP,
		COL_PLAYERS,
		COL_PING,
		COL_VERSION,
	};

	static CColumn s_aCols[] = {
		{-1,			-1,						" ",		-1, 2.0f, 0, {0}, {0}},
		{COL_FLAG_LOCK,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_PURE,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_FAV,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_NAME,		IServerBrowser::SORT_NAME,		"Name",		0, 300.0f, 0, {0}, {0}},	// Localize - these strings are localized within CLocConstString
		{COL_GAMETYPE,	IServerBrowser::SORT_GAMETYPE,	"Type",		1, 50.0f, 0, {0}, {0}},
		{COL_MAP,		IServerBrowser::SORT_MAP,			"Map", 		1, 100.0f, 0, {0}, {0}},
		{COL_PLAYERS,	IServerBrowser::SORT_NUMPLAYERS,	"Players",	1, 60.0f, 0, {0}, {0}},
		{-1,			-1,						" ",		1, 10.0f, 0, {0}, {0}},
		{COL_PING,		IServerBrowser::SORT_PING,		"Ping",		1, 40.0f, FIXED, {0}, {0}},
	};
	// This is just for scripts/update_localization.py to work correctly (all other strings are already Localize()'d somewhere else). Don't remove!
	// Localize("Type");

	int NumCols = sizeof(s_aCols)/sizeof(CColumn);

	// do layout
	for(int i = 0; i < NumCols; i++)
	{
		if(s_aCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(s_aCols[i].m_Width, &s_aCols[i].m_Rect, &Headers);

			if(i+1 < NumCols)
			{
				//Cols[i].flags |= SPACER;
				Headers.VSplitLeft(2, &s_aCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NumCols-1; i >= 0; i--)
	{
		if(s_aCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(s_aCols[i].m_Width, &Headers, &s_aCols[i].m_Rect);
			Headers.VSplitRight(2, &Headers, &s_aCols[i].m_Spacer);
		}
	}

	for(int i = 0; i < NumCols; i++)
	{
		if(s_aCols[i].m_Direction == 0)
			s_aCols[i].m_Rect = Headers;
	}

	// do headers
	for(int i = 0; i < NumCols; i++)
	{
		if(DoButton_GridHeader(s_aCols[i].m_Caption, s_aCols[i].m_Caption, g_Config.m_BrSort == s_aCols[i].m_Sort, &s_aCols[i].m_Rect))
		{
			if(s_aCols[i].m_Sort != -1)
			{
				if(g_Config.m_BrSort == s_aCols[i].m_Sort)
					g_Config.m_BrSortOrder ^= 1;
				else
					g_Config.m_BrSortOrder = 0;
				g_Config.m_BrSort = s_aCols[i].m_Sort;
			}
		}
	}

	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.15f), 0, 0);

	CUIRect Scroll;
	View.VSplitRight(15, &View, &Scroll);

	int NumServers = ServerBrowser()->NumSortedServers();

	// display important messages in the middle of the screen so no
	// users misses it
	{
		CUIRect MsgBox = View;
		MsgBox.y += View.h/3;

		if(m_ActivePage == PAGE_INTERNET && ServerBrowser()->IsRefreshingMasters())
			UI()->DoLabelScaled(&MsgBox, Localize("Refreshing master servers"), 16.0f, 0);
		else if(!ServerBrowser()->NumServers())
			UI()->DoLabelScaled(&MsgBox, Localize("No servers found"), 16.0f, 0);
		else if(ServerBrowser()->NumServers() && !NumServers)
			UI()->DoLabelScaled(&MsgBox, Localize("No servers match your filter criteria"), 16.0f, 0);
	}

	int Num = (int)(View.h/s_aCols[0].m_Rect.h) + 1;
	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;

	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

	int ScrollNum = NumServers-Num+1;
	if(ScrollNum > 0)
	{
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
			s_ScrollValue -= 3.0f/ScrollNum;
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
			s_ScrollValue += 3.0f/ScrollNum;
	}
	else
		ScrollNum = 0;

	if(m_SelectedIndex > -1)
	{
		for(int i = 0; i < m_NumInputEvents; i++)
		{
			int NewIndex = -1;
			if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
			{
				if(m_aInputEvents[i].m_Key == KEY_DOWN) NewIndex = m_SelectedIndex + 1;
				if(m_aInputEvents[i].m_Key == KEY_UP) NewIndex = m_SelectedIndex - 1;
			}
			if(NewIndex > -1 && NewIndex < NumServers)
			{
				//scroll
				float IndexY = View.y - s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h + NewIndex*s_aCols[0].m_Rect.h;
				int Scroll = View.y > IndexY ? -1 : View.y+View.h < IndexY+s_aCols[0].m_Rect.h ? 1 : 0;
				if(Scroll)
				{
					if(Scroll < 0)
					{
						int NumScrolls = (View.y-IndexY+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
						s_ScrollValue -= (1.0f/ScrollNum)*NumScrolls;
					}
					else
					{
						int NumScrolls = (IndexY+s_aCols[0].m_Rect.h-(View.y+View.h)+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
						s_ScrollValue += (1.0f/ScrollNum)*NumScrolls;
					}
				}

				m_SelectedIndex = NewIndex;

				const CServerInfo *pItem = ServerBrowser()->SortedGet(m_SelectedIndex);
				str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
			}
		}
	}

	if(s_ScrollValue < 0) s_ScrollValue = 0;
	if(s_ScrollValue > 1) s_ScrollValue = 1;

	// set clipping
	UI()->ClipEnable(&View);

	CUIRect OriginalView = View;
	View.y -= s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h;

	int NewSelected = -1;
	int NumPlayers = 0;

	m_SelectedIndex = -1;

	for (int i = 0; i < NumServers; i++)
	{
		const CServerInfo *pItem = ServerBrowser()->SortedGet(i);
		NumPlayers += pItem->m_NumPlayers;
	}

	for (int i = 0; i < NumServers; i++)
	{
		int ItemIndex = i;
		const CServerInfo *pItem = ServerBrowser()->SortedGet(ItemIndex);
		CUIRect Row;
		CUIRect SelectHitBox;

		int Selected = str_comp(pItem->m_aAddress, g_Config.m_UiServerAddress) == 0; //selected_index==ItemIndex;

		View.HSplitTop(17.0f, &Row, &View);
		SelectHitBox = Row;

		if(Selected)
			m_SelectedIndex = i;

		// make sure that only those in view can be selected
		if(Row.y+Row.h > OriginalView.y && Row.y < OriginalView.y+OriginalView.h)
		{
			if(Selected)
			{
				CUIRect r = Row;
				r.Margin(1.5f, &r);
				RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
			}

			// clip the selection
			if(SelectHitBox.y < OriginalView.y) // top
			{
				SelectHitBox.h -= OriginalView.y-SelectHitBox.y;
				SelectHitBox.y = OriginalView.y;
			}
			else if(SelectHitBox.y+SelectHitBox.h > OriginalView.y+OriginalView.h) // bottom
				SelectHitBox.h = OriginalView.y+OriginalView.h-SelectHitBox.y;

			if(UI()->DoButtonLogic(pItem, "", Selected, &SelectHitBox))
			{
				NewSelected = ItemIndex;
			}
		}
		else
		{
			// reset active item, if not visible
			if(UI()->ActiveItem() == pItem)
				UI()->SetActiveItem(0);

			// don't render invisible items
			continue;
		}

		for(int c = 0; c < NumCols; c++)
		{
			CUIRect Button;
			char aTemp[64];
			Button.x = s_aCols[c].m_Rect.x;
			Button.y = Row.y;
			Button.h = Row.h;
			Button.w = s_aCols[c].m_Rect.w;

			int ID = s_aCols[c].m_ID;

			if(ID == COL_FLAG_LOCK)
			{
				if(pItem->m_Flags & SERVER_FLAG_PASSWORD)
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_LOCK, &Button);
			}
			else if(ID == COL_FLAG_PURE)
			{
				if(	str_comp(pItem->m_aGameType, "DM") == 0 ||
					str_comp(pItem->m_aGameType, "TDM") == 0 ||
					str_comp(pItem->m_aGameType, "CTF") == 0)
				{
					// pure server
				}
				else
				{
					// unpure
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_UNPURE, &Button);
				}
			}
			else if(ID == COL_FLAG_FAV)
			{
				if(pItem->m_Favorite)
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART, &Button);
			}
			else if(ID == COL_NAME)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME))
				{
					// highlight the parts that matches
					const char *pStr = str_find_nocase(pItem->m_aName, g_Config.m_BrFilterString);
					if(pStr)
					{
						TextRender()->TextEx(&Cursor, pItem->m_aName, (int)(pStr-pItem->m_aName));
						TextRender()->TextColor(0.4f,0.4f,1.0f,1);
						TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(1,1,1,1);
						TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pItem->m_aName, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pItem->m_aName, -1);
			}
			else if(ID == COL_MAP)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_MAPNAME))
				{
					// highlight the parts that matches
					const char *pStr = str_find_nocase(pItem->m_aMap, g_Config.m_BrFilterString);
					if(pStr)
					{
						TextRender()->TextEx(&Cursor, pItem->m_aMap, (int)(pStr-pItem->m_aMap));
						TextRender()->TextColor(0.4f,0.4f,1.0f,1);
						TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(1,1,1,1);
						TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pItem->m_aMap, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pItem->m_aMap, -1);
			}
			else if(ID == COL_PLAYERS)
			{
				if(g_Config.m_BrFilterSpectators)
					str_format(aTemp, sizeof(aTemp), "%i/%i", pItem->m_NumPlayers, pItem->m_MaxPlayers);
				else
					str_format(aTemp, sizeof(aTemp), "%i/%i", pItem->m_NumClients, pItem->m_MaxClients);
				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_PLAYER))
					TextRender()->TextColor(0.4f,0.4f,1.0f,1);
				UI()->DoLabelScaled(&Button, aTemp, 12.0f, 1);
				TextRender()->TextColor(1,1,1,1);
			}
			else if(ID == COL_PING)
			{
				str_format(aTemp, sizeof(aTemp), "%i", pItem->m_Latency);
				UI()->DoLabelScaled(&Button, aTemp, 12.0f, 1);
			}
			else if(ID == COL_VERSION)
			{
				const char *pVersion = pItem->m_aVersion;
				UI()->DoLabelScaled(&Button, pVersion, 12.0f, 1);
			}
			else if(ID == COL_GAMETYPE)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f*UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
				TextRender()->TextEx(&Cursor, pItem->m_aGameType, -1);
			}

		}
	}

	UI()->ClipDisable();

	if(NewSelected != -1)
	{
		// select the new server
		const CServerInfo *pItem = ServerBrowser()->SortedGet(NewSelected);
		str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
		if(Input()->MouseDoubleClick())
			Client()->Connect(g_Config.m_UiServerAddress);
	}

	RenderTools()->DrawUIRect(&Status, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Status.Margin(5.0f, &Status);

	// render quick search
	CUIRect QuickSearch, Button;
	Status.VSplitLeft(240.0f, &QuickSearch, &Status);
	const char *pLabel = Localize("Quick search:");
	UI()->DoLabelScaled(&QuickSearch, pLabel, 12.0f, -1);
	float w = TextRender()->TextWidth(0, 12.0f, pLabel, -1);
	QuickSearch.VSplitLeft(w, 0, &QuickSearch);
	QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
	QuickSearch.VSplitLeft(240.0f-w-22.0f, &QuickSearch, &Button);
	static float Offset = 0.0f;
	if(DoEditBox(&g_Config.m_BrFilterString, &QuickSearch, g_Config.m_BrFilterString, sizeof(g_Config.m_BrFilterString), 12.0f, &Offset, false, CUI::CORNER_L))
		Client()->ServerBrowserUpdate();

	// clear button
	{
		static int s_ClearButton = 0;
		RenderTools()->DrawUIRect(&Button, vec4(1,1,1,0.33f)*ButtonColorMul(&s_ClearButton), CUI::CORNER_R, 3.0f);
		UI()->DoLabel(&Button, "x", Button.h*ms_FontmodHeight, 0);
		if(UI()->DoButtonLogic(&s_ClearButton, "x", 0, &Button))
		{
			g_Config.m_BrFilterString[0] = 0;
			UI()->SetActiveItem(&g_Config.m_BrFilterString);
			Client()->ServerBrowserUpdate();
		}
	}

	// render status
	char aBuf[128];
	if(ServerBrowser()->IsRefreshing())
		str_format(aBuf, sizeof(aBuf), Localize("%d%% loaded"), ServerBrowser()->LoadingProgression());
	else
		str_format(aBuf, sizeof(aBuf), Localize("%d of %d servers, %d players"), ServerBrowser()->NumSortedServers(), ServerBrowser()->NumServers(), NumPlayers);
	Status.VSplitRight(TextRender()->TextWidth(0, 14.0f, aBuf, -1), 0, &Status);
	UI()->DoLabelScaled(&Status, aBuf, 14.0f, -1);
}

void CMenus::RenderServerbrowserFilters(CUIRect View)
{
	CUIRect ServerFilter = View, FilterHeader;
	const float FontSize = 12.0f;
	ServerFilter.HSplitBottom(42.5f, &ServerFilter, 0);

	// server filter
	ServerFilter.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFilter);
	RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerFilter, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	UI()->DoLabelScaled(&FilterHeader, Localize("Server filter"), FontSize+2.0f, 0);
	CUIRect Button;

	ServerFilter.VSplitLeft(5.0f, 0, &ServerFilter);
	ServerFilter.Margin(3.0f, &ServerFilter);
	ServerFilter.VMargin(5.0f, &ServerFilter);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox(&g_Config.m_BrFilterEmpty, Localize("Has people playing"), g_Config.m_BrFilterEmpty, &Button))
		g_Config.m_BrFilterEmpty ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if(DoButton_CheckBox(&g_Config.m_BrFilterSpectators, Localize("Count players only"), g_Config.m_BrFilterSpectators, &Button))
		g_Config.m_BrFilterSpectators ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox(&g_Config.m_BrFilterFull, Localize("Server not full"), g_Config.m_BrFilterFull, &Button))
		g_Config.m_BrFilterFull ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox(&g_Config.m_BrFilterFriends, Localize("Show friends"), g_Config.m_BrFilterFriends, &Button))
		g_Config.m_BrFilterFriends ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox(&g_Config.m_BrFilterPw, Localize("No password"), g_Config.m_BrFilterPw, &Button))
		g_Config.m_BrFilterPw ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterCompatversion, Localize("Compatible version"), g_Config.m_BrFilterCompatversion, &Button))
		g_Config.m_BrFilterCompatversion ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterPure, Localize("Standard gametype"), g_Config.m_BrFilterPure, &Button))
		g_Config.m_BrFilterPure ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterPureMap, Localize("Standard map"), g_Config.m_BrFilterPureMap, &Button))
		g_Config.m_BrFilterPureMap ^= 1;
	
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterGametypeStrict, Localize("Strict gametype filter"), g_Config.m_BrFilterGametypeStrict, &Button))
		g_Config.m_BrFilterGametypeStrict ^= 1;

	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	UI()->DoLabelScaled(&Button, Localize("Game types:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static float Offset = 0.0f;
	if(DoEditBox(&g_Config.m_BrFilterGametype, &Button, g_Config.m_BrFilterGametype, sizeof(g_Config.m_BrFilterGametype), FontSize, &Offset))
		Client()->ServerBrowserUpdate();

	{
		ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
		CUIRect EditBox;
		Button.VSplitRight(60.0f, &Button, &EditBox);

		UI()->DoLabelScaled(&Button, Localize("Maximum ping:"), FontSize, -1);

		char aBuf[5];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_BrFilterPing);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_BrFilterPing, &EditBox, aBuf, sizeof(aBuf), FontSize, &Offset);
		g_Config.m_BrFilterPing = clamp(str_toint(aBuf), 0, 999);
	}

	// server address
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	UI()->DoLabelScaled(&Button, Localize("Server address:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	static float OffsetAddr = 0.0f;
	if(DoEditBox(&g_Config.m_BrFilterServerAddress, &Button, g_Config.m_BrFilterServerAddress, sizeof(g_Config.m_BrFilterServerAddress), FontSize, &OffsetAddr))
		Client()->ServerBrowserUpdate();

	ServerFilter.HSplitBottom(5.0f, &ServerFilter, 0);
	ServerFilter.HSplitBottom(ms_ButtonHeight-2.0f, &ServerFilter, &Button);
	static int s_ClearButton = 0;
	if(DoButton_Menu(&s_ClearButton, Localize("Reset filter"), 0, &Button))
	{
		g_Config.m_BrFilterFull = 0;
		g_Config.m_BrFilterEmpty = 0;
		g_Config.m_BrFilterPw = 0;
		g_Config.m_BrFilterPing = 999;
		g_Config.m_BrFilterGametype[0] = 0;
		g_Config.m_BrFilterServerAddress[0] = 0;
		g_Config.m_BrFilterCompatversion = 1;
		g_Config.m_BrFilterString[0] = 0;
		g_Config.m_BrFilterPure = 1;
		g_Config.m_BrFilterPureMap = 1;
		g_Config.m_BrFilterGametypeStrict = 0;
		Client()->ServerBrowserUpdate();
	}
}

void CMenus::RenderServerbrowserServerDetail(CUIRect View)
{
	CUIRect ServerDetails = View;
	CUIRect ServerScoreBoard, ServerHeader;

	const CServerInfo *pSelectedServer = ServerBrowser()->SortedGet(m_SelectedIndex);

	// split off a piece to use for scoreboard
	ServerDetails.HSplitTop(90.0f, &ServerDetails, &ServerScoreBoard);
	ServerDetails.HSplitBottom(2.5f, &ServerDetails, 0x0);

	// server details
	CTextCursor Cursor;
	const float FontSize = 12.0f;
	ServerDetails.HSplitTop(ms_ListheaderHeight, &ServerHeader, &ServerDetails);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerDetails, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	UI()->DoLabelScaled(&ServerHeader, Localize("Server details"), FontSize+2.0f, 0);

	if (pSelectedServer)
	{
		ServerDetails.VSplitLeft(5.0f, 0, &ServerDetails);
		ServerDetails.Margin(3.0f, &ServerDetails);

		CUIRect Row;
		static CLocConstString s_aLabels[] = {
			"Version",	// Localize - these strings are localized within CLocConstString
			"Game type",
			"Ping"};

		CUIRect LeftColumn;
		CUIRect RightColumn;

		//
		{
			CUIRect Button;
			ServerDetails.HSplitBottom(20.0f, &ServerDetails, &Button);
			Button.VSplitLeft(5.0f, 0, &Button);
			static int s_AddFavButton = 0;
			if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), pSelectedServer->m_Favorite, &Button))
			{
				if(pSelectedServer->m_Favorite)
					ServerBrowser()->RemoveFavorite(pSelectedServer->m_NetAddr);
				else
					ServerBrowser()->AddFavorite(pSelectedServer->m_NetAddr);
			}
		}

		ServerDetails.VSplitLeft(5.0f, 0x0, &ServerDetails);
		ServerDetails.VSplitLeft(80.0f, &LeftColumn, &RightColumn);

		for (unsigned int i = 0; i < sizeof(s_aLabels) / sizeof(s_aLabels[0]); i++)
		{
			LeftColumn.HSplitTop(15.0f, &Row, &LeftColumn);
			UI()->DoLabelScaled(&Row, s_aLabels[i], FontSize, -1);
		}

		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Row.w;
		TextRender()->TextEx(&Cursor, pSelectedServer->m_aVersion, -1);

		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Row.w;
		TextRender()->TextEx(&Cursor, pSelectedServer->m_aGameType, -1);

		char aTemp[16];
		str_format(aTemp, sizeof(aTemp), "%d", pSelectedServer->m_Latency);
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Row.w;
		TextRender()->TextEx(&Cursor, aTemp, -1);

	}

	// server scoreboard
	ServerScoreBoard.HSplitBottom(20.0f, &ServerScoreBoard, 0x0);
	ServerScoreBoard.HSplitTop(ms_ListheaderHeight, &ServerHeader, &ServerScoreBoard);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerScoreBoard, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	UI()->DoLabelScaled(&ServerHeader, Localize("Scoreboard"), FontSize+2.0f, 0);

	if (pSelectedServer)
	{
		ServerScoreBoard.Margin(3.0f, &ServerScoreBoard);
		for (int i = 0; i < pSelectedServer->m_NumClients; i++)
		{
			CUIRect Name, Clan, Score, Flag;
			ServerScoreBoard.HSplitTop(25.0f, &Name, &ServerScoreBoard);
			RenderTools()->DrawUIRect(&Name, vec4(1,1,1,(i%2+1)*0.05f), CUI::CORNER_ALL, 4.0f);
			Name.VSplitLeft(5.0f, 0, &Name);
			Name.VSplitLeft(30.0f, &Score, &Name);
			Name.VSplitRight(34.0f, &Name, &Flag);
			Flag.HMargin(4.0f, &Flag);
			Name.HSplitTop(11.0f, &Name, &Clan);

			// score
			if(pSelectedServer->m_aClients[i].m_Player)
			{
				char aTemp[16];
				str_format(aTemp, sizeof(aTemp), "%d", pSelectedServer->m_aClients[i].m_Score);
				TextRender()->SetCursor(&Cursor, Score.x, Score.y+(Score.h-FontSize)/4.0f, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Score.w;
				TextRender()->TextEx(&Cursor, aTemp, -1);
			}

			// name
			TextRender()->SetCursor(&Cursor, Name.x, Name.y, FontSize-2, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Name.w;
			const char *pName = pSelectedServer->m_aClients[i].m_aName;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pName, (int)(s-pName));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pName, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pName, -1);

			// clan
			TextRender()->SetCursor(&Cursor, Clan.x, Clan.y, FontSize-2, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Clan.w;
			const char *pClan = pSelectedServer->m_aClients[i].m_aClan;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pClan, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pClan, (int)(s-pClan));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pClan, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pClan, -1);

			// flag
			Graphics()->TextureSet(m_pClient->m_pCountryFlags->Get(pSelectedServer->m_aClients[i].m_Country)->m_Texture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
			IGraphics::CQuadItem QuadItem(Flag.x, Flag.y, Flag.w, Flag.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
	}
}

void CMenus::RenderServerbrowserFriends(CUIRect View)
{
	CUIRect ServerFriends = View, FilterHeader;
	const float FontSize = 12.0f;

	// header
	ServerFriends.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFriends);
	RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerFriends, vec4(0,0,0,0.15f), 0, 4.0f);
	UI()->DoLabelScaled(&FilterHeader, Localize("Friends"), FontSize+2.0f, 0);
	CUIRect Button, List;

	ServerFriends.VSplitLeft(5.0f, 0, &ServerFriends);
	ServerFriends.Margin(3.0f, &ServerFriends);
	ServerFriends.VMargin(5.0f, &ServerFriends);
	ServerFriends.HSplitBottom(100.0f, &List, &ServerFriends);

	// friends list(remove friend)
	static int s_FriendList = 0;
	static float s_ScrollValue = 0;
	UiDoListboxStart(&s_FriendList, &List, 40.0f, "", "", m_pClient->Friends()->NumFriends(), 1, m_FriendlistSelectedIndex, s_ScrollValue);

	for(int i = 0; i < m_pClient->Friends()->NumFriends(); ++i)
	{
		const CFriendInfo *pFriend = m_pClient->Friends()->GetFriend(i);
		CListboxItem Item = UiDoListboxNextItem(pFriend);

		if(Item.m_Visible)
		{
			Item.m_Rect.Margin(2.5f, &Item.m_Rect);
			RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1.0f, 1.0f, 1.0f, 0.1f), CUI::CORNER_ALL, 4.0f);
			Item.m_Rect.Margin(2.5f, &Item.m_Rect);
			Item.m_Rect.HSplitTop(14.0f, &Item.m_Rect, &Button);
			UI()->DoLabelScaled(&Item.m_Rect, pFriend->m_aName, FontSize, -1);
			UI()->DoLabelScaled(&Button, pFriend->m_aClan, FontSize, -1);
		}
	}

	m_FriendlistSelectedIndex = UiDoListboxEnd(&s_ScrollValue, 0);

	ServerFriends.HSplitTop(2.5f, 0, &ServerFriends);
	ServerFriends.HSplitTop(20.0f, &Button, &ServerFriends);
	if(m_FriendlistSelectedIndex != -1)
	{
		static int s_RemoveButton = 0;
		if(DoButton_Menu(&s_RemoveButton, Localize("Remove"), 0, &Button))
			m_Popup = POPUP_REMOVE_FRIEND;
	}

	// add friend
	if(m_pClient->Friends()->NumFriends() < IFriends::MAX_FRIENDS)
	{
		ServerFriends.HSplitTop(10.0f, 0, &ServerFriends);
		ServerFriends.HSplitTop(19.0f, &Button, &ServerFriends);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
		UI()->DoLabelScaled(&Button, aBuf, FontSize, -1);
		Button.VSplitLeft(80.0f, 0, &Button);
		static char s_aName[MAX_NAME_LENGTH] = {0};
		static float s_OffsetName = 0.0f;
		DoEditBox(&s_aName, &Button, s_aName, sizeof(s_aName), FontSize, &s_OffsetName);

		ServerFriends.HSplitTop(3.0f, 0, &ServerFriends);
		ServerFriends.HSplitTop(19.0f, &Button, &ServerFriends);
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
		UI()->DoLabelScaled(&Button, aBuf, FontSize, -1);
		Button.VSplitLeft(80.0f, 0, &Button);
		static char s_aClan[MAX_CLAN_LENGTH] = {0};
		static float s_OffsetClan = 0.0f;
		DoEditBox(&s_aClan, &Button, s_aClan, sizeof(s_aClan), FontSize, &s_OffsetClan);

		ServerFriends.HSplitTop(3.0f, 0, &ServerFriends);
		ServerFriends.HSplitTop(20.0f, &Button, &ServerFriends);
		static int s_RemoveButton = 0;
		if(DoButton_Menu(&s_RemoveButton, Localize("Add Friend"), 0, &Button))
		{
			m_pClient->Friends()->AddFriend(s_aName, s_aClan);
			Client()->ServerBrowserUpdate();
		}
	}
}

void CMenus::RenderServerbrowser(CUIRect MainView)
{
	/*
		+-----------------+	+-------+
		|				  |	|		|
		|				  |	| tool	|
		|   server list	  |	| box 	|
		|				  |	|	  	|
		|				  |	|		|
		+-----------------+	|	 	|
			status box	tab	+-------+
	*/

	CUIRect ServerList, ToolBox, StatusBox, TabBar;

	// background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	MainView.Margin(10.0f, &MainView);

	// create server list, status box, tab bar and tool box area
	MainView.VSplitRight(205.0f, &ServerList, &ToolBox);
	ServerList.HSplitBottom(70.0f, &ServerList, &StatusBox);
	StatusBox.VSplitRight(100.0f, &StatusBox, &TabBar);
	ServerList.VSplitRight(5.0f, &ServerList, 0);

	// server list
	{
		RenderServerbrowserServerList(ServerList);
	}

	int ToolboxPage = g_Config.m_UiToolboxPage;

	// tab bar
	{
		CUIRect TabButton0, TabButton1, TabButton2;
		TabBar.HSplitTop(5.0f, 0, &TabBar);
		TabBar.HSplitTop(20.0f, &TabButton0, &TabBar);
		TabBar.HSplitTop(2.5f, 0, &TabBar);
		TabBar.HSplitTop(20.0f, &TabButton1, &TabBar);
		TabBar.HSplitTop(2.5f, 0, &TabBar);
		TabBar.HSplitTop(20.0f, &TabButton2, 0);
		vec4 Active = ms_ColorTabbarActive;
		vec4 InActive = ms_ColorTabbarInactive;
		ms_ColorTabbarActive = vec4(0.0f, 0.0f, 0.0f, 0.3f);
		ms_ColorTabbarInactive = vec4(0.0f, 0.0f, 0.0f, 0.15f);

		static int s_FiltersTab = 0;
		if (DoButton_MenuTab(&s_FiltersTab, Localize("Filter"), ToolboxPage==0, &TabButton0, CUI::CORNER_L))
			ToolboxPage = 0;

		static int s_InfoTab = 0;
		if (DoButton_MenuTab(&s_InfoTab, Localize("Info"), ToolboxPage==1, &TabButton1, CUI::CORNER_L))
			ToolboxPage = 1;

		static int s_FriendsTab = 0;
		if (DoButton_MenuTab(&s_FriendsTab, Localize("Friends"), ToolboxPage==2, &TabButton2, CUI::CORNER_L))
			ToolboxPage = 2;

		ms_ColorTabbarActive = Active;
		ms_ColorTabbarInactive = InActive;
		g_Config.m_UiToolboxPage = ToolboxPage;
	}

	// tool box
	{
		RenderTools()->DrawUIRect(&ToolBox, vec4(0.0f, 0.0f, 0.0f, 0.15f), CUI::CORNER_T, 4.0f);


		if(ToolboxPage == 0)
			RenderServerbrowserFilters(ToolBox);
		else if(ToolboxPage == 1)
			RenderServerbrowserServerDetail(ToolBox);
		else if(ToolboxPage == 2)
			RenderServerbrowserFriends(ToolBox);
	}

	// status box
	{
		CUIRect Button, ButtonArea;
		StatusBox.HSplitTop(5.0f, 0, &StatusBox);

		// version note
		StatusBox.HSplitBottom(15.0f, &StatusBox, &Button);
		char aBuf[64];
		if(str_comp(Client()->LatestVersion(), "0") != 0)
		{
			str_format(aBuf, sizeof(aBuf), Localize("Teeworlds %s is out! Download it at www.teeworlds.com!"), Client()->LatestVersion());
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		}
		else
			str_format(aBuf, sizeof(aBuf), Localize("Current version: %s"), GAME_VERSION);
		UI()->DoLabelScaled(&Button, aBuf, 14.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		// button area
		StatusBox.VSplitRight(80.0f, &StatusBox, 0);
		StatusBox.VSplitRight(170.0f, &StatusBox, &ButtonArea);
		ButtonArea.VSplitRight(150.0f, 0, &ButtonArea);
		ButtonArea.HSplitTop(20.0f, &Button, &ButtonArea);
		Button.VMargin(2.0f, &Button);

		static int s_RefreshButton = 0;
		if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Button))
		{
			if(g_Config.m_UiPage == PAGE_INTERNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			else if(g_Config.m_UiPage == PAGE_LAN)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			else if(g_Config.m_UiPage == PAGE_FAVORITES)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
		}

		ButtonArea.HSplitTop(5.0f, 0, &ButtonArea);
		ButtonArea.HSplitTop(20.0f, &Button, &ButtonArea);
		Button.VMargin(2.0f, &Button);

		static int s_JoinButton = 0;
		if(DoButton_Menu(&s_JoinButton, Localize("Connect"), 0, &Button) || m_EnterPressed)
		{
			Client()->Connect(g_Config.m_UiServerAddress);
			m_EnterPressed = false;
		}

		// address info
		StatusBox.VSplitLeft(20.0f, 0, &StatusBox);
		StatusBox.HSplitTop(20.0f, &Button, &StatusBox);
		UI()->DoLabelScaled(&Button, Localize("Host address"), 14.0f, -1);
		StatusBox.HSplitTop(20.0f, &Button, 0);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_UiServerAddress, &Button, g_Config.m_UiServerAddress, sizeof(g_Config.m_UiServerAddress), 14.0f, &Offset);
	}
}

void CMenus::ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() && g_Config.m_UiPage == PAGE_FAVORITES && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
		((CMenus *)pUserData)->ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
}
