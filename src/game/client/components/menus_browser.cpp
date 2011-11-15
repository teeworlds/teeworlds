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

#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>

#include "menus.h"

CMenus::CColumn CMenus::ms_aCols[] = {
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

	
// filters
CMenus::CBrowserFilter::CBrowserFilter(int Custom, const char* pName, IServerBrowser *pServerBrowser, int Filter, int Ping, int Country, const char* pGametype, const char* pServerAddress)
: m_pServerBrowser(pServerBrowser)
{
	m_Extended = true;
	m_Custom = Custom;
	str_copy(m_aName, pName, sizeof(m_aName));
	m_Filter = pServerBrowser->AddFilter(Filter, Ping, Country, pGametype, pServerAddress);

	// init buttons
	m_SwitchButton = 0;
}

void CMenus::CBrowserFilter::Switch()
{
	m_Extended ^= 1;
}

bool CMenus::CBrowserFilter::Extended() const
{
	return m_Extended;
}

int CMenus::CBrowserFilter::Custom() const
{
	return m_Custom;
}

int CMenus::CBrowserFilter::Filter() const
{
	return m_Filter;
}

const char* CMenus::CBrowserFilter::Name() const
{
	return m_aName;
}

const void *CMenus::CBrowserFilter::ID(int Index) const
{
	return m_pServerBrowser->GetID(m_Filter, Index);
}

int CMenus::CBrowserFilter::NumSortedServers() const
{
	return m_pServerBrowser->NumSortedServers(m_Filter);
}

const CServerInfo *CMenus::CBrowserFilter::SortedGet(int Index) const
{
	return m_pServerBrowser->SortedGet(m_Filter, Index);
}

void CMenus::CBrowserFilter::SetFilter(int Num)
{
	m_Filter = Num;
}

void CMenus::RemoveFilter(int FilterIndex)
{
	int Filter = m_lFilters[FilterIndex].Filter();
	ServerBrowser()->RemoveFilter(Filter);
	m_lFilters.remove_index(FilterIndex);

	// update filter indexes
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		CBrowserFilter *pFilter = &m_lFilters[i];
		if(pFilter->Filter() > Filter)
			pFilter->SetFilter(pFilter->Filter()-1);
	}
}

void CMenus::Move(bool Up, int Filter)
{
	// move up
	CBrowserFilter Temp = m_lFilters[Filter];
	if(Up)
	{
		if(Filter > 0)
		{
			m_lFilters[Filter] = m_lFilters[Filter-1];
			m_lFilters[Filter-1] = Temp;
		}
	}
	else // move down
	{
		if(Filter < m_lFilters.size()-1)
		{
			m_lFilters[Filter] = m_lFilters[Filter+1];
			m_lFilters[Filter+1] = Temp;
		}
	}
}

int CMenus::DoBrowserEntry(const void *pID, CUIRect *pRect, const CServerInfo *pEntry)
{
	// logic
	int ReturnValue = 0;
	int Inside = UI()->MouseInside(pRect);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
		{
			if(Inside >= 0)
				ReturnValue = 1;
			UI()->SetActiveItem(0);
		}
	}
	if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
			UI()->SetActiveItem(pID);

		CUIRect r = *pRect;
		r.Margin(1.5f, &r);
		RenderTools()->DrawUIRect(&r, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 4.0f);
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// update friend counter
	if(pEntry->m_FriendState != IFriends::FRIEND_NO)
	{
		for(int j = 0; j < pEntry->m_NumClients; ++j)
		{
			if(pEntry->m_aClients[j].m_FriendState != IFriends::FRIEND_NO)
			{
				unsigned NameHash = str_quickhash(pEntry->m_aClients[j].m_aName);
				unsigned ClanHash = str_quickhash(pEntry->m_aClients[j].m_aClan);
				for(int f = 0; f < m_lFriends.size(); ++f)
				{
					if(ClanHash == m_lFriends[f].m_pFriendInfo->m_ClanHash &&
						(!m_lFriends[f].m_pFriendInfo->m_aName[0] || NameHash == m_lFriends[f].m_pFriendInfo->m_NameHash))
					{
						m_lFriends[f].m_NumFound++;
						if(m_lFriends[f].m_pFriendInfo->m_aName[0])
							break;
					}
				}
			}
		}
	}

	for(int c = 0; c < NUM_COLS; c++)
	{
		CUIRect Button;
		char aTemp[64];
		Button.x = ms_aCols[c].m_Rect.x;
		Button.y = pRect->y;
		Button.h = pRect->h;
		Button.w = ms_aCols[c].m_Rect.w;

		int ID = ms_aCols[c].m_ID;

		if(ID == COL_FLAG_LOCK)
		{
			if(pEntry->m_Flags&IServerBrowser::FLAG_PASSWORD)
				DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_LOCK, &Button);
		}
		else if(ID == COL_FLAG_PURE)
		{
			if(!(pEntry->m_Flags&IServerBrowser::FLAG_PURE))
				DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_UNPURE, &Button);
		}
		else if(ID == COL_FLAG_FAV)
		{
			if(pEntry->m_Favorite)
				DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART, &Button);
		}
		else if(ID == COL_NAME)
		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Button.w;

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME))
			{
				// highlight the parts that matches
				const char *pStr = str_find_nocase(pEntry->m_aName, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aName, (int)(pStr-pEntry->m_aName));
					TextRender()->TextColor(0.4f,0.4f,1.0f,1);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1,1,1,1);
					TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pEntry->m_aName, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pEntry->m_aName, -1);
		}
		else if(ID == COL_MAP)
		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Button.w;

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_MAPNAME))
			{
				// highlight the parts that matches
				const char *pStr = str_find_nocase(pEntry->m_aMap, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aMap, (int)(pStr-pEntry->m_aMap));
					TextRender()->TextColor(0.4f,0.4f,1.0f,1);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1,1,1,1);
					TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pEntry->m_aMap, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pEntry->m_aMap, -1);
		}
		else if(ID == COL_PLAYERS)
		{
			CUIRect Icon;
			Button.VMargin(4.0f, &Button);
			if(pEntry->m_FriendState != IFriends::FRIEND_NO)
			{
				Button.VSplitLeft(Button.h, &Icon, &Button);
				Icon.Margin(2.0f, &Icon);
				DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART, &Icon);
			}

			if(g_Config.m_BrFilterSpectators)
				str_format(aTemp, sizeof(aTemp), "%i/%i", pEntry->m_NumPlayers, pEntry->m_MaxPlayers);
			else
				str_format(aTemp, sizeof(aTemp), "%i/%i", pEntry->m_NumClients, pEntry->m_MaxClients);
			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_PLAYER))
				TextRender()->TextColor(0.4f,0.4f,1.0f,1);
			UI()->DoLabelScaled(&Button, aTemp, 12.0f, 1);
			TextRender()->TextColor(1,1,1,1);
		}
		else if(ID == COL_PING)
		{
			str_format(aTemp, sizeof(aTemp), "%i", pEntry->m_Latency);
			UI()->DoLabelScaled(&Button, aTemp, 12.0f, 1);
		}
		else if(ID == COL_VERSION)
		{
			const char *pVersion = pEntry->m_aVersion;
			UI()->DoLabelScaled(&Button, pVersion, 12.0f, 1);
		}
		else if(ID == COL_GAMETYPE)
		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f*UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Button.w;
			TextRender()->TextEx(&Cursor, pEntry->m_aGameType, -1);
		}
	}

	return ReturnValue;
}

bool CMenus::RenderFilterHeader(CUIRect View, int FilterIndex)
{
	CBrowserFilter *pFilter = &m_lFilters[FilterIndex];

	RenderTools()->DrawUIRect(&View, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);

	CUIRect Button, Label;
	View.HMargin(1.0f, &View);

	View.VSplitLeft(20.0f, 0, &Button);
	Button.VSplitLeft(18.0f, &Button, 0);
	if(DoButton_SpriteClean(IMAGE_BROWSERICONS, pFilter->Extended() ? SPRITE_BROWSERICON_COLLAPSE : SPRITE_BROWSERICON_EXPAND, &Button))
		pFilter->Switch();

	View.VSplitLeft(50.0f, 0, &Label);
	Label.HMargin(2.0f, &Label);
	char aBuf[256];
	if(!pFilter->Custom())
	{
		str_format(aBuf, sizeof(aBuf), "(%d) %s", pFilter->NumSortedServers(), pFilter->Name());
		UI()->DoLabel(&Label, aBuf, 12.0f, -1);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "(%d)", pFilter->NumSortedServers());
		UI()->DoLabel(&Label, aBuf, 12.0f, -1);
		float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1);

		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BROWSERICONS].m_Id);
		Graphics()->QuadsBegin();
		if(pFilter->Custom() == CBrowserFilter::FILTER_STANDARD)
			RenderTools()->SelectSprite(SPRITE_BROWSERICON_TEE);
		else if(pFilter->Custom() == CBrowserFilter::FILTER_FAVORITES)
			RenderTools()->SelectSprite(SPRITE_BROWSERICON_STAR_ACTIVE);
		IGraphics::CQuadItem QuadItem(Label.x+tw+5.0f, View.y, 18.0f, 18.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		Label.VSplitLeft(tw+25.0f, 0, &Label);
		UI()->DoLabel(&Label, pFilter->Name(), 12.0f, -1);
	}

	View.VSplitRight(20.0f, &Button, 0);
	Button.VSplitRight(18.0f, &View, &Button);
	if(DoButton_SpriteClean(IMAGE_BROWSERICONS, SPRITE_BROWSERICON_CLOSE, &Button))
	{
		RemoveFilter(FilterIndex);
		return true;
	}

	View.VSplitRight(2.0f, &Button, 0);
	Button.VSplitRight(18.0f, &View, &Button);
	if(DoButton_SpriteClean(IMAGE_BROWSERICONS, SPRITE_BROWSERICON_EDIT, &Button))
	{
		//
	}

	View.VSplitRight(2.0f, &Button, 0);
	Button.VSplitRight(18.0f, &View, &Button);
	if(DoButton_SpriteClean(IMAGE_BROWSERICONS, SPRITE_BROWSERICON_UP, &Button))
		Move(true, FilterIndex);

	View.VSplitRight(2.0f, &Button, 0);
	Button.VSplitRight(18.0f, &View, &Button);
	if(DoButton_SpriteClean(IMAGE_BROWSERICONS, SPRITE_BROWSERICON_DOWN, &Button))
		Move(false, FilterIndex);

	return false;
}

void CMenus::RenderServerbrowserServerList(CUIRect View)
{
	CUIRect Headers;
	CUIRect Status;

	View.HSplitTop(ms_ListheaderHeight, &Headers, &View);
	View.HSplitBottom(28.0f, &View, &Status);

	// split of the scrollbar
	RenderTools()->DrawUIRect(&Headers, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f);
	Headers.VSplitRight(20.0f, &Headers, 0);

	// do layout
	for(int i = 0; i < NUM_COLS; i++)
	{
		if(ms_aCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(ms_aCols[i].m_Width, &ms_aCols[i].m_Rect, &Headers);

			if(i+1 < NUM_COLS)
			{
				//Cols[i].flags |= SPACER;
				Headers.VSplitLeft(2, &ms_aCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NUM_COLS-1; i >= 0; i--)
	{
		if(ms_aCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(ms_aCols[i].m_Width, &Headers, &ms_aCols[i].m_Rect);
			Headers.VSplitRight(2, &Headers, &ms_aCols[i].m_Spacer);
		}
	}

	for(int i = 0; i < NUM_COLS; i++)
	{
		if(ms_aCols[i].m_Direction == 0)
			ms_aCols[i].m_Rect = Headers;
	}

	// do headers
	for(int i = 0; i < NUM_COLS; i++)
	{
		if(DoButton_GridHeader(ms_aCols[i].m_Caption, ms_aCols[i].m_Caption, g_Config.m_BrSort == ms_aCols[i].m_Sort, &ms_aCols[i].m_Rect))
		{
			if(ms_aCols[i].m_Sort != -1)
			{
				if(g_Config.m_BrSort == ms_aCols[i].m_Sort)
					g_Config.m_BrSortOrder ^= 1;
				else
					g_Config.m_BrSortOrder = 0;
				g_Config.m_BrSort = ms_aCols[i].m_Sort;
			}
		}
	}

	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.15f), 0, 0);

	CUIRect Scroll;
	View.VSplitRight(15, &View, &Scroll);

	// display important messages in the middle of the screen so no
	// users misses it
	{
		CUIRect MsgBox = View;
		MsgBox.y += View.h/3;

		if(m_ActivePage == PAGE_INTERNET && ServerBrowser()->IsRefreshingMasters())
			UI()->DoLabelScaled(&MsgBox, Localize("Refreshing master servers"), 16.0f, 0);
		else if(!ServerBrowser()->NumServers())
			UI()->DoLabelScaled(&MsgBox, Localize("No servers found"), 16.0f, 0);
		/*else if(ServerBrowser()->NumServers() && !NumServers)
			UI()->DoLabelScaled(&MsgBox, Localize("No servers match your filter criteria"), 16.0f, 0);*/
	}

	// count all the servers
	int NumServers = 0;
	for(int i = 0; i < m_lFilters.size(); i++)
		if(m_lFilters[i].Extended())
			NumServers += m_lFilters[i].NumSortedServers();

	int NumFilters = m_lFilters.size();
	float ListHieght = (NumServers+(NumFilters-1)) * ms_aCols[0].m_Rect.h + NumFilters * 20.0f;

	//int Num = (int)((ListHieght-View.h)/ms_aCols[0].m_Rect.h))+1;
	//int Num = (int)(View.h/ms_aCols[0].m_Rect.h) + 1;
	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;

	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

	int ScrollNum = (int)((ListHieght-View.h)/ms_aCols[0].m_Rect.h)+1;
	if(ScrollNum > 0)
	{
		if(m_ScrollOffset)
		{
			s_ScrollValue = (float)(m_ScrollOffset)/ScrollNum;
			m_ScrollOffset = 0;
		}
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP) && UI()->MouseInside(&View))
			s_ScrollValue -= 3.0f/ScrollNum;
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN) && UI()->MouseInside(&View))
			s_ScrollValue += 3.0f/ScrollNum;
	}
	else
		ScrollNum = 0;

	int SelectedFilter = m_SelectedServer.m_Filter;
	int SelectedIndex = m_SelectedServer.m_Index;
	if(SelectedFilter > -1)
	{
		for(int i = 0; i < m_NumInputEvents; i++)
		{
			int NewIndex = -1;
			int NewFilter = SelectedFilter;
			if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
			{
				if(m_aInputEvents[i].m_Key == KEY_DOWN)
				{
					NewIndex = SelectedIndex + 1;
					if(NewIndex >= m_lFilters[SelectedFilter].NumSortedServers())
					{
						// try to move to next filter
						for(int j = SelectedFilter+1; j < m_lFilters.size(); j++)
						{
							CBrowserFilter *pFilter = &m_lFilters[j];
							if(pFilter->Extended() && pFilter->NumSortedServers())
							{
								NewFilter = j;
								NewIndex = 0;
								break;
							}
						}
					}
				}
				else if(m_aInputEvents[i].m_Key == KEY_UP)
				{
					NewIndex = SelectedIndex - 1;
					if(NewIndex < 0)
					{
						// try to move to prev filter
						for(int j = SelectedFilter-1; j >= 0; j--)
						{
							CBrowserFilter *pFilter = &m_lFilters[j];
							if(pFilter->Extended() && pFilter->NumSortedServers())
							{
								NewFilter = j;
								NewIndex = pFilter->NumSortedServers()-1;
								break;
							}
						}
					}
				}
			}
			if(NewIndex > -1 && NewIndex < m_lFilters[NewFilter].NumSortedServers())
			{
				// get index depending on all filters
				int TotalIndex = 0;
				int Filter = 0;
				while(Filter != NewFilter)
				{
					CBrowserFilter *pFilter = &m_lFilters[Filter];
					if(pFilter->Extended())
						TotalIndex += m_lFilters[Filter].NumSortedServers();
					Filter++;
				}
				TotalIndex += NewIndex+1;

				//scroll
				float IndexY = View.y - s_ScrollValue*ScrollNum*ms_aCols[0].m_Rect.h + TotalIndex*ms_aCols[0].m_Rect.h + Filter*ms_aCols[0].m_Rect.h + Filter*20.0f;
				int Scroll = View.y > IndexY ? -1 : View.y+View.h < IndexY+ms_aCols[0].m_Rect.h ? 1 : 0;
				if(Scroll)
				{
					if(Scroll < 0)
					{
						int NumScrolls = (View.y-IndexY+ms_aCols[0].m_Rect.h-1.0f)/ms_aCols[0].m_Rect.h;
						s_ScrollValue -= (1.0f/ScrollNum)*NumScrolls;
					}
					else
					{
						int NumScrolls = (IndexY+ms_aCols[0].m_Rect.h-(View.y+View.h)+ms_aCols[0].m_Rect.h-1.0f)/ms_aCols[0].m_Rect.h;
						s_ScrollValue += (1.0f/ScrollNum)*NumScrolls;
					}
				}

				m_SelectedServer.m_Filter = NewFilter;
				m_SelectedServer.m_Index = NewIndex;

				const CServerInfo *pItem = ServerBrowser()->SortedGet(NewFilter, NewIndex);
				str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
			}
		}
	}

	if(s_ScrollValue < 0) s_ScrollValue = 0;
	if(s_ScrollValue > 1) s_ScrollValue = 1;

	// set clipping
	UI()->ClipEnable(&View);

	CUIRect OriginalView = View;
	View.y -= s_ScrollValue*ScrollNum*ms_aCols[0].m_Rect.h;

	int NumPlayers = 0;

	// reset friend counter
	for(int i = 0; i < m_lFriends.size(); m_lFriends[i++].m_NumFound = 0);

	bool Selected = false;
	for(int s = 0; s < m_lFilters.size(); s++)
	{
		CBrowserFilter *pFilter = &m_lFilters[s];

		// filter header
		CUIRect Row;
		View.HSplitTop(20.0f, &Row, &View);

		// render header
		bool Deleted = RenderFilterHeader(Row, s);
		if(Deleted && s >= m_lFilters.size())
			break;

		if(pFilter->Extended())
		{
			for (int i = 0; i < pFilter->NumSortedServers(); i++)
			{
				int ItemIndex = i;
				const CServerInfo *pItem = pFilter->SortedGet(ItemIndex);
				NumPlayers += g_Config.m_BrFilterSpectators ? pItem->m_NumPlayers : pItem->m_NumClients;
				
				CUIRect SelectHitBox;

				View.HSplitTop(17.0f, &Row, &View);
				SelectHitBox = Row;

				// select server
				if(m_SelectedServer.m_Filter == -1 || m_SelectedServer.m_Filter == s)
				{
					if(!str_comp(pItem->m_aAddress, g_Config.m_UiServerAddress))
					{
						Selected = true;
						m_SelectedServer.m_Filter = s;
						m_SelectedServer.m_Index = ItemIndex;
					}
				}

				// make sure that only those in view can be selected
				if(Row.y+Row.h > OriginalView.y && Row.y < OriginalView.y+OriginalView.h)
				{
					if(m_SelectedServer.m_Filter == s && m_SelectedServer.m_Index == i)
					{
						CUIRect r = Row;
						r.Margin(1.5f, &r);
						RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
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

				if(DoBrowserEntry(pFilter->ID(ItemIndex), &Row, pItem))
				{
					m_SelectedServer.m_Filter = s;
					m_SelectedServer.m_Index = i;
					str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
					if(Input()->MouseDoubleClick())
						Client()->Connect(g_Config.m_UiServerAddress);
				}
				
			}
		}

		if(s < m_lFilters.size()-1)
			View.HSplitTop(17.0f, &Row, &View);
	}

	UI()->ClipDisable();

	if(!Selected)
		m_SelectedServer.m_Filter = -1;

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
		float *pClearButtonFade = ButtonFade(&s_ClearButton, 0.6f);
		RenderTools()->DrawUIRect(&Button, vec4(1.0f, 1.0f, 1.0f, 0.33f+(*pClearButtonFade/0.6f)*0.165f), CUI::CORNER_R, 3.0f);
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
		str_format(aBuf, sizeof(aBuf), Localize("%d of %d servers, %d players"), NumServers, ServerBrowser()->NumServers(), NumPlayers);
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
	static int s_BrFilterEmpty = 0;
	if(DoButton_CheckBox(&s_BrFilterEmpty, Localize("Has people playing"), g_Config.m_BrFilterEmpty, &Button))
		g_Config.m_BrFilterEmpty ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterSpectators = 0;
	if(DoButton_CheckBox(&s_BrFilterSpectators, Localize("Count players only"), g_Config.m_BrFilterSpectators, &Button))
		g_Config.m_BrFilterSpectators ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterFull = 0;
	if(DoButton_CheckBox(&s_BrFilterFull, Localize("Server not full"), g_Config.m_BrFilterFull, &Button))
		g_Config.m_BrFilterFull ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterFriends = 0;
	if(DoButton_CheckBox(&s_BrFilterFriends, Localize("Show friends only"), g_Config.m_BrFilterFriends, &Button))
		g_Config.m_BrFilterFriends ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPw = 0;
	if(DoButton_CheckBox(&s_BrFilterPw, Localize("No password"), g_Config.m_BrFilterPw, &Button))
		g_Config.m_BrFilterPw ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterCompatversion = 0;
	if(DoButton_CheckBox(&s_BrFilterCompatversion, Localize("Compatible version"), g_Config.m_BrFilterCompatversion, &Button))
		g_Config.m_BrFilterCompatversion ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPure = 0;
	if(DoButton_CheckBox(&s_BrFilterPure, Localize("Standard gametype"), g_Config.m_BrFilterPure, &Button))
		g_Config.m_BrFilterPure ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPureMap = 0;
	if(DoButton_CheckBox(&s_BrFilterPureMap, Localize("Standard map"), g_Config.m_BrFilterPureMap, &Button))
		g_Config.m_BrFilterPureMap ^= 1;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterGametypeStrict = 0;
	if(DoButton_CheckBox(&s_BrFilterGametypeStrict, Localize("Strict gametype filter"), g_Config.m_BrFilterGametypeStrict, &Button))
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

	// player country
	{
		CUIRect Rect;
		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(26.0f, &Button, &ServerFilter);
		Button.VSplitRight(60.0f, &Button, &Rect);
		Button.HMargin(3.0f, &Button);
		static int s_BrFilterCountry = 0;
		if(DoButton_CheckBox(&s_BrFilterCountry, Localize("Player country:"), g_Config.m_BrFilterCountry, &Button))
			g_Config.m_BrFilterCountry ^= 1;

		float OldWidth = Rect.w;
		Rect.w = Rect.h*2;
		Rect.x += (OldWidth-Rect.w)/2.0f;
		vec4 Color(1.0f, 1.0f, 1.0f, g_Config.m_BrFilterCountry?1.0f: 0.5f);
		m_pClient->m_pCountryFlags->Render(g_Config.m_BrFilterCountryIndex, &Color, Rect.x, Rect.y, Rect.w, Rect.h);

		if(g_Config.m_BrFilterCountry && UI()->DoButtonLogic(&g_Config.m_BrFilterCountryIndex, "", 0, &Rect))
			m_Popup = POPUP_COUNTRY;
	}

	ServerFilter.HSplitBottom(5.0f, &ServerFilter, 0);
	ServerFilter.HSplitBottom(ms_ButtonHeight-2.0f, &ServerFilter, &Button);
	static int s_ClearButton = 0;
	if(DoButton_Menu(&s_ClearButton, Localize("Reset filter"), 0, &Button))
	{
		g_Config.m_BrFilterString[0] = 0;
		g_Config.m_BrFilterFull = 0;
		g_Config.m_BrFilterEmpty = 0;
		g_Config.m_BrFilterSpectators = 0;
		g_Config.m_BrFilterFriends = 0;
		g_Config.m_BrFilterCountry = 0;
		g_Config.m_BrFilterCountryIndex = -1;
		g_Config.m_BrFilterPw = 0;
		g_Config.m_BrFilterPing = 999;
		g_Config.m_BrFilterGametype[0] = 0;
		g_Config.m_BrFilterGametypeStrict = 0;
		g_Config.m_BrFilterServerAddress[0] = 0;
		g_Config.m_BrFilterPure = 1;
		g_Config.m_BrFilterPureMap = 1;
		g_Config.m_BrFilterCompatversion = 1;
		Client()->ServerBrowserUpdate();
	}
}

void CMenus::RenderServerbrowserServerDetail(CUIRect View)
{
	CUIRect ServerDetails = View;
	CUIRect ServerScoreBoard, ServerHeader;

	const CServerInfo *pSelectedServer = (m_SelectedServer.m_Filter > -1 && m_SelectedServer.m_Filter < m_lFilters.size()) ? m_lFilters[m_SelectedServer.m_Filter].SortedGet(m_SelectedServer.m_Index) : 0;

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

	if(pSelectedServer)
	{
		ServerScoreBoard.Margin(3.0f, &ServerScoreBoard);
		for (int i = 0; i < pSelectedServer->m_NumClients; i++)
		{
			CUIRect Name, Clan, Score, Flag;
			ServerScoreBoard.HSplitTop(25.0f, &Name, &ServerScoreBoard);
			if(UI()->DoButtonLogic(&pSelectedServer->m_aClients[i], "", 0, &Name))
			{
				if(pSelectedServer->m_aClients[i].m_FriendState == IFriends::FRIEND_PLAYER)
					m_pClient->Friends()->RemoveFriend(pSelectedServer->m_aClients[i].m_aName, pSelectedServer->m_aClients[i].m_aClan);
				else
					m_pClient->Friends()->AddFriend(pSelectedServer->m_aClients[i].m_aName, pSelectedServer->m_aClients[i].m_aClan);
				FriendlistOnUpdate();
				Client()->ServerBrowserUpdate();
			}

			vec4 Colour = pSelectedServer->m_aClients[i].m_FriendState == IFriends::FRIEND_NO ? vec4(1.0f, 1.0f, 1.0f, (i%2+1)*0.05f) :
																								vec4(0.5f, 1.0f, 0.5f, 0.15f+(i%2+1)*0.05f);
			RenderTools()->DrawUIRect(&Name, Colour, CUI::CORNER_ALL, 4.0f);
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
			vec4 Color(1.0f, 1.0f, 1.0f, 0.5f);
			m_pClient->m_pCountryFlags->Render(pSelectedServer->m_aClients[i].m_Country, &Color, Flag.x, Flag.y, Flag.w, Flag.h);
		}
	}
}

void CMenus::FriendlistOnUpdate()
{
	m_lFriends.clear();
	for(int i = 0; i < m_pClient->Friends()->NumFriends(); ++i)
	{
		CFriendItem Item;
		Item.m_pFriendInfo = m_pClient->Friends()->GetFriend(i);
		Item.m_NumFound = 0;
		m_lFriends.add_unsorted(Item);
	}
	m_lFriends.sort_range();
}

void CMenus::RenderServerbrowserFriends(CUIRect View)
{
	static int s_Inited = 0;
	if(!s_Inited)
	{
		FriendlistOnUpdate();
		s_Inited = 1;
	}

	CUIRect ServerFriends = View, FilterHeader;
	const float FontSize = 10.0f;

	// header
	ServerFriends.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFriends);
	RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerFriends, vec4(0,0,0,0.15f), 0, 4.0f);
	UI()->DoLabelScaled(&FilterHeader, Localize("Friends"), FontSize+4.0f, 0);
	CUIRect Button, List;

	ServerFriends.Margin(3.0f, &ServerFriends);
	ServerFriends.VMargin(3.0f, &ServerFriends);
	ServerFriends.HSplitBottom(100.0f, &List, &ServerFriends);

	// friends list(remove friend)
	static float s_ScrollValue = 0;
	static int s_FriendsList = 0;
	UiDoListboxStart(&s_FriendsList, &List, 30.0f, "", "", m_lFriends.size(), 1, m_FriendlistSelectedIndex, s_ScrollValue);

	m_lFriends.sort_range();
	for(int i = 0; i < m_lFriends.size(); ++i)
	{
		CListboxItem Item = UiDoListboxNextItem(&m_lFriends[i]);

		if(Item.m_Visible)
		{
			Item.m_Rect.Margin(1.5f, &Item.m_Rect);
			CUIRect OnState;
			Item.m_Rect.VSplitRight(30.0f, &Item.m_Rect, &OnState);
			RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1.0f, 1.0f, 1.0f, 0.1f), CUI::CORNER_L, 4.0f);

			Item.m_Rect.VMargin(2.5f, &Item.m_Rect);
			Item.m_Rect.HSplitTop(12.0f, &Item.m_Rect, &Button);
			UI()->DoLabelScaled(&Item.m_Rect, m_lFriends[i].m_pFriendInfo->m_aName, FontSize, -1);
			UI()->DoLabelScaled(&Button, m_lFriends[i].m_pFriendInfo->m_aClan, FontSize, -1);

			RenderTools()->DrawUIRect(&OnState, m_lFriends[i].m_NumFound ? vec4(0.0f, 1.0f, 0.0f, 0.25f) : vec4(1.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_R, 4.0f);
			OnState.HMargin((OnState.h-FontSize)/3, &OnState);
			OnState.VMargin(5.0f, &OnState);
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%i", m_lFriends[i].m_NumFound);
			UI()->DoLabelScaled(&OnState, aBuf, FontSize+2, 1);
		}
	}

	bool Activated = false;
	m_FriendlistSelectedIndex = UiDoListboxEnd(&s_ScrollValue, &Activated);

	// activate found server with friend
	if(Activated && !m_EnterPressed && m_lFriends[m_FriendlistSelectedIndex].m_NumFound)
	{
		bool Found = false;
		for(int s = 0; s < m_lFilters.size(); s++)
		{
			CBrowserFilter *pFilter = &m_lFilters[s];

			int NumServers = pFilter->NumSortedServers();
			for (int i = 0; i < NumServers && !Found; i++)
			{
				int ItemIndex = m_SelectedServer.m_Filter != -1 ? (m_SelectedServer.m_Index+i+1)%NumServers : i;
				const CServerInfo *pItem = pFilter->SortedGet(ItemIndex);
				if(pItem->m_FriendState != IFriends::FRIEND_NO)
				{
					for(int j = 0; j < pItem->m_NumClients && !Found; ++j)
					{
						if(pItem->m_aClients[j].m_FriendState != IFriends::FRIEND_NO &&
							str_quickhash(pItem->m_aClients[j].m_aClan) == m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_ClanHash &&
							(!m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aName[0] ||
							str_quickhash(pItem->m_aClients[j].m_aName) == m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_NameHash))
						{
							str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
							m_ScrollOffset = ItemIndex;
							m_SelectedServer.m_Filter = s;
							m_SelectedServer.m_Index = ItemIndex;
							Found = true;
						}
					}
				}
			}
		}
	}

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
		static int s_AddButton = 0;
		if(DoButton_Menu(&s_AddButton, Localize("Add Friend"), 0, &Button))
		{
			m_pClient->Friends()->AddFriend(s_aName, s_aClan);
			FriendlistOnUpdate();
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
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 12.0f);
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
			if(m_MenuPage == PAGE_INTERNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			else if(m_MenuPage == PAGE_LAN)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
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

void CMenus::ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 2 && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
	{
		((CMenus *)pUserData)->FriendlistOnUpdate();
		((CMenus *)pUserData)->Client()->ServerBrowserUpdate();
	}
}

void CMenus::ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	/*if(pResult->NumArguments() && ((CMenus *)pUserData)->m_MenuPage == PAGE_FAVORITES && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
		((CMenus *)pUserData)->ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);*/
}
