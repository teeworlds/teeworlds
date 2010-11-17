#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include "menus.h"
#include <game/localization.h>
#include <game/version.h>

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
		int m_Id;
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
			UI()->DoLabel(&MsgBox, Localize("Refreshing master servers"), 16.0f, 0);
		else if(!ServerBrowser()->NumServers())
			UI()->DoLabel(&MsgBox, Localize("No servers found"), 16.0f, 0);
		else if(ServerBrowser()->NumServers() && !NumServers)
			UI()->DoLabel(&MsgBox, Localize("No servers match your filter criteria"), 16.0f, 0);
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
			s_ScrollValue -= 1.0f/ScrollNum;
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
			s_ScrollValue += 1.0f/ScrollNum;
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

			int Id = s_aCols[c].m_Id;

			if(Id == COL_FLAG_LOCK)
			{
				if(pItem->m_Flags & SERVER_FLAG_PASSWORD)
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_LOCK, &Button);
			}
			else if(Id == COL_FLAG_PURE)
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
			else if(Id == COL_FLAG_FAV)
			{
				if(pItem->m_Favorite)
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART, &Button);
			}
			else if(Id == COL_NAME)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME))
				{
					// highlight the parts that matches
					const char *s = str_find_nocase(pItem->m_aName, g_Config.m_BrFilterString);
					if(s)
					{
						TextRender()->TextEx(&Cursor, pItem->m_aName, (int)(s-pItem->m_aName));
						TextRender()->TextColor(0.4f,0.4f,1.0f,1);
						TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(1,1,1,1);
						TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pItem->m_aName, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pItem->m_aName, -1);
			}
			else if(Id == COL_MAP)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
				TextRender()->TextEx(&Cursor, pItem->m_aMap, -1);
			}
			else if(Id == COL_PLAYERS)
			{
				str_format(aTemp, sizeof(aTemp), "%i/%i", pItem->m_NumPlayers, pItem->m_MaxPlayers);
				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_PLAYERNAME))
					TextRender()->TextColor(0.4f,0.4f,1.0f,1);
				UI()->DoLabel(&Button, aTemp, 12.0f, 1);
				TextRender()->TextColor(1,1,1,1);
			}
			else if(Id == COL_PING)
			{
				str_format(aTemp, sizeof(aTemp), "%i", pItem->m_Latency);
				UI()->DoLabel(&Button, aTemp, 12.0f, 1);
			}
			else if(Id == COL_VERSION)
			{
				const char *pVersion = pItem->m_aVersion;
				if(str_comp(pVersion, "0.3 e2d7973c6647a13c") == 0) // TODO: remove me later on
					pVersion = "0.3.0";
				UI()->DoLabel(&Button, pVersion, 12.0f, 1);
			}
			else if(Id == COL_GAMETYPE)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
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
	Status.VSplitLeft(260.0f, &QuickSearch, &Status);
	const char *pLabel = Localize("Quick search:");
	UI()->DoLabel(&QuickSearch, pLabel, 12.0f, -1);
	float w = TextRender()->TextWidth(0, 12.0f, pLabel, -1);
	QuickSearch.VSplitLeft(w, 0, &QuickSearch);
	QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
	QuickSearch.VSplitLeft(260.0f-w-22.0f, &QuickSearch, &Button);
	static float Offset = 0.0f;
	DoEditBox(&g_Config.m_BrFilterString, &QuickSearch, g_Config.m_BrFilterString, sizeof(g_Config.m_BrFilterString), 12.0f, &Offset, false, CUI::CORNER_L);
	// clear button
	{
		static int s_ClearButton = 0;
		RenderTools()->DrawUIRect(&Button, vec4(1,1,1,0.33f)*ButtonColorMul(&s_ClearButton), CUI::CORNER_R, 3.0f);
		UI()->DoLabel(&Button, "x", Button.h*ms_FontmodHeight, 0);
		if(UI()->DoButtonLogic(&s_ClearButton, "x", 0, &Button))
		{
			g_Config.m_BrFilterString[0] = 0;
			UI()->SetActiveItem(&g_Config.m_BrFilterString);
		}
	}
	
	// render status
	char aBuf[128];
	if(ServerBrowser()->IsRefreshing())
		str_format(aBuf, sizeof(aBuf), Localize("%d%% loaded"), ServerBrowser()->LoadingProgression());
	else
		str_format(aBuf, sizeof(aBuf), Localize("%d of %d servers, %d players"), ServerBrowser()->NumSortedServers(), ServerBrowser()->NumServers(), NumPlayers);
	Status.VSplitRight(TextRender()->TextWidth(0, 14.0f, aBuf, -1), 0, &Status);
	UI()->DoLabel(&Status, aBuf, 14.0f, -1);
}

void CMenus::RenderServerbrowserFilters(CUIRect View)
{
	// filters
	CUIRect Button;

	View.HSplitTop(5.0f, 0, &View);
	View.VSplitLeft(5.0f, 0, &View);
	View.VSplitRight(5.0f, &View, 0);
	View.HSplitBottom(5.0f, &View, 0);

	// render filters
	View.HSplitTop(20.0f, &Button, &View);
	if (DoButton_CheckBox(&g_Config.m_BrFilterEmpty, Localize("Has people playing"), g_Config.m_BrFilterEmpty, &Button))
		g_Config.m_BrFilterEmpty ^= 1;

	View.HSplitTop(20.0f, &Button, &View);
	if (DoButton_CheckBox(&g_Config.m_BrFilterFull, Localize("Server not full"), g_Config.m_BrFilterFull, &Button))
		g_Config.m_BrFilterFull ^= 1;

	View.HSplitTop(20.0f, &Button, &View);
	if (DoButton_CheckBox(&g_Config.m_BrFilterPw, Localize("No password"), g_Config.m_BrFilterPw, &Button))
		g_Config.m_BrFilterPw ^= 1;

	View.HSplitTop(20.0f, &Button, &View);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterCompatversion, Localize("Compatible version"), g_Config.m_BrFilterCompatversion, &Button))
		g_Config.m_BrFilterCompatversion ^= 1;

	View.HSplitTop(20.0f, &Button, &View);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterPure, Localize("Standard gametype"), g_Config.m_BrFilterPure, &Button))
		g_Config.m_BrFilterPure ^= 1;

	View.HSplitTop(20.0f, &Button, &View);
	//button.VSplitLeft(20.0f, 0, &button);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterPureMap, Localize("Standard map"), g_Config.m_BrFilterPureMap, &Button))
		g_Config.m_BrFilterPureMap ^= 1;
	
	View.HSplitTop(5.0f, 0, &View);

	View.HSplitTop(19.0f, &Button, &View);
	UI()->DoLabel(&Button, Localize("Game types:"), 12.0f, -1);
	Button.VSplitLeft(95.0f, 0, &Button);
	View.HSplitTop(3.0f, 0, &View);
	static float Offset = 0.0f;
	DoEditBox(&g_Config.m_BrFilterGametype, &Button, g_Config.m_BrFilterGametype, sizeof(g_Config.m_BrFilterGametype), 12.0f, &Offset);

	{
		View.HSplitTop(19.0f, &Button, &View);
		CUIRect EditBox;
		Button.VSplitRight(50.0f, &Button, &EditBox);
		EditBox.VSplitRight(5.0f, &EditBox, 0);
		
		UI()->DoLabel(&Button, Localize("Maximum ping:"), 12.0f, -1);
		
		char aBuf[5];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_BrFilterPing);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_BrFilterPing, &EditBox, aBuf, sizeof(aBuf), 12.0f, &Offset);
		g_Config.m_BrFilterPing = str_toint(aBuf);
	}

	View.HSplitBottom(ms_ButtonHeight, &View, &Button);
	static int s_ClearButton = 0;
	if(DoButton_Menu(&s_ClearButton, Localize("Reset filter"), 0, &Button))
	{
		g_Config.m_BrFilterFull = 0;
		g_Config.m_BrFilterEmpty = 0;
		g_Config.m_BrFilterPw = 0;
		g_Config.m_BrFilterPing = 999;
		g_Config.m_BrFilterGametype[0] = 0;
		g_Config.m_BrFilterCompatversion = 1;
		g_Config.m_BrFilterString[0] = 0;
		g_Config.m_BrFilterPure = 1;
		g_Config.m_BrFilterPureMap = 1;
	}
}

void CMenus::RenderServerbrowserServerDetail(CUIRect View)
{
	CUIRect ServerDetails = View;
	CUIRect ServerScoreBoard, ServerHeader;

	const CServerInfo *pSelectedServer = ServerBrowser()->SortedGet(m_SelectedIndex);

	//server_details.VSplitLeft(10.0f, 0x0, &server_details);

	// split off a piece to use for scoreboard
	ServerDetails.HSplitTop(140.0f, &ServerDetails, &ServerScoreBoard);
	ServerDetails.HSplitBottom(10.0f, &ServerDetails, 0x0);

	// server details
	const float FontSize = 12.0f;
	ServerDetails.HSplitTop(20.0f, &ServerHeader, &ServerDetails);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerDetails, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	ServerHeader.VSplitLeft(8.0f, 0x0, &ServerHeader);
	UI()->DoLabel(&ServerHeader, Localize("Server details"), FontSize+2.0f, -1);

	ServerDetails.VSplitLeft(5.0f, 0x0, &ServerDetails);

	ServerDetails.Margin(3.0f, &ServerDetails);

	if (pSelectedServer)
	{
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
			static int s_AddFavButton = 0;
			if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), pSelectedServer->m_Favorite, &Button))
			{
				if(pSelectedServer->m_Favorite)
					ServerBrowser()->RemoveFavorite(pSelectedServer->m_NetAddr);
				else
					ServerBrowser()->AddFavorite(pSelectedServer->m_NetAddr);
			}
		}
		//UI()->DoLabel(&row, temp, font_size, -1);

		ServerDetails.VSplitLeft(5.0f, 0x0, &ServerDetails);
		ServerDetails.VSplitLeft(80.0f, &LeftColumn, &RightColumn);

		for (unsigned int i = 0; i < sizeof(s_aLabels) / sizeof(s_aLabels[0]); i++)
		{
			LeftColumn.HSplitTop(15.0f, &Row, &LeftColumn);
			UI()->DoLabel(&Row, s_aLabels[i], FontSize, -1);
		}

		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabel(&Row, pSelectedServer->m_aVersion, FontSize, -1);

		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabel(&Row, pSelectedServer->m_aGameType, FontSize, -1);

		char aTemp[16];
		str_format(aTemp, sizeof(aTemp), "%d", pSelectedServer->m_Latency);
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabel(&Row, aTemp, FontSize, -1);

	}

	// server scoreboard

	ServerScoreBoard.HSplitBottom(10.0f, &ServerScoreBoard, 0x0);
	ServerScoreBoard.HSplitTop(20.0f, &ServerHeader, &ServerScoreBoard);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerScoreBoard, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	ServerHeader.VSplitLeft(8.0f, 0x0, &ServerHeader);
	UI()->DoLabel(&ServerHeader, Localize("Scoreboard"), FontSize+2.0f, -1);

	ServerScoreBoard.VSplitLeft(5.0f, 0x0, &ServerScoreBoard);

	ServerScoreBoard.Margin(3.0f, &ServerScoreBoard);

	if (pSelectedServer)
	{
		for (int i = 0; i < pSelectedServer->m_NumPlayers; i++)
		{
			CUIRect Row;
			char aTemp[16];
			ServerScoreBoard.HSplitTop(16.0f, &Row, &ServerScoreBoard);

			str_format(aTemp, sizeof(aTemp), "%d", pSelectedServer->m_aPlayers[i].m_Score);
			UI()->DoLabel(&Row, aTemp, FontSize, -1);

			Row.VSplitLeft(25.0f, 0x0, &Row);

			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Row.w;

			const char *pName = pSelectedServer->m_aPlayers[i].m_aName;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pName, (int)(s-pName));
					TextRender()->TextColor(0.4f,0.4f,1,1);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1,1,1,1);
					TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pName, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pName, -1);

		}
	}
}

void CMenus::RenderServerbrowser(CUIRect MainView)
{
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	CUIRect View;
	MainView.Margin(10.0f, &View);

	/*
		+-----------------+ +------+
		|                 | |      |
		|                 | | tool |
		|                 | | box  |
		|                 | |      |
		|                 | +------+
		+-----------------+  button
	      status toolbar      box
	*/


	//CUIRect filters;
	CUIRect StatusToolBox;
	CUIRect ToolBox;
	CUIRect ButtonBox;

	// split off a piece for filters, details and scoreboard
	View.VSplitRight(200.0f, &View, &ToolBox);
	ToolBox.HSplitBottom(80.0f, &ToolBox, &ButtonBox);
	View.HSplitBottom(ms_ButtonHeight+5.0f, &View, &StatusToolBox);

	RenderServerbrowserServerList(View);

	int ToolboxPage = g_Config.m_UiToolboxPage;

	ToolBox.VSplitLeft(5.0f, 0, &ToolBox);

	// do tabbar
	{
		CUIRect TabBar;
		CUIRect TabButton0, TabButton1;
		ToolBox.HSplitTop(22.0f, &TabBar, &ToolBox);

		TabBar.VSplitMid(&TabButton0, &TabButton1);
		//TabButton0.VSplitRight(5.0f, &TabButton0, 0);
		//TabButton1.VSplitLeft(5.0f, 0, &TabButton1);

		static int s_FiltersTab = 0;
		if (DoButton_MenuTab(&s_FiltersTab, Localize("Filter"), ToolboxPage==0, &TabButton0, CUI::CORNER_TL))
			ToolboxPage = 0;

		static int s_InfoTab = 0;
		if (DoButton_MenuTab(&s_InfoTab, Localize("Info"), ToolboxPage==1, &TabButton1, CUI::CORNER_TR))
			ToolboxPage = 1;
	}

	g_Config.m_UiToolboxPage = ToolboxPage;

	RenderTools()->DrawUIRect(&ToolBox, vec4(0,0,0,0.15f), 0, 0);

	ToolBox.HSplitTop(5.0f, 0, &ToolBox);

	if(ToolboxPage == 0)
		RenderServerbrowserFilters(ToolBox);
	else if(ToolboxPage == 1)
		RenderServerbrowserServerDetail(ToolBox);

	{
		StatusToolBox.HSplitTop(5.0f, 0, &StatusToolBox);

		CUIRect Button;
		//buttons.VSplitRight(20.0f, &buttons, &button);
		StatusToolBox.VSplitRight(110.0f, &StatusToolBox, &Button);
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

		char aBuf[512];
		if(str_comp(Client()->LatestVersion(), "0") != 0)
			str_format(aBuf, sizeof(aBuf), Localize("Teeworlds %s is out! Download it at www.teeworlds.com!"), Client()->LatestVersion());
		else
			str_format(aBuf, sizeof(aBuf), Localize("Current version: %s"), GAME_VERSION);
		UI()->DoLabel(&StatusToolBox, aBuf, 14.0f, -1);
	}

	// do the button box
	{

		ButtonBox.VSplitLeft(5.0f, 0, &ButtonBox);
		ButtonBox.VSplitRight(5.0f, &ButtonBox, 0);

		CUIRect Button;
		ButtonBox.HSplitBottom(ms_ButtonHeight, &ButtonBox, &Button);
		Button.VSplitRight(120.0f, 0, &Button);
		Button.VMargin(2.0f, &Button);
		//button.VMargin(2.0f, &button);
		static int s_JoinButton = 0;
		if(DoButton_Menu(&s_JoinButton, Localize("Connect"), 0, &Button) || m_EnterPressed)
		{
			//dbg_msg("", "%s", g_Config.m_UiServerAddress);
			Client()->Connect(g_Config.m_UiServerAddress);
			m_EnterPressed = false;
		}

		ButtonBox.HSplitBottom(5.0f, &ButtonBox, &Button);
		ButtonBox.HSplitBottom(20.0f, &ButtonBox, &Button);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_UiServerAddress, &Button, g_Config.m_UiServerAddress, sizeof(g_Config.m_UiServerAddress), 14.0f, &Offset);
		ButtonBox.HSplitBottom(20.0f, &ButtonBox, &Button);
		UI()->DoLabel(&Button, Localize("Host address"), 14.0f, -1);
	}
}

void CMenus::ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() && g_Config.m_UiPage == PAGE_FAVORITES && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
		((CMenus *)pUserData)->ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
}
