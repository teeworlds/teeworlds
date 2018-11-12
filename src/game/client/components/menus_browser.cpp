/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <algorithm> // sort  TODO: remove this

#include <engine/external/json-parser/json.h>

#include <engine/config.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>

#include "menus.h"

CMenus::CColumn CMenus::ms_aBrowserCols[] = {
	{COL_BROWSER_FLAG,		-1,									" ",		-1, 87.0f, 0, {0}, {0}}, // Localize - these strings are localized within CLocConstString
	{COL_BROWSER_NAME,		IServerBrowser::SORT_NAME,			"Server",		0, 300.0f, 0, {0}, {0}},
	{COL_BROWSER_GAMETYPE,	IServerBrowser::SORT_GAMETYPE,		"Type",		1, 70.0f, 0, {0}, {0}},
	{COL_BROWSER_MAP,		IServerBrowser::SORT_MAP,			"Map",		1, 100.0f, 0, {0}, {0}},
	{COL_BROWSER_PLAYERS,	IServerBrowser::SORT_NUMPLAYERS,	"Players",	1, 60.0f, 0, {0}, {0}},
	{COL_BROWSER_PING,		IServerBrowser::SORT_PING,			"Ping",		1, 40.0f, 0, {0}, {0}},
};

CMenus::CColumn CMenus::ms_aFriendCols[] = {
	{COL_FRIEND_TYPE,		FRIENDS_SORT_TYPE,			"Type",		-1, 70.0f, 0, {0}, {0}},
	{COL_FRIEND_SERVER,		FRIENDS_SORT_SERVER,		"Server",	0, 300.0f, 0, {0}, {0}},
	{COL_FRIEND_NAME,		FRIENDS_SORT_NAME,			"Name",		1, 150.0f, 0, {0}, {0}},
	{COL_FRIEND_CLAN,		FRIENDS_SORT_CLAN,			"Clan",		1, 120.0f, 0, {0}, {0}},
	{COL_FRIEND_DELETE,		-1,							"",			1, 20.0f, 0, {0}, {0}},
};

CServerFilterInfo CMenus::CBrowserFilter::ms_FilterStandard = {IServerBrowser::FILTER_COMPAT_VERSION|IServerBrowser::FILTER_PURE|IServerBrowser::FILTER_PURE_MAP, 999, -1, 0, {0}, 0};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterFavorites = {IServerBrowser::FILTER_COMPAT_VERSION|IServerBrowser::FILTER_FAVORITE, 999, -1, 0, {0}, 0};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterAll = {IServerBrowser::FILTER_COMPAT_VERSION, 999, -1, 0, {0}, 0};

class SortWrap
{
	typedef bool (CMenus::*SortFunc)(int, int) const;
	SortFunc m_pfnSort;
	CMenus *m_pThis;
public:
	SortWrap(CMenus *t, SortFunc f) : m_pfnSort(f), m_pThis(t) {}
	bool operator()(int a, int b) { return (g_Config.m_BrFriendSortOrder ? (m_pThis->*m_pfnSort)(b, a) : (m_pThis->*m_pfnSort)(a, b)); }
};

// filters
CMenus::CBrowserFilter::CBrowserFilter(int Custom, const char* pName, IServerBrowser *pServerBrowser)
{
	m_Extended = false;
	m_Custom = Custom;
	str_copy(m_aName, pName, sizeof(m_aName));
	m_pServerBrowser = pServerBrowser;
	switch(m_Custom)
	{
	case CBrowserFilter::FILTER_STANDARD:
		m_Filter = m_pServerBrowser->AddFilter(&ms_FilterStandard);
		break;
	case CBrowserFilter::FILTER_FAVORITES:
		m_Filter = m_pServerBrowser->AddFilter(&ms_FilterFavorites);
		break;
	default:
		m_Filter = m_pServerBrowser->AddFilter(&ms_FilterAll);
	}

	// init buttons
	m_SwitchButton = 0;
}

void CMenus::CBrowserFilter::Reset()
{
	switch(m_Custom)
	{
	case CBrowserFilter::FILTER_STANDARD:
		m_pServerBrowser->SetFilter(m_Filter, &ms_FilterStandard);
		break;
	case CBrowserFilter::FILTER_FAVORITES:
		m_pServerBrowser->SetFilter(m_Filter, &ms_FilterFavorites);
		break;
	default:
		m_pServerBrowser->SetFilter(m_Filter, &ms_FilterAll);
	}
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

int CMenus::CBrowserFilter::NumPlayers() const
{
	return m_pServerBrowser->NumSortedPlayers(m_Filter);
}

const CServerInfo *CMenus::CBrowserFilter::SortedGet(int Index) const
{
	if(Index < 0 || Index >= m_pServerBrowser->NumSortedServers(m_Filter))
		return 0;
	return m_pServerBrowser->SortedGet(m_Filter, Index);
}

void CMenus::CBrowserFilter::SetFilterNum(int Num)
{
	m_Filter = Num;
}

void CMenus::CBrowserFilter::GetFilter(CServerFilterInfo *pFilterInfo) const
{
	m_pServerBrowser->GetFilter(m_Filter, pFilterInfo);
}

void CMenus::CBrowserFilter::SetFilter(const CServerFilterInfo *pFilterInfo)
{
	m_pServerBrowser->SetFilter(m_Filter, pFilterInfo);
}

void CMenus::LoadFilters()
{
	// read file data into buffer
	const char *pFilename = "ui_settings.json";
	IOHANDLE File = Storage()->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return;
	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize + 1, 1);
	io_read(File, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
	if(pJsonData == 0)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, pFilename, aError);
		mem_free(pFileData);
		return;
	}

	// extract settings data
	const json_value &rSettingsEntry = (*pJsonData)["settings"];
	if(rSettingsEntry["sidebar_active"].type == json_integer)
		m_SidebarActive = rSettingsEntry["sidebar_active"].u.integer;
	if(rSettingsEntry["sidebar_tab"].type == json_integer)
		m_SidebarTab = clamp(int(rSettingsEntry["sidebar_tab"].u.integer), 0, 2);

	// extract filter data
	int Extended = 0;
	const json_value &rFilterEntry = (*pJsonData)["filter"];
	for(unsigned i = 0; i < rFilterEntry.u.array.length; ++i)
	{
		char *pName = rFilterEntry[i].u.object.values[0].name;
		const json_value &rStart = *(rFilterEntry[i].u.object.values[0].value);
		if(rStart.type != json_object)
			continue;

		int Type = 0;
		if(rStart["type"].type == json_integer)
			Type = rStart["type"].u.integer;
		if(rStart["extended"].type == json_integer && rStart["extended"].u.integer)
			Extended = i;

		// filter setting
		CServerFilterInfo FilterInfo;
		for(int j = 0; j < CServerFilterInfo::MAX_GAMETYPES; ++j)
			FilterInfo.m_aGametype[j][0] = 0;
		const json_value &rSubStart = rStart["settings"];
		if(rSubStart.type == json_object)
		{
			if(rSubStart["filter_hash"].type == json_integer)
				FilterInfo.m_SortHash = rSubStart["filter_hash"].u.integer;
			const json_value &rGametypeEntry = rSubStart["filter_gametype"];
			if(rGametypeEntry.type == json_array)
			{
				for(unsigned j = 0; j < rGametypeEntry.u.array.length && j < CServerFilterInfo::MAX_GAMETYPES; ++j)
				{
					if(rGametypeEntry[j].type == json_string)
						str_copy(FilterInfo.m_aGametype[j], rGametypeEntry[j], sizeof(FilterInfo.m_aGametype[j]));
				}
			}
			if(rSubStart["filter_ping"].type == json_integer)
				FilterInfo.m_Ping = rSubStart["filter_ping"].u.integer;
			if(rSubStart["filter_serverlevel"].type == json_integer)
				FilterInfo.m_ServerLevel = rSubStart["filter_serverlevel"].u.integer;
			if(rSubStart["filter_address"].type == json_string)
				str_copy(FilterInfo.m_aAddress, rSubStart["filter_address"], sizeof(FilterInfo.m_aAddress));
			if(rSubStart["filter_country"].type == json_integer)
				FilterInfo.m_Country = rSubStart["filter_country"].u.integer;
		}

		// add filter
		m_lFilters.add(CBrowserFilter(Type, pName, ServerBrowser()));
		if(Type == CBrowserFilter::FILTER_STANDARD)		//	make sure the pure filter is enabled in the Teeworlds-filter
			FilterInfo.m_SortHash |= IServerBrowser::FILTER_PURE;
		m_lFilters[i].SetFilter(&FilterInfo);
	}

	// clean up
	json_value_free(pJsonData);
	mem_free(pFileData);

	m_lFilters[Extended].Switch();
}

void CMenus::SaveFilters()
{
	IOHANDLE File = Storage()->OpenFile("ui_settings.json", IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;

	char aBuf[512];

	// settings
	const char *p = "{\"settings\": {\n";
	io_write(File, p, str_length(p));

	str_format(aBuf, sizeof(aBuf), "\t\"sidebar_active\": %d,\n", m_SidebarActive);
	io_write(File, aBuf, str_length(aBuf));
	str_format(aBuf, sizeof(aBuf), "\t\"sidebar_tab\": %d,\n", m_SidebarTab);
	io_write(File, aBuf, str_length(aBuf));

	// settings end
	p = "\t},\n";
	io_write(File, p, str_length(p));
	
	// filter
	p = " \"filter\": [";
	io_write(File, p, str_length(p));

	for(int i = 0; i < m_lFilters.size(); i++)
	{
		// part start
		if(i == 0)
			p = "\n";
		else
			p = ",\n"; 
		io_write(File, p, str_length(p));

		str_format(aBuf, sizeof(aBuf), "\t{\"%s\": {\n", m_lFilters[i].Name());
		io_write(File, aBuf, str_length(aBuf));

		str_format(aBuf, sizeof(aBuf), "\t\t\"type\": %d,\n", m_lFilters[i].Custom());
		io_write(File, aBuf, str_length(aBuf));
		str_format(aBuf, sizeof(aBuf), "\t\t\"extended\": %d,\n", m_lFilters[i].Extended()?1:0);
		io_write(File, aBuf, str_length(aBuf));

		// filter setting
		CServerFilterInfo FilterInfo;
		m_lFilters[i].GetFilter(&FilterInfo);

		str_format(aBuf, sizeof(aBuf), "\t\t\"settings\": {\n");
		io_write(File, aBuf, str_length(aBuf));
		str_format(aBuf, sizeof(aBuf), "\t\t\t\"filter_hash\": %d,\n", FilterInfo.m_SortHash);
		io_write(File, aBuf, str_length(aBuf));
		str_format(aBuf, sizeof(aBuf), "\t\t\t\"filter_gametype\": [");
		io_write(File, aBuf, str_length(aBuf));
		for(unsigned j = 0; j < CServerFilterInfo::MAX_GAMETYPES && FilterInfo.m_aGametype[j][0]; ++j)
		{
			str_format(aBuf, sizeof(aBuf), "\n\t\t\t\t\"%s\",", FilterInfo.m_aGametype[j]);
			io_write(File, aBuf, str_length(aBuf));
		}
		p = "\n\t\t\t],\n";
		io_write(File, p, str_length(p));
		str_format(aBuf, sizeof(aBuf), "\t\t\t\"filter_ping\": %d,\n", FilterInfo.m_Ping);
		io_write(File, aBuf, str_length(aBuf));
		str_format(aBuf, sizeof(aBuf), "\t\t\t\"filter_serverlevel\": %d,\n", FilterInfo.m_ServerLevel);
		io_write(File, aBuf, str_length(aBuf));
		str_format(aBuf, sizeof(aBuf), "\t\t\t\"filter_address\": \"%s\",\n", FilterInfo.m_aAddress);
		io_write(File, aBuf, str_length(aBuf));
		str_format(aBuf, sizeof(aBuf), "\t\t\t\"filter_country\": %d,\n\t\t\t}", FilterInfo.m_Country);
		io_write(File, aBuf, str_length(aBuf));

		// part end
		p = "\n\t\t}\n\t}";
		io_write(File, p, str_length(p));
	}

	// filter end
	p = "]\n}";
	io_write(File, p, str_length(p));

	io_close(File);
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
			pFilter->SetFilterNum(pFilter->Filter()-1);
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

void CMenus::SetOverlay(int Type, float x, float y, const void *pData)
{
	if(!m_PopupActive && m_InfoOverlay.m_Reset)
	{
		m_InfoOverlayActive = true;
		m_InfoOverlay.m_Type = Type;
		m_InfoOverlay.m_X = x;
		m_InfoOverlay.m_Y = y;
		m_InfoOverlay.m_pData = pData;
		m_InfoOverlay.m_Reset = false;
	}
}

int CMenus::DoBrowserEntry(const void *pID, CUIRect View, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected)
{
	// logic
	int ReturnValue = 0;
	int Inside = UI()->MouseInside(&View);

	if(UI()->CheckActiveItem(pID))
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

		CUIRect r = View;
		RenderTools()->DrawUIRect(&r, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 4.0f);
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

	vec3 TextBaseColor = vec3(1.0f, 1.0f, 1.0f);
	if(Selected || Inside)
	{
		TextBaseColor = vec3(0.0f, 0.0f, 0.0f);
		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
	}

	float TextAplpha = (pEntry->m_NumPlayers == pEntry->m_MaxPlayers || pEntry->m_NumClients == pEntry->m_MaxClients) ? 0.5f : 1.0f;
	for(int c = 0; c < NUM_BROWSER_COLS; c++)
	{
		CUIRect Button = ms_aBrowserCols[c].m_Rect;
		char aTemp[64];
		Button.x = ms_aBrowserCols[c].m_Rect.x;
		Button.y = View.y;
		Button.h = ms_aBrowserCols[c].m_Rect.h;
		Button.w = ms_aBrowserCols[c].m_Rect.w;

		int ID = ms_aBrowserCols[c].m_ID;

		if(ID == COL_BROWSER_FLAG)
		{
			CUIRect Rect = Button;
			CUIRect Icon;

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			if(pEntry->m_Flags&IServerBrowser::FLAG_PASSWORD)
			{
				Icon.Margin(2.0f, &Icon);
				DoIcon(IMAGE_BROWSEICONS, Selected ? SPRITE_BROWSE_LOCK_B : SPRITE_BROWSE_LOCK_A, &Icon);
			}

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			int Level = pEntry->m_ServerLevel;
			DoIcon(IMAGE_LEVELICONS, Level==0 ? SPRITE_LEVEL_A_OFF : Level==1 ? SPRITE_LEVEL_B_OFF : SPRITE_LEVEL_C_OFF, &Icon);

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			if(DoButton_SpriteClean(IMAGE_BROWSEICONS, pEntry->m_Favorite ? SPRITE_BROWSE_STAR_A : SPRITE_BROWSE_STAR_B, &Icon))
			{
				if(!pEntry->m_Favorite)
					ServerBrowser()->AddFavorite(pEntry);
				else
					ServerBrowser()->RemoveFavorite(pEntry);
			}

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			if(pEntry->m_FriendState != IFriends::FRIEND_NO)
			{
				Icon.Margin(2.0f, &Icon);
				DoIcon(IMAGE_BROWSEICONS, Selected ? SPRITE_BROWSE_HEART_B : SPRITE_BROWSE_HEART_A, &Icon);
			}
		}
		else if(ID == COL_BROWSER_NAME)
		{
			CTextCursor Cursor;
			float tw = TextRender()->TextWidth(0, 12.0f, pEntry->m_aName, -1);
			if(tw < Button.w)
				TextRender()->SetCursor(&Cursor, Button.x+Button.w/2.0f-tw/2.0f, Button.y, 12.0f, TEXTFLAG_RENDER);
			else
			{
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
			}
			
			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME))
			{
				// highlight the parts that matches
				const char *pStr = str_find_nocase(pEntry->m_aName, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aName, (int)(pStr-pEntry->m_aName));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, TextAplpha);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);
					TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pEntry->m_aName, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pEntry->m_aName, -1);
		}
		else if(ID == COL_BROWSER_MAP)
		{
			CTextCursor Cursor;
			float tw = TextRender()->TextWidth(0, 12.0f, pEntry->m_aMap, -1);
			if(tw < Button.w)
				TextRender()->SetCursor(&Cursor, Button.x+Button.w/2.0f-tw/2.0f, Button.y, 12.0f, TEXTFLAG_RENDER);
			else
			{
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
			}

			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_MAPNAME))
			{
				// highlight the parts that matches
				const char *pStr = str_find_nocase(pEntry->m_aMap, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aMap, (int)(pStr-pEntry->m_aMap));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, TextAplpha);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);
					TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pEntry->m_aMap, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pEntry->m_aMap, -1);
		}
		else if(ID == COL_BROWSER_PLAYERS)
		{
			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);
			CServerFilterInfo FilterInfo;
			pFilter->GetFilter(&FilterInfo);

			if(FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS)
				str_format(aTemp, sizeof(aTemp), "%d/%d", pEntry->m_NumPlayers, pEntry->m_MaxPlayers);
			else
				str_format(aTemp, sizeof(aTemp), "%d/%d", pEntry->m_NumClients, pEntry->m_MaxClients);
			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_PLAYER))
				TextRender()->TextColor(0.4f, 0.4f, 1.0f, TextAplpha);
			Button.y += 2.0f;
			UI()->DoLabel(&Button, aTemp, 12.0f, CUI::ALIGN_CENTER);
		}
		else if(ID == COL_BROWSER_PING)
		{
			int Ping = pEntry->m_Latency;

			vec4 Color;
			if(Selected || Inside)
			{
				Color = vec4(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);
			}
			else
			{
				vec4 StartColor;
				vec4 EndColor;
				float MixVal;
				if(Ping <= 125)
				{
					StartColor = vec4(0.0f, 1.0f, 0.0f, TextAplpha);
					EndColor = vec4(1.0f, 1.0f, 0.0f, TextAplpha);
					
					MixVal = (Ping-50.0f)/75.0f;
				}
				else
				{
					StartColor = vec4(1.0f, 1.0f, 0.0f, TextAplpha);
					EndColor = vec4(1.0f, 0.0f, 0.0f, TextAplpha);
					
					MixVal = (Ping-125.0f)/75.0f;
				}
				Color = mix(StartColor, EndColor, MixVal);
			}
			
			str_format(aTemp, sizeof(aTemp), "%d", pEntry->m_Latency);
			TextRender()->TextColor(Color.r, Color.g, Color.b, Color.a);
			Button.y += 2.0f;
			UI()->DoLabel(&Button, aTemp, 12.0f, CUI::ALIGN_CENTER);
		}
		else if(ID == COL_BROWSER_GAMETYPE)
		{
			// gametype icon
			CUIRect Icon;
			Button.VSplitLeft(Button.h, &Icon, &Button);
			Icon.y -= 0.5f;
			if(!(pEntry->m_Flags&IServerBrowser::FLAG_PURE))
			{
				DoIcon(IMAGE_BROWSEICONS, Selected ? SPRITE_BROWSE_UNPURE_B : SPRITE_BROWSE_UNPURE_A, &Icon);
			}
			else
			{
				// todo: display gametype icon
			}

			// gametype text
			CTextCursor Cursor;
			{
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
			}

			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAplpha);
			TextRender()->TextEx(&Cursor, pEntry->m_aGameType, -1);
		}
	}

	// show server info
	if(!m_SidebarActive && Selected)
	{
		CUIRect Info;
		View.HSplitTop(ms_aBrowserCols[0].m_Rect.h, 0, &View);
		View.VSplitLeft(160.0f, &Info, &View);
		//Info.Margin(2.0f, &Info);
		RenderDetailInfo(Info, pEntry);

		RenderDetailScoreboard(View, pEntry, 4);
	}

	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	return ReturnValue;
}

bool CMenus::RenderFilterHeader(CUIRect View, int FilterIndex)
{
	CBrowserFilter *pFilter = &m_lFilters[FilterIndex];

	float ButtonHeight = 20.0f;
	float Spacing = 3.0f;

	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Button, EditButtons;
	View.VSplitLeft(20.0f, &Button, &View);
	Button.Margin(2.0f, &Button);
	if(DoButton_SpriteClean(IMAGE_MENUICONS, pFilter->Extended() ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED, &Button))
	{
		pFilter->Switch();
		
		// retract the other filters
		if(pFilter->Extended())
		{
			for(int i = 0; i < m_lFilters.size(); ++i)
			{
				if(i != FilterIndex && m_lFilters[i].Extended())
					m_lFilters[i].Switch();
			}
		}
	}

	// split buttons from label
	View.VSplitLeft(Spacing, 0, &View);
	View.VSplitRight((ButtonHeight+Spacing)*4.0f, &View, &EditButtons);

	View.VSplitLeft(20.0f, 0, &View); // little space
	View.y += 2.0f;
	UI()->DoLabel(&View, pFilter->Name(), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	View.VSplitRight(20.0f, &View, 0); // little space
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), Localize("%d servers, %d players"), pFilter->NumSortedServers(), pFilter->NumPlayers());
	UI()->DoLabel(&View, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_RIGHT);
	/*if(pFilter->Custom() <= CBrowserFilter::FILTER_ALL)
		UI()->DoLabel(&View, pFilter->Name(), 12.0f, CUI::ALIGN_LEFT);
	else
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BROWSEICONS].m_Id);
		Graphics()->QuadsBegin();
		if(pFilter->Custom() == CBrowserFilter::FILTER_STANDARD)
			RenderTools()->SelectSprite(SPRITE_BROWSE_STAR_B);
		else if(pFilter->Custom() == CBrowserFilter::FILTER_FAVORITES)
			RenderTools()->SelectSprite(SPRITE_BROWSE_STAR_A);
		IGraphics::CQuadItem QuadItem(Label.x, View.y, 18.0f, 18.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		Label.VSplitLeft(25.0f, 0, &Label);
		UI()->DoLabel(&Label, pFilter->Name(), 12.0f, CUI::ALIGN_LEFT);
	}*/

	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(pFilter->Custom() == CBrowserFilter::FILTER_CUSTOM)
	{
		if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_X_A, &Button))
		{
			RemoveFilter(FilterIndex);
			return true;
		}
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_X_B, &Button);

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_EDIT_A, &Button))
	{
		static int s_EditPopupID = 0;
		m_SelectedFilter = FilterIndex;
		InvokePopupMenu(&s_EditPopupID, 0, UI()->MouseX(), UI()->MouseY(), 200.0f, 310.0f, PopupFilter);
	}

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(FilterIndex > 0 && (pFilter->Custom() > CBrowserFilter::FILTER_ALL || m_lFilters[FilterIndex-1].Custom() != CBrowserFilter::FILTER_STANDARD))
	{
		if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_UP_A, &Button))
			Move(true, FilterIndex);
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_UP_B, &Button);

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(FilterIndex >= 0 && FilterIndex < m_lFilters.size()-1 && (pFilter->Custom() != CBrowserFilter::FILTER_STANDARD || m_lFilters[FilterIndex+1].Custom() > CBrowserFilter::FILTER_ALL))
	{
		if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_DOWN_A, &Button))
			Move(false, FilterIndex);
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_DOWN_B, &Button);

	return false;
}

void CMenus::RenderServerbrowserOverlay()
{
	if(!m_InfoOverlayActive)
	{
		m_InfoOverlay.m_Reset = true;
		return;
	}

	int Type = m_InfoOverlay.m_Type;
	CUIRect View;

	if(Type == CInfoOverlay::OVERLAY_HEADERINFO)
	{
		CBrowserFilter *pFilter = (CBrowserFilter*)m_InfoOverlay.m_pData;

		// get position
		View.x = m_InfoOverlay.m_X-100.0f;
		View.y = m_InfoOverlay.m_Y;
		View.w = 100.0f;
		View.h = 30.0f;

		// render background
		RenderTools()->DrawUIRect(&View, vec4(0.25f, 0.25f, 0.25f, 1.0f), CUI::CORNER_ALL, 6.0f);

		View.Margin(2.0f, &View);

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Servers"), pFilter->NumSortedServers());
		UI()->DoLabel(&View, aBuf, 12.0f, CUI::ALIGN_CENTER);

		View.HSplitMid(0, &View);
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Players"), pFilter->NumPlayers());
		UI()->DoLabel(&View, aBuf, 12.0f, CUI::ALIGN_CENTER);
	}
	else if(Type == CInfoOverlay::OVERLAY_SERVERINFO)
	{
		const CServerInfo *pInfo = (CServerInfo*)m_InfoOverlay.m_pData;

		// get position
		View.x = m_InfoOverlay.m_X-210.0f;
		View.y = m_InfoOverlay.m_Y;
		View.w = 210.0f;
		View.h = pInfo->m_NumClients ? 98.0f + pInfo->m_NumClients*25.0f : 72.0f;
		if(View.y+View.h >= 590.0f)
			View.y -= View.y+View.h - 590.0f;

		// render background
		RenderTools()->DrawUIRect(&View, vec4(0.25f, 0.25f, 0.25f, 1.0f), CUI::CORNER_ALL, 6.0f);

		RenderServerbrowserServerDetail(View, pInfo);
	}
	else if(Type == CInfoOverlay::OVERLAY_PLAYERSINFO)
	{
		const CServerInfo *pInfo = (CServerInfo*)m_InfoOverlay.m_pData;

		CUIRect Screen = *UI()->Screen();
		float ButtonHeight = 20.0f;

		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
		TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);

		if(pInfo && pInfo->m_NumClients)
		{
			// get position
			View.x = m_InfoOverlay.m_X+25.0f;
			View.y = m_InfoOverlay.m_Y;
			View.w = 250.0f;
			View.h = pInfo->m_NumClients*ButtonHeight;
			if(View.x+View.w > Screen.w-5.0f)
			{
				View.y += 25.0f;
				View.x -= View.x+View.w-Screen.w+5.0f;
			}
			if(View.y+View.h >= 590.0f)
				View.y -= View.y+View.h - 590.0f;

			// render background
			RenderTools()->DrawUIRect(&View, vec4(1.0f, 1.0f, 1.0f, 0.75f), CUI::CORNER_ALL, 6.0f);

			CUIRect ServerScoreBoard = View;
			CTextCursor Cursor;
			const float FontSize = 12.0f;
			for (int i = 0; i < pInfo->m_NumClients; i++)
			{
				CUIRect Name, Clan, Score, Flag;
				ServerScoreBoard.HSplitTop(ButtonHeight, &Name, &ServerScoreBoard);
				if(UI()->DoButtonLogic(&pInfo->m_aClients[i], "", 0, &Name))
				{
					if(pInfo->m_aClients[i].m_FriendState == IFriends::FRIEND_PLAYER)
						m_pClient->Friends()->RemoveFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
					else
						m_pClient->Friends()->AddFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
					FriendlistOnUpdate();
					Client()->ServerBrowserUpdate();
				}

				vec4 Colour = pInfo->m_aClients[i].m_FriendState == IFriends::FRIEND_NO ? vec4(1.0f, 1.0f, 1.0f, (i%2+1)*0.05f) :
																									vec4(0.5f, 1.0f, 0.5f, 0.15f+(i%2+1)*0.05f);
				RenderTools()->DrawUIRect(&Name, Colour, CUI::CORNER_ALL, 4.0f);
				Name.VSplitLeft(5.0f, 0, &Name);
				Name.VSplitLeft(30.0f, &Score, &Name);
				Name.VSplitRight(34.0f, &Name, &Flag);
				Flag.HMargin(4.0f, &Flag);
				Name.VSplitRight(40.0f, &Name, &Clan);

				// score
				if(pInfo->m_aClients[i].m_Player)
				{
					char aTemp[16];
					str_format(aTemp, sizeof(aTemp), "%d", pInfo->m_aClients[i].m_Score);
					TextRender()->SetCursor(&Cursor, Score.x, Score.y+(Score.h-FontSize)/4.0f, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
					Cursor.m_LineWidth = Score.w;
					TextRender()->TextEx(&Cursor, aTemp, -1);
				}

				// name
				TextRender()->SetCursor(&Cursor, Name.x, Name.y+(Name.h-FontSize)/4.0f, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Name.w;
				const char *pName = pInfo->m_aClients[i].m_aName;
				if(g_Config.m_BrFilterString[0])
				{
					// highlight the parts that matches
					const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
					if(s)
					{
						TextRender()->TextEx(&Cursor, pName, (int)(s-pName));
						TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
						TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
						TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pName, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pName, -1);

				// clan
				TextRender()->SetCursor(&Cursor, Clan.x, Clan.y+(Clan.h-FontSize)/4.0f, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Clan.w;
				const char *pClan = pInfo->m_aClients[i].m_aClan;
				if(g_Config.m_BrFilterString[0])
				{
					// highlight the parts that matches
					const char *s = str_find_nocase(pClan, g_Config.m_BrFilterString);
					if(s)
					{
						TextRender()->TextEx(&Cursor, pClan, (int)(s-pClan));
						TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
						TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
						TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pClan, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pClan, -1);

				// flag
				vec4 Color(1.0f, 1.0f, 1.0f, 0.75f);
				m_pClient->m_pCountryFlags->Render(pInfo->m_aClients[i].m_Country, &Color, Flag.x, Flag.y, Flag.w, Flag.h);
			}
		}
		else
		{
			View.x = m_InfoOverlay.m_X+25.0f;
			View.y = m_InfoOverlay.m_Y;
			View.w = 150.0f;
			View.h = ButtonHeight;
			if(View.x+View.w > Screen.w-5.0f)
			{
				View.y += 25.0f;
				View.x -= View.x+View.w-Screen.w+5.0f;
			}
			if(View.y+View.h >= 590.0f)
				View.y -= View.y+View.h - 590.0f;

			// render background
			RenderTools()->DrawUIRect(&View, vec4(1.0f, 1.0f, 1.0f, 0.75f), CUI::CORNER_ALL, 6.0f);

			View.y += 2.0f;
			UI()->DoLabel(&View, Localize("no players"), View.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
		}

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}

	// deactivate it
	vec2 OverlayCenter = vec2(View.x+View.w/2.0f, View.y+View.h/2.0f);
	float MouseDistance = distance(m_MousePos, OverlayCenter);
	float PrefMouseDistance = distance(m_PrevMousePos, OverlayCenter);
	if(PrefMouseDistance > MouseDistance && !UI()->MouseInside(&View))
	{
		m_InfoOverlayActive = false;
		m_InfoOverlay.m_Reset = true;
	}
}

void CMenus::RenderServerbrowserServerList(CUIRect View)
{
	CUIRect Headers, Status, InfoButton;

	float SpacingH = 2.0f;
	float ButtonHeight = 20.0f;

	// background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	View.HSplitTop(ms_ListheaderHeight, &Headers, &View);
	View.HSplitBottom(ButtonHeight*3.0f+SpacingH*2.0f, &View, &Status);

	Headers.VSplitRight(ms_ListheaderHeight, &Headers, &InfoButton); // split for info button

	// do layout
	for(int i = 0; i < NUM_BROWSER_COLS; i++)
	{
		if(ms_aBrowserCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(ms_aBrowserCols[i].m_Width, &ms_aBrowserCols[i].m_Rect, &Headers);

			if(i+1 < NUM_BROWSER_COLS)
			{
				//Cols[i].flags |= SPACER;
				Headers.VSplitLeft(2, &ms_aBrowserCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NUM_BROWSER_COLS-1; i >= 0; i--)
	{
		if(ms_aBrowserCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(ms_aBrowserCols[i].m_Width, &Headers, &ms_aBrowserCols[i].m_Rect);
			Headers.VSplitRight(2, &Headers, &ms_aBrowserCols[i].m_Spacer);
		}
	}

	for(int i = 0; i < NUM_BROWSER_COLS; i++)
	{
		if(ms_aBrowserCols[i].m_Direction == 0)
			ms_aBrowserCols[i].m_Rect = Headers;
	}

	// do headers
	for(int i = 0; i < NUM_BROWSER_COLS; i++)
	{
		if(i == COL_BROWSER_FLAG)
		{
			CUIRect Rect = ms_aBrowserCols[i].m_Rect;
			CUIRect Icon;

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_LOCK_A, &Icon);

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_UNPURE_A, &Icon);

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_STAR_A, &Icon);

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART_A, &Icon);
		}
		else if(DoButton_GridHeader(ms_aBrowserCols[i].m_Caption, ms_aBrowserCols[i].m_Caption, g_Config.m_BrSort == ms_aBrowserCols[i].m_Sort, &ms_aBrowserCols[i].m_Rect))
		{
			if(ms_aBrowserCols[i].m_Sort != -1)
			{
				if(g_Config.m_BrSort == ms_aBrowserCols[i].m_Sort)
					g_Config.m_BrSortOrder ^= 1;
				else
					g_Config.m_BrSortOrder = 0;
				g_Config.m_BrSort = ms_aBrowserCols[i].m_Sort;
			}
		}
	}

	// split scrollbar from view
	CUIRect Scroll;
	View.VSplitRight(20.0f, &View, &Scroll);

	// scrollbar background
	RenderTools()->DrawUIRect(&Scroll, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// list background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	{
		int Column = COL_BROWSER_PING;
		switch(g_Config.m_BrSort)
		{
			case IServerBrowser::SORT_NAME:
				Column = COL_BROWSER_NAME;
				break;
			case IServerBrowser::SORT_GAMETYPE:
				Column = COL_BROWSER_GAMETYPE;
				break;
			case IServerBrowser::SORT_MAP:
				Column = COL_BROWSER_MAP;
				break;
			case IServerBrowser::SORT_NUMPLAYERS:
				Column = COL_BROWSER_PLAYERS;
				break;
		}

		CUIRect Rect = View;
		Rect.x = CMenus::ms_aBrowserCols[Column].m_Rect.x;
		Rect.w = CMenus::ms_aBrowserCols[Column].m_Rect.w;
		RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.05f), CUI::CORNER_ALL, 5.0f);
	}

	// display important messages in the middle of the screen so no
	// users misses it
	{
		CUIRect MsgBox = View;
		MsgBox.y += View.h/3;

		if(m_ActivePage == PAGE_INTERNET && ServerBrowser()->IsRefreshingMasters())
			UI()->DoLabelScaled(&MsgBox, Localize("Refreshing master servers"), 16.0f, CUI::ALIGN_CENTER);
		else if(!ServerBrowser()->NumServers())
			UI()->DoLabelScaled(&MsgBox, Localize("No servers found"), 16.0f, CUI::ALIGN_CENTER);
		/*else if(ServerBrowser()->NumServers() && !NumServers)
			UI()->DoLabelScaled(&MsgBox, Localize("No servers match your filter criteria"), 16.0f, CUI::ALIGN_CENTER);*/
	}

	// count all the servers
	int NumServers = 0;
	for(int i = 0; i < m_lFilters.size(); i++)
		if(m_lFilters[i].Extended())
			NumServers += m_lFilters[i].NumSortedServers();

	int NumFilters = m_lFilters.size();
	float ListHeight = (NumServers+(NumFilters-1)) * ms_aBrowserCols[0].m_Rect.h + NumFilters * 20.0f;

	//int Num = (int)((ListHeight-View.h)/ms_aBrowserCols[0].m_Rect.h))+1;
	//int Num = (int)(View.h/ms_aBrowserCols[0].m_Rect.h) + 1;
	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;

	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

	int ScrollNum = (int)((ListHeight-View.h)/ms_aBrowserCols[0].m_Rect.h)+(m_SidebarActive?1:4);
	if(ScrollNum > 0)
	{
		/*if(m_ScrollOffset)
		{
			s_ScrollValue = (float)(m_ScrollOffset)/ScrollNum;
			m_ScrollOffset = 0;
		}*/
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP) && UI()->MouseInside(&View))
			s_ScrollValue -= 3.0f/ScrollNum;
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) && UI()->MouseInside(&View))
			s_ScrollValue += 3.0f/ScrollNum;
	}
	else
		ScrollNum = 0;

	int SelectedFilter = m_SelectedServer.m_Filter;
	int SelectedIndex = m_SelectedServer.m_Index;
	if(SelectedFilter > -1)
	{
		int NewIndex = -1;
		int NewFilter = SelectedFilter;
		if(m_DownArrowPressed)
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
		else if(m_UpArrowPressed)
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
			float IndexY = View.y - s_ScrollValue*ScrollNum*ms_aBrowserCols[0].m_Rect.h + TotalIndex*ms_aBrowserCols[0].m_Rect.h + Filter*ms_aBrowserCols[0].m_Rect.h + Filter*20.0f;
			int Scroll = View.y > IndexY ? -1 : View.y+View.h < IndexY+ms_aBrowserCols[0].m_Rect.h ? 1 : 0;
			if(Scroll)
			{
				if(Scroll < 0)
				{
					int NumScrolls = (View.y-IndexY+ms_aBrowserCols[0].m_Rect.h-1.0f)/ms_aBrowserCols[0].m_Rect.h;
					s_ScrollValue -= (1.0f/ScrollNum)*NumScrolls;
				}
				else
				{
					int NumScrolls = (IndexY+ms_aBrowserCols[0].m_Rect.h-(View.y+View.h)+ms_aBrowserCols[0].m_Rect.h-1.0f)/ms_aBrowserCols[0].m_Rect.h;
					s_ScrollValue += (1.0f/ScrollNum)*NumScrolls;
				}
			}

			m_SelectedServer.m_Filter = NewFilter;
			m_SelectedServer.m_Index = NewIndex;

			const CServerInfo *pItem = ServerBrowser()->SortedGet(NewFilter, NewIndex);
			str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
		}
	}

	if(s_ScrollValue < 0) s_ScrollValue = 0;
	if(s_ScrollValue > 1) s_ScrollValue = 1;

	// set clipping
	UI()->ClipEnable(&View);

	CUIRect OriginalView = View;
	View.y -= s_ScrollValue*ScrollNum*ms_aBrowserCols[0].m_Rect.h;

	int NumPlayers = ServerBrowser()->NumPlayers();

	// reset friend counter
	for(int i = 0; i < m_lFriends.size(); m_lFriends[i++].m_NumFound = 0);

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
				const CServerInfo *pItem = pFilter->SortedGet(i);

				// select server
				if(m_SelectedServer.m_Filter == -1 || m_SelectedServer.m_Filter == s)
				{
					if(!str_comp(pItem->m_aAddress, g_Config.m_UiServerAddress))
					{
						m_SelectedServer.m_Filter = s;
						m_SelectedServer.m_Index = i;
					}
				}

				if(!m_SidebarActive && m_SelectedServer.m_Filter == s && m_SelectedServer.m_Index == i)
					View.HSplitTop(ms_ListheaderHeight*6, &Row, &View);
				else
					View.HSplitTop(ms_ListheaderHeight, &Row, &View);

				// make sure that only those in view can be selected
				if(Row.y+Row.h > OriginalView.y && Row.y < OriginalView.y+OriginalView.h)
				{
					if(m_SelectedServer.m_Filter == s && m_SelectedServer.m_Index == i)
					{
						CUIRect r = Row;
						RenderTools()->DrawUIRect(&r, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 4.0f);
					}
				}
				else
				{
					// reset active item, if not visible
					if(UI()->CheckActiveItem(pItem))
						UI()->SetActiveItem(0);

					// don't render invisible items
					continue;
				}

				if(DoBrowserEntry(pFilter->ID(i), Row, pItem, pFilter, m_SelectedServer.m_Filter == s && m_SelectedServer.m_Index == i))
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
			View.HSplitTop(SpacingH, &Row, &View);
	}

	UI()->ClipDisable();

	// bottom
	float SpacingW = 3.0f;
	float ButtonWidth = (Status.w/6.0f)-(SpacingW*5.0)/6.0f;

	// cut view
	CUIRect Left, Label, EditBox, Button;
	Status.VSplitLeft(ButtonWidth*3.0f+SpacingH*2.0f, &Left, &Status);

	// render quick search and host address
	Left.HSplitTop(((ButtonHeight*3.0f+SpacingH*2.0f)-(ButtonHeight*2.0f+SpacingH))/2.0f, 0, &Left);
	Left.HSplitTop(ButtonHeight, &Label, &Left);
	Label.VSplitRight(ButtonWidth*2.0f+SpacingH, &Label, &EditBox);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Search:"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	EditBox.VSplitRight(EditBox.h, &EditBox, &Button);
	static float s_ClearOffset = 0.0f;
	if(DoEditBox(&g_Config.m_BrFilterString, &EditBox, g_Config.m_BrFilterString, sizeof(g_Config.m_BrFilterString), ButtonHeight*ms_FontmodHeight*0.8f, &s_ClearOffset, false, CUI::CORNER_ALL))
		Client()->ServerBrowserUpdate();

	// clear button
	{
		static CButtonContainer s_ClearButton;
		if(DoButton_SpriteID(&s_ClearButton, IMAGE_TOOLICONS, SPRITE_TOOL_X_A, false, &Button, CUI::CORNER_ALL, 5.0f, false))
		{
			g_Config.m_BrFilterString[0] = 0;
			UI()->SetActiveItem(&g_Config.m_BrFilterString);
			Client()->ServerBrowserUpdate();
		}
	}

	Left.HSplitTop(SpacingH, 0, &Left);
	Left.HSplitTop(ButtonHeight, &Label, 0);
	Label.VSplitRight(ButtonWidth*2.0f+SpacingH, &Label, &EditBox);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Host address:"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	static float s_AddressOffset = 0.0f;
	DoEditBox(&g_Config.m_UiServerAddress, &EditBox, g_Config.m_UiServerAddress, sizeof(g_Config.m_UiServerAddress), ButtonHeight*ms_FontmodHeight*0.8f, &s_AddressOffset, false, CUI::CORNER_ALL);

	// render status
	Status.HSplitTop(ButtonHeight+SpacingH, 0, &Status);
	Status.HSplitTop(ButtonHeight, &Status, 0);
	char aBuf[128];
	if(ServerBrowser()->IsRefreshing())
		str_format(aBuf, sizeof(aBuf), Localize("%d%% loaded"), ServerBrowser()->LoadingProgression());
	else
		str_format(aBuf, sizeof(aBuf), Localize("%d servers, %d players"), ServerBrowser()->NumServers(), NumPlayers);
	Status.y += 2.0f;
	UI()->DoLabel(&Status, aBuf, 14.0f, CUI::ALIGN_CENTER);
}

void CMenus::RenderServerbrowserSidebar(CUIRect View)
{
	CUIRect Header, Button;

	// background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// header
	View.HSplitTop(ms_ListheaderHeight, &Header, &View);
	Header.VSplitLeft(Header.w/2, &Button, &Header);
	static CButtonContainer s_TabFriends;
	if(DoButton_SpriteID(&s_TabFriends, IMAGE_SIDEBARICONS, m_SidebarTab!=0?SPRITE_SIDEBAR_FRIEND_A:SPRITE_SIDEBAR_FRIEND_B, m_SidebarTab==0 , &Button, CUI::CORNER_TL, 5.0f, true))
	{
		m_SidebarTab = 0;
	}
	Header.VSplitLeft(Header.w/2, &Button, &Header);
	static CButtonContainer s_TabFilter;
	if(DoButton_SpriteID(&s_TabFilter, IMAGE_SIDEBARICONS, m_SidebarTab!=1?SPRITE_SIDEBAR_FILTER_A: SPRITE_SIDEBAR_FILTER_B, m_SidebarTab==1, &Button, 0, 0.0f, true))
	{
		m_SidebarTab = 1;
	}
	static CButtonContainer s_TabInfo;
	if(DoButton_SpriteID(&s_TabInfo, IMAGE_SIDEBARICONS, m_SidebarTab!=2?SPRITE_SIDEBAR_INFO_A: SPRITE_SIDEBAR_INFO_B, m_SidebarTab == 2, &Header, CUI::CORNER_TR, 5.0f, true))
	{
		m_SidebarTab = 2;
	}

	// tabs
	switch(m_SidebarTab)
	{
	case 0:
		RenderServerbrowserFriendTab(View);
		break;
	case 1:
		RenderServerbrowserFilterTab(View);
		break;
	case 2:
		RenderServerbrowserInfoTab(View);
	}
}

void CMenus::RenderServerbrowserFriendTab(CUIRect View)
{

}

void CMenus::RenderServerbrowserFilterTab(CUIRect View)
{
	PopupFilter(this, View);
}

void CMenus::RenderServerbrowserInfoTab(CUIRect View)
{
	const CServerInfo *pItem = 0;
	if(m_SelectedServer.m_Filter >= 0 && m_SelectedServer.m_Filter < m_lFilters.size())
	{
		pItem = m_lFilters[m_SelectedServer.m_Filter].SortedGet(m_SelectedServer.m_Index);
		RenderServerbrowserServerDetail(View, pItem);
	}
}

void CMenus::RenderDetailInfo(CUIRect View, const CServerInfo *pInfo)
{
	CUIRect ServerHeader;
	const float FontSize = 10.0f;
	View.HSplitTop(ms_ListheaderHeight, &ServerHeader, &View);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1, 1, 1, 0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&View, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	ServerHeader.HMargin(2.0f, &ServerHeader);
	UI()->DoLabelScaled(&ServerHeader, Localize("Server details"), FontSize + 2.0f, CUI::ALIGN_CENTER);

	if(pInfo)
	{
		CUIRect Row;
		static CLocConstString s_aLabels[] = {
			"Map",			// Localize - these strings are localized within CLocConstString
			"Game type",
			"Version",
			"Difficulty" };

		static CLocConstString s_aDifficulty[] = {
			"Casual",			// Localize - these strings are localized within CLocConstString
			"Normal",
			"Competitive" };

		CUIRect LeftColumn, RightColumn;
		View.VMargin(2.0f, &View);
		View.VSplitLeft(70.0f, &LeftColumn, &RightColumn);

		for(unsigned int i = 0; i < sizeof(s_aLabels) / sizeof(s_aLabels[0]); i++)
		{

			LeftColumn.HSplitTop(15.0f, &Row, &LeftColumn);
			UI()->DoLabelScaled(&Row, s_aLabels[i], FontSize, CUI::ALIGN_LEFT);
		}

		// map
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabelScaled(&Row, pInfo->m_aMap, FontSize, CUI::ALIGN_LEFT);

		// game type
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		CUIRect Icon;
		Row.VSplitLeft(Row.h, &Icon, &Row);
		Icon.y -= 2.0f;
		if(!(pInfo->m_Flags&IServerBrowser::FLAG_PURE))
		{
			DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_UNPURE_A, &Icon);
		}
		else
		{
			// todo: display gametype icon
		}
		UI()->DoLabelScaled(&Row, pInfo->m_aGameType, FontSize, CUI::ALIGN_LEFT);

		// version
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabelScaled(&Row, pInfo->m_aVersion, FontSize, CUI::ALIGN_LEFT);

		// difficulty
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		Row.VSplitLeft(Row.h, &Icon, &Row);
		Icon.y -= 2.0f;
		switch(pInfo->m_ServerLevel)
		{
		case 0:
			DoIcon(IMAGE_LEVELICONS, SPRITE_LEVEL_A_ON, &Icon);
			break;
		case 1:
			DoIcon(IMAGE_LEVELICONS, SPRITE_LEVEL_B_ON, &Icon);
			break;
		case 2:
			DoIcon(IMAGE_LEVELICONS, SPRITE_LEVEL_C_ON, &Icon);
		}
		UI()->DoLabelScaled(&Row, s_aDifficulty[pInfo->m_ServerLevel], FontSize, CUI::ALIGN_LEFT);
	}
}

void CMenus::RenderDetailScoreboard(CUIRect View, const CServerInfo *pInfo, int Column)
{
	// server scoreboard
	CUIRect ServerHeader;
	CTextCursor Cursor;
	const float FontSize = 10.0f;
	int ActColumn = 0;
	RenderTools()->DrawUIRect(&View, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	View.Margin(2.0f, &View);
	
	if(pInfo)
	{
		for(int i = 0; i < pInfo->m_NumClients; i++)
		{
			CUIRect Name, Clan, Score, Flag, Row;
			if(i % (16 / Column) == 0)
			{
				View.VSplitLeft(View.w / (Column - ActColumn), &Row, &View);
				ActColumn++;
			}
	
			Row.HSplitTop(20.0f, &Name, &Row);
			if(UI()->DoButtonLogic(&pInfo->m_aClients[i], "", 0, &Name))
			{
				if(pInfo->m_aClients[i].m_FriendState == IFriends::FRIEND_PLAYER)
					m_pClient->Friends()->RemoveFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
				else
					m_pClient->Friends()->AddFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
				FriendlistOnUpdate();
				Client()->ServerBrowserUpdate();
			}

			vec4 Colour = pInfo->m_aClients[i].m_FriendState == IFriends::FRIEND_NO ? vec4(1.0f, 1.0f, 1.0f, (i % 2 + 1)*0.05f) :
				vec4(0.5f, 1.0f, 0.5f, 0.15f + (i % 2 + 1)*0.05f);
			RenderTools()->DrawUIRect(&Name, Colour, CUI::CORNER_ALL, 4.0f);
			Name.VSplitLeft(5.0f, 0, &Name);
			Name.VSplitLeft(30.0f, &Score, &Name);
			Name.VSplitRight(34.0f, &Name, &Flag);
			Flag.HMargin(2.0f, &Flag);
			Name.HSplitTop(10.0f, &Name, &Clan);

			// score
			if(pInfo->m_aClients[i].m_Player)
			{
				char aTemp[16];
				str_format(aTemp, sizeof(aTemp), "%d", pInfo->m_aClients[i].m_Score);
				TextRender()->SetCursor(&Cursor, Score.x, Score.y + (Score.h - FontSize) / 4.0f, FontSize, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Score.w;
				TextRender()->TextEx(&Cursor, aTemp, -1);
			}

			// name
			TextRender()->SetCursor(&Cursor, Name.x, Name.y, FontSize - 2, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Name.w;
			const char *pName = pInfo->m_aClients[i].m_aName;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pName, (int)(s - pName));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s + str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pName, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pName, -1);

			// clan
			TextRender()->SetCursor(&Cursor, Clan.x, Clan.y, FontSize - 2, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Clan.w;
			const char *pClan = pInfo->m_aClients[i].m_aClan;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pClan, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pClan, (int)(s - pClan));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s + str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pClan, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pClan, -1);

			// flag
			Flag.w = Flag.h*2;
			vec4 Color(1.0f, 1.0f, 1.0f, 0.5f);
			m_pClient->m_pCountryFlags->Render(pInfo->m_aClients[i].m_Country, &Color, Flag.x, Flag.y, Flag.w, Flag.h);
		}
	}
}

void CMenus::RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo)
{
	CUIRect ServerHeader, ServerDetails, ServerScoreboard;
	const float FontSize = 10.0f;

	// split off a piece to use for scoreboard
	//View.HMargin(2.0f, &View);
	View.HSplitTop(80.0f, &ServerDetails, &ServerScoreboard);

	// server details
	RenderDetailInfo(ServerDetails, pInfo);

	// server scoreboard
	ServerScoreboard.HSplitTop(ms_ListheaderHeight, &ServerHeader, &ServerScoreboard);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1, 1, 1, 0.25f), CUI::CORNER_T, 4.0f);
	//RenderTools()->DrawUIRect(&View, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	ServerHeader.HMargin(2.0f, &ServerHeader);
	UI()->DoLabelScaled(&ServerHeader, Localize("Scoreboard"), FontSize + 2.0f, CUI::ALIGN_CENTER);
	RenderDetailScoreboard(ServerScoreboard, pInfo, 1);
}

void CMenus::FriendlistOnUpdate()
{
	m_lFriends.clear();
	if(m_pFriendIndexes)
		mem_free(m_pFriendIndexes);
	m_pFriendIndexes = (int *)mem_alloc(m_pClient->Friends()->NumFriends() * sizeof(int), 1);
	for(int i = 0; i < m_pClient->Friends()->NumFriends(); ++i)
	{
		CFriendItem Item;
		Item.m_pFriendInfo = m_pClient->Friends()->GetFriend(i);
		Item.m_NumFound = 0;
		m_lFriends.add(Item);
		m_pFriendIndexes[i] = i;
	}
	SortFriends();
}

void CMenus::RenderServerbrowserBottomBox(CUIRect MainView)
{
	// same size like tabs in top but variables not really needed
	float Spacing = 3.0f;
	float ButtonWidth = MainView.w/2.0f-Spacing/2.0f;

	// render background
	RenderTools()->DrawUIRect4(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	// back to main menu
	CUIRect Button;
	MainView.HSplitTop(25.0f, &MainView, 0);
	MainView.VSplitLeft(ButtonWidth, &Button, &MainView);
	static CButtonContainer s_RefreshButton;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Button) || (Input()->KeyPress(KEY_R) && (Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))))
	{
		if(m_MenuPage == PAGE_INTERNET)
			ServerBrowser()->Refresh(IServerBrowser::REFRESHFLAG_INTERNET);
		else if(m_MenuPage == PAGE_LAN)
			ServerBrowser()->Refresh(IServerBrowser::REFRESHFLAG_LAN);
	}

	MainView.VSplitLeft(Spacing, 0, &MainView); // little space
	MainView.VSplitLeft(ButtonWidth, &Button, &MainView);
	static CButtonContainer s_JoinButton;
	if(DoButton_Menu(&s_JoinButton, Localize("Connect"), 0, &Button) || m_EnterPressed)
	{
		Client()->Connect(g_Config.m_UiServerAddress);
		m_EnterPressed = false;
	}
}

void CMenus::RenderServerbrowser(CUIRect MainView)
{
	/*
		+---------------------------+-------+
		|							|		|
		|							| side-	|
		|		server list			| bar	|
		|							| <->	|
		|---------------------------+-------+
		| back |	   | bottom box |
		+------+       +------------+
	*/

	CUIRect ServerList, Sidebar, BottomBox, SidebarButton;

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitBottom(80.0f, &ServerList, &MainView);
	ServerList.VSplitRight(20.0f, &ServerList, &SidebarButton);
	if(m_SidebarActive)
		ServerList.VSplitRight(150.0f, &ServerList, &Sidebar);

	// server list
	if(m_BorwserPage == PAGE_BROWSER_BROWSER)
		RenderServerbrowserServerList(ServerList);
	else if(m_BorwserPage == PAGE_BROWSER_FRIENDS)
		RenderServerbrowserFriendList(ServerList);

	// sidebar
	if(m_SidebarActive)
		RenderServerbrowserSidebar(Sidebar);

	// sidebar button
	SidebarButton.HMargin(150.0f, &SidebarButton);
	static CButtonContainer s_SidebarButton;
	if(DoButton_SpriteID(&s_SidebarButton, IMAGE_INFOICONS, m_SidebarActive?SPRITE_INFO_B:SPRITE_INFO_A, false, &SidebarButton, CUI::CORNER_R, 5.0f, true))
	{
		m_SidebarActive ^= 1;
	}

	float Spacing = 3.0f;
	float ButtonWidth = (MainView.w/6.0f)-(Spacing*5.0)/6.0f;

	MainView.VSplitRight(ButtonWidth+Spacing, &MainView, 0);
	MainView.HSplitBottom(60.0f, 0, &BottomBox);
	BottomBox.VSplitRight(ButtonWidth*2.0f+Spacing, 0, &BottomBox);

	// connect box
	{
		RenderServerbrowserBottomBox(BottomBox);
	}

	// back button
	RenderBackButton(MainView);

	// render overlay if there is any
	RenderServerbrowserOverlay();
}

// friend list
bool CMenus::SortCompareName(int Index1, int Index2) const
{
	const CFriendItem *a = &m_lFriends[Index1];
	const CFriendItem *b = &m_lFriends[Index2];
	return str_comp_nocase(a->m_pFriendInfo->m_aName, b->m_pFriendInfo->m_aName) < 0;
}

bool CMenus::SortCompareClan(int Index1, int Index2) const
{
	const CFriendItem *a = &m_lFriends[Index1];
	const CFriendItem *b = &m_lFriends[Index2];
	return str_comp_nocase(a->m_pFriendInfo->m_aClan, b->m_pFriendInfo->m_aClan) < 0;
}

bool CMenus::SortCompareServer(int Index1, int Index2) const
{
	const CFriendItem *a = &m_lFriends[Index1];
	const CFriendItem *b = &m_lFriends[Index2];
	if(a->m_pServerInfo && !b->m_pServerInfo)
		return true;
	else if(!a->m_pServerInfo && b->m_pServerInfo)
		return false;
	return a->m_pServerInfo && b->m_pServerInfo && str_comp_nocase(a->m_pServerInfo->m_aName, b->m_pServerInfo->m_aName) < 0;
}

bool CMenus::SortCompareType(int Index1, int Index2) const
{
	const CFriendItem *a = &m_lFriends[Index1];
	const CFriendItem *b = &m_lFriends[Index2];
	return a->m_NumFound < b->m_NumFound;
}

void CMenus::SortFriends()
{
	// sort
	switch(g_Config.m_BrFriendSort)
	{
	case FRIENDS_SORT_NAME:
		std::stable_sort(m_pFriendIndexes, m_pFriendIndexes + m_lFriends.size(), SortWrap(this, &CMenus::SortCompareName));
		break;
	case FRIENDS_SORT_CLAN:
		std::stable_sort(m_pFriendIndexes, m_pFriendIndexes + m_lFriends.size(), SortWrap(this, &CMenus::SortCompareClan));
		break;
	case FRIENDS_SORT_SERVER:
		std::stable_sort(m_pFriendIndexes, m_pFriendIndexes + m_lFriends.size(), SortWrap(this, &CMenus::SortCompareServer));
		break;
	case FRIENDS_SORT_TYPE:
		std::stable_sort(m_pFriendIndexes, m_pFriendIndexes + m_lFriends.size(), SortWrap(this, &CMenus::SortCompareType));
	}
}

void CMenus::UpdateFriendCounter(const CServerInfo *pEntry)
{
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
						if(!m_lFriends[f].IsClanFriend())
							m_lFriends[f].m_pServerInfo = pEntry;
						else
							m_lFriends[f].m_pServerInfo = 0;
						if(m_lFriends[f].m_pFriendInfo->m_aName[0])
							break;
					}

					// sort friend in clan friend
					if(ClanHash == m_lFriends[f].m_pFriendInfo->m_ClanHash && !m_lFriends[f].m_pFriendInfo->m_aName[0])
					{
						CFriendInfo FriendInfo = CFriendInfo();
						str_copy(FriendInfo.m_aName, pEntry->m_aClients[j].m_aName, sizeof(FriendInfo.m_aName));
						str_copy(FriendInfo.m_aClan, pEntry->m_aClients[j].m_aClan, sizeof(FriendInfo.m_aClan));
						FriendInfo.m_NameHash = NameHash;
						FriendInfo.m_ClanHash = ClanHash;
						m_lFriends[f].m_ClanFriend.m_lFriendInfos.add(FriendInfo);
						m_lFriends[f].m_ClanFriend.m_lServerInfos.add(pEntry);
					}
				}
			}
		}
	}
}

void CMenus::RenderServerbrowserFriendList(CUIRect View)
{
	static int s_Inited = 0;
	if(!s_Inited)
	{
		FriendlistOnUpdate();
		s_Inited = 1;
	}

	CUIRect Headers, Status, InfoButton;

	float SpacingH = 2.0f;
	float ButtonHeight = 20.0f;

	// background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	View.HSplitTop(ms_ListheaderHeight, &Headers, &View);
	View.HSplitBottom(ButtonHeight*3.0f + SpacingH * 2.0f, &View, &Status);

	Headers.VSplitRight(ms_ListheaderHeight, &Headers, &InfoButton); // split for info button

	// do layout
	for(int i = 0; i < NUM_FRIEND_COLS; i++)
	{
		if(ms_aFriendCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(ms_aFriendCols[i].m_Width, &ms_aFriendCols[i].m_Rect, &Headers);

			if(i + 1 < NUM_FRIEND_COLS)
			{
				//Cols[i].flags |= SPACER;
				Headers.VSplitLeft(2, &ms_aFriendCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NUM_FRIEND_COLS - 1; i >= 0; i--)
	{
		if(ms_aFriendCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(ms_aFriendCols[i].m_Width, &Headers, &ms_aFriendCols[i].m_Rect);
			Headers.VSplitRight(2, &Headers, &ms_aFriendCols[i].m_Spacer);
		}
	}

	for(int i = 0; i < NUM_FRIEND_COLS; i++)
	{
		if(ms_aFriendCols[i].m_Direction == 0)
			ms_aFriendCols[i].m_Rect = Headers;
	}

	// do headers
	for(int i = 0; i < NUM_FRIEND_COLS; i++)
	{
		if(DoButton_GridHeader(ms_aFriendCols[i].m_Caption, ms_aFriendCols[i].m_Caption, g_Config.m_BrFriendSort == ms_aFriendCols[i].m_Sort, &ms_aFriendCols[i].m_Rect))
		{
			if(ms_aFriendCols[i].m_Sort != -1)
			{
				if(g_Config.m_BrFriendSort == ms_aFriendCols[i].m_Sort)
					g_Config.m_BrFriendSortOrder ^= 1;
				else
					g_Config.m_BrFriendSortOrder = 0;
				g_Config.m_BrFriendSort = ms_aFriendCols[i].m_Sort;
				SortFriends();

				int Items = 0;
				for(int j = 0; j < m_lFriends.size(); j++)
				{
					CFriendItem *pFriend = &m_lFriends[m_pFriendIndexes[j]];
					if(!m_SelectedFriend.m_FakeFriend && m_SelectedFriend.m_ClanFriend == pFriend->IsClanFriend() && m_SelectedFriend.m_ClanHash == pFriend->m_pFriendInfo->m_ClanHash && pFriend->m_pFriendInfo->m_NameHash)
					{
						m_FriendlistSelectedIndex = Items;
						break;
					}
					Items++;

					if(pFriend->IsClanFriend())
					{
						for(int k = 0; k < pFriend->m_NumFound; k++)
						{
							if(m_SelectedFriend.m_FakeFriend && m_SelectedFriend.m_ClanFriend == pFriend->IsClanFriend() && m_SelectedFriend.m_ClanHash == pFriend->m_ClanFriend.m_lFriendInfos[k].m_ClanHash && pFriend->m_ClanFriend.m_lFriendInfos[k].m_NameHash)
							{
								m_FriendlistSelectedIndex = Items;
								break;
							}
							Items++;
						}
					}
				}
			}
		}
	}

	// split scrollbar from view
	CUIRect Scroll;
	View.VSplitRight(20.0f, &View, &Scroll);

	// scrollbar background
	RenderTools()->DrawUIRect(&Scroll, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// list background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	{
		int Column = g_Config.m_BrFriendSort;

		CUIRect Rect = View;
		Rect.x = CMenus::ms_aFriendCols[Column].m_Rect.x;
		Rect.w = CMenus::ms_aFriendCols[Column].m_Rect.w;
		RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.05f), CUI::CORNER_ALL, 5.0f);
	}

	// count all the servers
	int NumFriends = m_lFriends.size();

	int NumEntries = 0;
	int ClanEntries = 0;
	for(int i = 0; i < NumFriends; i++)
	{
		CFriendItem *pFriend = &m_lFriends[m_pFriendIndexes[i]];
		NumEntries++;
		if(pFriend->IsClanFriend())
		{
			NumEntries += pFriend->m_ClanFriend.m_lFriendInfos.size();
			ClanEntries++;
		}
	}

	float ListHeight = ms_aBrowserCols[0].m_Rect.h + NumEntries * 20.0f + ClanEntries * 20.0f;

	//int Num = (int)((ListHeight-View.h)/ms_aBrowserCols[0].m_Rect.h))+1;
	//int Num = (int)(View.h/ms_aBrowserCols[0].m_Rect.h) + 1;
	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;

	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

	int ScrollNum = (int)((ListHeight - View.h) / ms_aBrowserCols[0].m_Rect.h) + 1;
	if(ScrollNum > 0)
	{
		/*if(m_ScrollOffset)
		{
			s_ScrollValue = (float)(m_ScrollOffset) / ScrollNum;
			m_ScrollOffset = 0;
		}*/
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP) && UI()->MouseInside(&View))
			s_ScrollValue -= 3.0f / ScrollNum;
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) && UI()->MouseInside(&View))
			s_ScrollValue += 3.0f / ScrollNum;
	}
	else
		ScrollNum = 0;

	int SelectedIndex = m_FriendlistSelectedIndex;
	if(SelectedIndex > -1)
	{
		int NewIndex = -1;
		if(m_DownArrowPressed)
		{
			NewIndex = SelectedIndex + 1;
			if(NewIndex >= NumEntries)
				NewIndex = NumEntries;
		}
		else if(m_UpArrowPressed)
		{
			NewIndex = SelectedIndex - 1;
			if(NewIndex < 0)
				NewIndex = 0;
		}
		if(NewIndex > -1 && NewIndex < NumEntries)
		{
			//scroll
			float IndexY = View.y - s_ScrollValue * ScrollNum*ms_aFriendCols[0].m_Rect.h + NewIndex * ms_aFriendCols[0].m_Rect.h;
			int Scroll = View.y > IndexY ? -1 : View.y + View.h < IndexY + ms_aFriendCols[0].m_Rect.h ? 1 : 0;
			if(Scroll)
			{
				if(Scroll < 0)
				{
					int NumScrolls = (View.y - IndexY + ms_aFriendCols[0].m_Rect.h - 1.0f) / ms_aFriendCols[0].m_Rect.h;
					s_ScrollValue -= (1.0f / ScrollNum)*NumScrolls;
				}
				else
				{
					int NumScrolls = (IndexY + ms_aFriendCols[0].m_Rect.h - (View.y + View.h) + ms_aFriendCols[0].m_Rect.h - 1.0f) / ms_aFriendCols[0].m_Rect.h;
					s_ScrollValue += (1.0f / ScrollNum)*NumScrolls;
				}
			}

			m_FriendlistSelectedIndex = NewIndex;
		}
	}

	if(s_ScrollValue < 0) s_ScrollValue = 0;
	if(s_ScrollValue > 1) s_ScrollValue = 1;

	// reset friend counter
	for(int i = 0; i < m_lFriends.size(); m_lFriends[i++].Reset());

	// get all the friends only from all filter
	for(int s = 0; s < m_lFilters.size(); s++)
	{
		CBrowserFilter *pFilter = &m_lFilters[s];
		if(pFilter->Custom() == CBrowserFilter::FILTER_ALL)
		{
			for(int i = 0; i < pFilter->NumSortedServers(); i++)
			{
				const CServerInfo *pItem = pFilter->SortedGet(i);
				UpdateFriendCounter(pItem);
			}
		}
	}

	// set clipping
	UI()->ClipEnable(&View);

	CUIRect OriginalView = View;
	View.y -= s_ScrollValue * ScrollNum*ms_aFriendCols[0].m_Rect.h;

	int Items = 0;
	for(int i = 0; i < NumFriends; i++)
	{
		CFriendItem *pFriend = &m_lFriends[m_pFriendIndexes[i]];

		// scip clan friends
		if(Items == m_FriendlistSelectedIndex && pFriend->IsClanFriend())
		{
			if(SelectedIndex > m_FriendlistSelectedIndex && m_FriendlistSelectedIndex > 0)
				m_FriendlistSelectedIndex--;
			else if(m_FriendlistSelectedIndex < NumEntries)
				m_FriendlistSelectedIndex++;
		}
		if(View.y >= OriginalView.y)
			DoFriendListEntry(&View, pFriend, pFriend->m_pFriendInfo, pFriend->m_pFriendInfo, pFriend->m_pServerInfo, Items == m_FriendlistSelectedIndex);
		else
			View.HSplitTop(ms_ListheaderHeight, 0, &View);
		Items++;

		if(UI()->CheckActiveItem(pFriend->m_pFriendInfo))
			m_FriendlistSelectedIndex = Items - 1;

		if(pFriend->IsClanFriend())
		{
			for(int j = 0; j < pFriend->m_NumFound; j++)
			{
				if(View.y >= OriginalView.y)
					DoFriendListEntry(&View, pFriend, &pFriend->m_ClanFriend.m_lFriendInfos+j, &pFriend->m_ClanFriend.m_lFriendInfos[j], pFriend->m_ClanFriend.m_lServerInfos[j], Items == m_FriendlistSelectedIndex, true);
				else
					View.HSplitTop(ms_ListheaderHeight, 0, &View);
				Items++;

				if(UI()->CheckActiveItem(&pFriend->m_ClanFriend.m_lFriendInfos + j))
					m_FriendlistSelectedIndex = Items - 1;
			}

			View.HSplitTop(ms_ListheaderHeight, 0, &View);
		}
	}

	UI()->ClipDisable();

	// add friend thingy
	CUIRect Button, ButtonBackground;
	Status.HMargin(ButtonHeight + SpacingH, &Status);
	Status.VSplitRight(20.0f, &Status, 0);
	Status.VSplitRight(20.0f + 150.0f + 120.0f + SpacingH * 2.0f, 0, &ButtonBackground);
	Status.VSplitRight(20.0f, &Status, &Button);

	// background for buttons
	RenderTools()->DrawUIRect(&ButtonBackground, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	static char s_aClan[MAX_CLAN_LENGTH] = { 0 };
	static char s_aName[MAX_NAME_LENGTH] = { 0 };

	static int s_AddButton = 0;
	Button.Margin(1.0f, &Button);
	if(DoButton_SpriteCleanID(&s_AddButton, IMAGE_FRIENDICONS, SPRITE_FRIEND_PLUS_A, &Button, false))
	{
		m_pClient->Friends()->AddFriend(s_aName, s_aClan);
		FriendlistOnUpdate();
		Client()->ServerBrowserUpdate();
	}

	Status.VSplitRight(SpacingH, &Status, 0);
	Status.VSplitRight(120.0f, &Status, &Button);

	static float s_OffsetClan = 0.0f;
	DoEditBox(&s_aClan, &Button, s_aClan, sizeof(s_aClan), Button.h*ms_FontmodHeight*0.8f, &s_OffsetClan);

	Status.VSplitRight(SpacingH, &Status, 0);
	Status.VSplitRight(150.0f, &Status, &Button);

	static float s_OffsetName = 0.0f;
	DoEditBox(&s_aName, &Button, s_aName, sizeof(s_aName), Button.h*ms_FontmodHeight*0.8f, &s_OffsetName);

	// get all numbers we need
	int NumSubjects = 0;
	int NumPlayerFriends = 0;
	int NumClanFriends = 0;
	for(int i = 0; i < NumFriends; i++)
	{
		CFriendItem *pFriend = &m_lFriends[m_pFriendIndexes[i]];
		if(pFriend->m_NumFound)
		{
			NumSubjects++;
			if(pFriend->IsClanFriend())
			{
				NumSubjects += pFriend->m_ClanFriend.m_lFriendInfos.size();
				NumClanFriends++;
			}
			else
			{
				NumPlayerFriends++;
			}
		}
	}

	Status.VSplitLeft(10.0f, 0, &Status);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("%d subjects, %d friends and %d clans online"), NumSubjects, NumPlayerFriends, NumClanFriends);
	UI()->DoLabel(&Status, aBuf, Button.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	// delete friend
	if(m_pDeleteFriendInfo)
	{
		m_pClient->Friends()->RemoveFriend(m_pDeleteFriendInfo->m_aName, m_pDeleteFriendInfo->m_aClan);
		FriendlistOnUpdate();
		m_pDeleteFriendInfo = 0;
		UI()->SetActiveItem(0); // reset active item!
	}
}

void CMenus::DoFriendListEntry(CUIRect *pView, CFriendItem *pFriend, const void *pID, const CFriendInfo *pFriendInfo, const CServerInfo *pServerInfo, bool Checked, bool Clan)
{
	CUIRect Row;
	pView->HSplitTop(ms_ListheaderHeight, &Row, pView);

	// entry background for selected item
	if(Checked)
		RenderTools()->DrawUIRect(&Row, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 4.0f);

	// entry background for clan item
	if(pFriend->IsClanFriend() && !Clan && !Checked)
		RenderTools()->DrawUIRect(&Row, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 4.0f);

	int Inside = UI()->MouseInside(&Row);

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
	}
	if(UI()->HotItem() == pID && (!pFriend->IsClanFriend() || Clan))
	{
		if(UI()->MouseButton(0))
		{
			// store selected friend
			m_SelectedFriend.m_ClanFriend = pFriend->IsClanFriend();
			m_SelectedFriend.m_FakeFriend = Clan;
			m_SelectedFriend.m_NameHash = pFriendInfo->m_NameHash;
			m_SelectedFriend.m_ClanHash = pFriendInfo->m_ClanHash;

			UI()->SetActiveItem(pID);
		}

		CUIRect r = Row;
		RenderTools()->DrawUIRect(&r, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 4.0f);
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// select server in case the friend is online
	if(pFriend->m_NumFound && pServerInfo && Checked)
	{
		str_copy(g_Config.m_UiServerAddress, pServerInfo->m_aAddress, sizeof(g_Config.m_UiServerAddress));
		if(Input()->MouseDoubleClick())
			Client()->Connect(g_Config.m_UiServerAddress);
	}

	for(int c = 0; c < NUM_FRIEND_COLS; c++)
	{
		vec3 TextBaseColor = vec3(1.0f, 1.0f, 1.0f);
		if(!pFriend->IsClanFriend() || Clan)
		{
			if(Checked || UI()->HotItem() == pID)
			{
				TextBaseColor = vec3(0.0f, 0.0f, 0.0f);
				TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
			}
		}
		bool RealFriend = false;
		if(Clan)
		{
			for(int i = 0; i < m_lFriends.size(); i++)
			{
				CFriendItem *pFriend = &m_lFriends[i];
				if(!pFriend->IsClanFriend())
				{
					if(pFriend->m_pFriendInfo->m_NameHash == pFriendInfo->m_NameHash && pFriend->m_pFriendInfo->m_ClanHash == pFriendInfo->m_ClanHash)
					{
						RealFriend = true;
						break;
					}
				}
			}
		}
		float Alpha = Clan && !RealFriend ? 0.5f : 1.0f;

		CUIRect Button;
		Button.x = ms_aFriendCols[c].m_Rect.x;
		Button.y = Row.y;
		Button.h = Row.h;
		Button.w = ms_aFriendCols[c].m_Rect.w;

		int ID = ms_aFriendCols[c].m_ID;

		if(ID != COL_FRIEND_DELETE && (ID != COL_FRIEND_TYPE || !Clan))
		{
			char aBuf[128];
			CTextCursor Cursor;
			switch(ID)
			{
			case COL_FRIEND_NAME:
				str_copy(aBuf, pFriendInfo->m_aName, sizeof(aBuf));
				break;
			case COL_FRIEND_CLAN:
				str_copy(aBuf, pFriendInfo->m_aClan, sizeof(aBuf));
				break;
			case COL_FRIEND_SERVER:
				str_copy(aBuf, pFriend->m_NumFound && pServerInfo ? pServerInfo->m_aName : "", sizeof(aBuf));
				break;
			case COL_FRIEND_TYPE:
				if(pFriend->m_NumFound)
				{
					TextBaseColor = vec3(0.0f, 1.0f, 0.0f);
					char aState[8];
					if(pFriend->IsClanFriend())
						str_format(aState, sizeof(aState), "%d", pFriend->m_NumFound);
					else
						str_copy(aState, Localize("On"), sizeof(aState));
					str_copy(aBuf, aState, sizeof(aBuf));
				}
				else
				{
					TextBaseColor = vec3(1.0f, 0.0f, 0.0f);
					str_copy(aBuf, pFriend->IsClanFriend() ? "0" : Localize("Off"), sizeof(aBuf));
				}
			}
			if(!aBuf[0])
				str_copy(aBuf, "", sizeof(aBuf));

			float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1);
			if(tw < Button.w)
				TextRender()->SetCursor(&Cursor, Button.x + Button.w / 2.0f - tw / 2.0f, Button.y, 12.0f, TEXTFLAG_RENDER);
			else
			{
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
			}

			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, Alpha);

			TextRender()->TextEx(&Cursor, aBuf, -1);
		}
		else if(!Clan && (UI()->HotItem() == pID || pFriend->IsClanFriend()))
		{
			// delete button
			Button.Margin(1.0f, &Button);
			if(DoButton_SpriteClean(IMAGE_FRIENDICONS, pFriend->IsClanFriend() ? SPRITE_FRIEND_X_A : SPRITE_FRIEND_X_B, &Button))
			{
				m_pDeleteFriendInfo = pFriend->m_pFriendInfo;
			}
		}
	}

	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
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
