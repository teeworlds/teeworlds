/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <algorithm> // sort  TODO: remove this

#include <engine/external/json-parser/json.h>

#include <engine/config.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <engine/client/contacts.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>

#include "menus.h"

CMenus::CColumn CMenus::ms_aBrowserCols[] = {  // Localize("Server"); Localize("Type"); Localize("Map"); Localize("Players"); Localize("Ping"); - these strings are localized within CLocConstString
	{COL_BROWSER_FLAG,		-1,									" ",		-1, 4*16.0f+3*2.0f, 0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_BROWSER_NAME,		IServerBrowser::SORT_NAME,			"Server",	0, 310.0f, 0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_BROWSER_GAMETYPE,	IServerBrowser::SORT_GAMETYPE,		"Type",		1, 70.0f,  0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_BROWSER_MAP,		IServerBrowser::SORT_MAP,			"Map",		1, 100.0f, 0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_BROWSER_PLAYERS,	IServerBrowser::SORT_NUMPLAYERS,	"Players",	1, 50.0f,  0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_BROWSER_PING,		IServerBrowser::SORT_PING,			"Ping",		1, 40.0f,  0, {0}, {0}, CUI::ALIGN_CENTER},
};

CServerFilterInfo CMenus::CBrowserFilter::ms_FilterStandard = {IServerBrowser::FILTER_COMPAT_VERSION|IServerBrowser::FILTER_PURE|IServerBrowser::FILTER_PURE_MAP, 999, -1, 0, {{0}}, {0}};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterFavorites = {IServerBrowser::FILTER_COMPAT_VERSION|IServerBrowser::FILTER_FAVORITE, 999, -1, 0, {{0}}, {0}};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterAll = {IServerBrowser::FILTER_COMPAT_VERSION, 999, -1, 0, {{0}}, {0}};

vec3 TextHighlightColor = vec3(0.4f, 0.4f, 1.0f);

// filters
CMenus::CBrowserFilter::CBrowserFilter(int Custom, const char* pName, IServerBrowser *pServerBrowser)
{
	m_Extended = false;
	m_Custom = Custom;
	for(int Type = 0; Type < IServerBrowser::NUM_TYPES; Type++)
		m_aSelectedServers[Type] = -1;
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
		SetFilter(&ms_FilterStandard);
		break;
	case CBrowserFilter::FILTER_FAVORITES:
		SetFilter(&ms_FilterFavorites);
		break;
	default:
		SetFilter(&ms_FilterAll);
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

const CServerInfo* CMenus::CBrowserFilter::SortedGet(int Index) const
{
	if(Index < 0 || Index >= m_pServerBrowser->NumSortedServers(m_Filter))
		return 0;
	return m_pServerBrowser->SortedGet(m_Filter, Index);
}

const CServerInfo* CMenus::CBrowserFilter::SortedGetSelected() const
{
	return SortedGet(m_aSelectedServers[m_pServerBrowser->GetType()]);
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
	char *pFileData = (char *)mem_alloc(FileSize, 1);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);

	if(pJsonData == 0)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, pFilename, aError);
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

// 1 = browser entry click, 2 = server info click
int CMenus::DoBrowserEntry(const void *pID, CUIRect View, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected)
{
	// logic
	int ReturnValue = 0;
	int Inside = UI()->MouseInside(&View);

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
		{
			if(Inside)
				ReturnValue = 1;
			UI()->SetActiveItem(0);
		}
	}
	if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
			UI()->SetActiveItem(pID);
	}

	if(Inside)
	{
		UI()->SetHotItem(pID);
		RenderTools()->DrawUIRect(&View, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 4.0f);
	}

	vec3 TextBaseColor = vec3(1.0f, 1.0f, 1.0f);
	if(Selected || Inside)
	{
		TextBaseColor = vec3(0.0f, 0.0f, 0.0f);
		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
	}

	float TextAlpha = (pEntry->m_NumClients == pEntry->m_MaxClients) ? 0.5f : 1.0f;
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

			DoIcon(IMAGE_LEVELICONS, Level==0 ? SPRITE_LEVEL_A_ON : Level==1 ? SPRITE_LEVEL_B_ON : SPRITE_LEVEL_C_ON, &Icon);

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
			if(pEntry->m_FriendState != CContactInfo::CONTACT_NO)
			{
				Icon.Margin(2.0f, &Icon);
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BROWSEICONS].m_Id);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f, 0.75f, 1.0f, 1.0f);
				RenderTools()->SelectSprite(Selected ? SPRITE_BROWSE_HEART_B : SPRITE_BROWSE_HEART_A);
				IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				Graphics()->QuadsEnd();
			}
		}
		else if(ID == COL_BROWSER_NAME)
		{
			CTextCursor Cursor;
			float tw = TextRender()->TextWidth(0, 12.0f, pEntry->m_aName, -1, -1.0f);
			if(tw < Button.w)
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER);
			else
			{
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
			}
			
			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME))
			{
				// highlight the part that matches
				const char *pStr = str_find_nocase(pEntry->m_aName, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aName, (int)(pStr-pEntry->m_aName));
					TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextAlpha);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);
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
			float tw = TextRender()->TextWidth(0, 12.0f, pEntry->m_aMap, -1, -1.0f);
			if(tw < Button.w)
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER);
			else
			{
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;
			}

			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_MAPNAME))
			{
				// highlight the part that matches
				const char *pStr = str_find_nocase(pEntry->m_aMap, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aMap, (int)(pStr-pEntry->m_aMap));
					TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextAlpha);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);
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
			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);
			CServerFilterInfo FilterInfo;
			pFilter->GetFilter(&FilterInfo);

			int Num = (FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS) ? pEntry->m_NumPlayers : pEntry->m_NumClients;
			int Max = (FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS) ? pEntry->m_MaxPlayers : pEntry->m_MaxClients;
			if(FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS)
			{
				int SpecNum = pEntry->m_NumClients - pEntry->m_NumPlayers;
				if(pEntry->m_MaxClients - pEntry->m_MaxPlayers < SpecNum)
					Max -= SpecNum;
			}
			if(FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS)
			{
				Num -= pEntry->m_NumBotPlayers;
				Max -= pEntry->m_NumBotPlayers;
				if(!(FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS))
				{
					Num -= pEntry->m_NumBotSpectators;
					Max -= pEntry->m_NumBotSpectators;
				}

			}
			static float RenderOffset = 0.0f;
			if(RenderOffset == 0.0f)
			{
				char aChar[2] = "0";
				RenderOffset = TextRender()->TextWidth(0, 12.0f, aChar, -1, -1.0f);
			}

			str_format(aTemp, sizeof(aTemp), "%d/%d", Num, Max);
			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_PLAYER))
				TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextAlpha);
			Button.y += 2.0f;

			if(Num < 100)
				Button.x += RenderOffset;
			if(Num < 10)
				Button.x += RenderOffset;
			if(!Num)
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
			UI()->DoLabel(&Button, aTemp, 12.0f, CUI::ALIGN_LEFT);
			Button.x += TextRender()->TextWidth(0, 12.0f, aTemp, -1, -1.0f);
		}
		else if(ID == COL_BROWSER_PING)
		{
			int Ping = pEntry->m_Latency;

			vec4 Color;
			if(Selected || Inside)
			{
				Color = vec4(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);
			}
			else
			{
				vec4 StartColor;
				vec4 EndColor;
				float MixVal;
				if(Ping <= 125)
				{
					StartColor = vec4(0.0f, 1.0f, 0.0f, TextAlpha);
					EndColor = vec4(1.0f, 1.0f, 0.0f, TextAlpha);
					
					MixVal = (Ping-50.0f)/75.0f;
				}
				else
				{
					StartColor = vec4(1.0f, 1.0f, 0.0f, TextAlpha);
					EndColor = vec4(1.0f, 0.0f, 0.0f, TextAlpha);
					
					MixVal = (Ping-125.0f)/75.0f;
				}
				Color = mix(StartColor, EndColor, MixVal);
			}
			
			str_format(aTemp, sizeof(aTemp), "%d", pEntry->m_Latency);
			TextRender()->TextColor(Color.r, Color.g, Color.b, Color.a);
			Button.y += 2.0f;
			Button.w -= 4.0f;
			UI()->DoLabel(&Button, aTemp, 12.0f, CUI::ALIGN_RIGHT);
		}
		else if(ID == COL_BROWSER_GAMETYPE)
		{
			// gametype icon
			CUIRect Icon;
			Button.VSplitLeft(Button.h, &Icon, &Button);
			Icon.y -= 0.5f;
			DoGameIcon(pEntry->m_aGameType, &Icon);

			// gametype text
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Button.w;
			TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);

			if(g_Config.m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_GAMETYPE))
			{
				// highlight the part that matches
				const char *pStr = str_find_nocase(pEntry->m_aGameType, g_Config.m_BrFilterString);
				if(pStr)
				{
					TextRender()->TextEx(&Cursor, pEntry->m_aGameType, (int)(pStr-pEntry->m_aGameType));
					TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextAlpha);
					TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha);
					TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pEntry->m_aGameType, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pEntry->m_aGameType, -1);
		}
	}

	// show server info
	if(!m_SidebarActive && m_ShowServerDetails && Selected)
	{
		CUIRect Info;
		View.HSplitTop(ms_aBrowserCols[0].m_Rect.h, 0, &View);

		if(ReturnValue && UI()->MouseInside(&View))
			ReturnValue++;

		View.VSplitLeft(160.0f, &Info, &View);
		RenderDetailInfo(Info, pEntry);

		CUIRect NewClipArea = *UI()->ClipArea();
		CUIRect OldClipArea = NewClipArea;
		NewClipArea.x = View.x;
		NewClipArea.w = View.w;
		UI()->ClipEnable(&NewClipArea);
		RenderDetailScoreboard(View, pEntry, 4, vec4(TextBaseColor.r, TextBaseColor.g, TextBaseColor.b, TextAlpha));
		UI()->ClipEnable(&OldClipArea);
	}

	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	return ReturnValue;
}

void CMenus::RenderFilterHeader(CUIRect View, int FilterIndex)
{
	CBrowserFilter *pFilter = &m_lFilters[FilterIndex];

	float ButtonHeight = 20.0f;
	float Spacing = 3.0f;
	bool Switch = false;

	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Button, EditButtons;
	if(UI()->DoButtonLogic(pFilter, "", 0, &View))
	{
		Switch = true; // switch later, to make sure we haven't clicked one of the filter buttons (edit...)
	}
	vec4 Color = UI()->MouseInside(&View) ? vec4(1.0f, 1.0f, 1.0f, 1.0f) : vec4(0.6f, 0.6f, 0.6f, 1.0f);
	View.VSplitLeft(20.0f, &Button, &View);
	Button.Margin(2.0f, &Button);
	DoIconColor(IMAGE_MENUICONS, pFilter->Extended() ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED, &Button, Color);
	
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
	
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(pFilter->Custom() == CBrowserFilter::FILTER_CUSTOM)
	{
		if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_X_A, &Button))
		{
			m_RemoveFilterIndex = FilterIndex;
			m_Popup = POPUP_REMOVE_FILTER;
		}
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_X_B, &Button);

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(FilterIndex > 0 && (pFilter->Custom() > CBrowserFilter::FILTER_ALL || m_lFilters[FilterIndex-1].Custom() != CBrowserFilter::FILTER_STANDARD))
	{
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_UP_A, &Button);
		if(UI()->DoButtonLogic(&pFilter->m_aButtonID[0], "", 0, &Button))
		{
			Move(true, FilterIndex);
			Switch = false;
		}
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_UP_B, &Button);

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(FilterIndex >= 0 && FilterIndex < m_lFilters.size()-1 && (pFilter->Custom() != CBrowserFilter::FILTER_STANDARD || m_lFilters[FilterIndex+1].Custom() > CBrowserFilter::FILTER_ALL))
	{
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_DOWN_A, &Button);
		if(UI()->DoButtonLogic(&pFilter->m_aButtonID[1], "", 0, &Button))
		{
			Move(false, FilterIndex);
			Switch = false;
		}
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_DOWN_B, &Button);

	if(Switch)
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
}

static void FormatScore(char *pBuf, int BufSize, bool TimeScore, const CServerInfo::CClient *pClient)
{
	if(TimeScore)
		FormatTime(pBuf, BufSize, pClient->m_Score * 1000, 0);
	else
		str_format(pBuf, BufSize, "%d", pClient->m_Score);
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
		vec4 TextColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
		TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);

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
					if(pInfo->m_aClients[i].m_FriendState == CContactInfo::CONTACT_PLAYER)
						m_pClient->Friends()->RemoveFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
					else
						m_pClient->Friends()->AddFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
					FriendlistOnUpdate();
					Client()->ServerBrowserUpdate();
				}

				vec4 Colour = pInfo->m_aClients[i].m_FriendState == CContactInfo::CONTACT_NO ? vec4(1.0f, 1.0f, 1.0f, (i%2+1)*0.05f) :
																									vec4(0.5f, 1.0f, 0.5f, 0.15f+(i%2+1)*0.05f);
				RenderTools()->DrawUIRect(&Name, Colour, CUI::CORNER_ALL, 4.0f);
				Name.VSplitLeft(5.0f, 0, &Name);
				Name.VSplitLeft(30.0f, &Score, &Name);
				Name.VSplitRight(34.0f, &Name, &Flag);
				Flag.HMargin(4.0f, &Flag);
				Name.VSplitRight(40.0f, &Name, &Clan);

				// score
				if(!(pInfo->m_aClients[i].m_PlayerType&CServerInfo::CClient::PLAYERFLAG_SPEC))
				{
					char aTemp[16];
					FormatScore(aTemp, sizeof(aTemp), pInfo->m_Flags&IServerBrowser::FLAG_TIMESCORE, &pInfo->m_aClients[i]);
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
					// highlight the part that matches
					const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
					if(s)
					{
						TextRender()->TextEx(&Cursor, pName, (int)(s-pName));
						TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextColor.a);
						TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);
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
					// highlight the part that matches
					const char *s = str_find_nocase(pClan, g_Config.m_BrFilterString);
					if(s)
					{
						TextRender()->TextEx(&Cursor, pClan, (int)(s-pClan));
						TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextColor.a);
						TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);
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
			UI()->DoLabel(&View, Localize("no players", "server browser message"), View.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
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
	CUIRect Headers, Status;

	float SpacingH = 2.0f;
	float ButtonHeight = 20.0f;

	// background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), (Client()->State() == IClient::STATE_OFFLINE) ? CUI::CORNER_ALL : CUI::CORNER_B|CUI::CORNER_TR, 5.0f);

	// make room for scrollbar
	{
		CUIRect Scroll;
		View.VSplitRight(20.0f, &View, &Scroll);
	}

	View.HSplitTop(GetListHeaderHeight(), &Headers, &View);
	View.HSplitBottom(ButtonHeight*3.0f+SpacingH*2.0f, &View, &Status);

	Headers.VSplitRight(2.f, &Headers, 0); // some margin on the right

	// do layout
	for(int i = 0; i < NUM_BROWSER_COLS; i++)
	{
		if(ms_aBrowserCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(ms_aBrowserCols[i].m_Width*GetListHeaderHeightFactor(), &ms_aBrowserCols[i].m_Rect, &Headers);

			if(i+1 < NUM_BROWSER_COLS)
			{
				Headers.VSplitLeft(2, &ms_aBrowserCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NUM_BROWSER_COLS-1; i >= 0; i--)
	{
		if(ms_aBrowserCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(ms_aBrowserCols[i].m_Width*GetListHeaderHeightFactor(), &Headers, &ms_aBrowserCols[i].m_Rect);
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
			continue;

		if(DoButton_GridHeader(ms_aBrowserCols[i].m_Caption, ms_aBrowserCols[i].m_Caption, g_Config.m_BrSort == ms_aBrowserCols[i].m_Sort, ms_aBrowserCols[i].m_Align, &ms_aBrowserCols[i].m_Rect))
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

	const int BrowserType = ServerBrowser()->GetType();
	int ToBeSelectedFilter = -2; // -2 to not restore, -1 to restore all filters closed
	int ToBeSelectedServer = -1;
	// update selection based on address if it changed
	if(!(m_AddressSelection&ADDR_SELECTION_CHANGE) && m_aLastServerAddress[0] && str_comp(m_aLastServerAddress, g_Config.m_UiServerAddress) != 0)
		m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_REVEAL;
	else if(ServerBrowser()->IsRefreshing())
		m_AddressSelection |= ADDR_SELECTION_CHANGE;

	if(m_LastBrowserType == -1)
		m_LastBrowserType = BrowserType;
	else if(BrowserType != m_LastBrowserType)
	{
		// restore selected filter and server when changing browser page
		m_LastBrowserType = BrowserType;
		ToBeSelectedFilter = m_aSelectedFilters[BrowserType];
		if(ToBeSelectedFilter != -1)
		{
			ToBeSelectedServer = m_lFilters[ToBeSelectedFilter].m_aSelectedServers[BrowserType];
			if(ToBeSelectedServer == -1)
			{
				m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_ADDRESS_IF_NOT_FOUND | ADDR_SELECTION_REVEAL;
			}
		}
		m_AddressSelection |= ADDR_SELECTION_UPDATE_ADDRESS;
	}

	// count all the servers and update selected filter based on UI state
	int NumServers = 0;
	int SelectedFilter = -1;
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		// restore selected filter from browser page
		if(ToBeSelectedFilter != -2 && (ToBeSelectedFilter == i) != m_lFilters[i].Extended())
		{
			m_lFilters[i].Switch();
		}

		if(m_lFilters[i].Extended())
		{
			if(SelectedFilter == -1)
			{
				SelectedFilter = i;
			}
			NumServers += m_lFilters[i].NumSortedServers();
		}
	}

	if(m_aSelectedFilters[BrowserType] == -2)
		m_aSelectedFilters[BrowserType] = SelectedFilter;
	else if(SelectedFilter != m_aSelectedFilters[BrowserType])
	{
		// update stored state based on updated state of UI
		m_aSelectedFilters[BrowserType] = SelectedFilter;
		if(SelectedFilter != -1)
		{
			m_AddressSelection |= ADDR_SELECTION_REVEAL;
			if(m_lFilters[SelectedFilter].m_aSelectedServers[BrowserType] == -1)
			{
				// restore selection based on address only if no stored selection index for this filter
				m_AddressSelection |= ADDR_SELECTION_CHANGE;
			}
			else
			{
				m_AddressSelection |= ADDR_SELECTION_UPDATE_ADDRESS;
			}
		}
	}

	const bool CtrlPressed = Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL);

	// handle arrow hotkeys
	if(SelectedFilter > -1)
	{
		int SelectedIndex = m_lFilters[SelectedFilter].m_aSelectedServers[BrowserType];
		int NewFilter = SelectedFilter;

		if(m_DownArrowPressed)
		{
			if(!CtrlPressed)
			{
				ToBeSelectedServer = SelectedIndex + 1;
				if(ToBeSelectedServer >= m_lFilters[SelectedFilter].NumSortedServers())
					ToBeSelectedServer = m_lFilters[SelectedFilter].NumSortedServers() - 1;
			}
			else if(SelectedFilter + 1 < m_lFilters.size())
			{
				// move to next filter
				NewFilter = SelectedFilter + 1;
			}
		}
		else if(m_UpArrowPressed)
		{
			if(!CtrlPressed)
			{
				ToBeSelectedServer = SelectedIndex - 1;
				if(ToBeSelectedServer < 0)
					ToBeSelectedServer = 0;
			}
			else if(SelectedFilter - 1 >= 0)
			{
				// move to previous filter
				NewFilter = SelectedFilter - 1;
			}
		}

		if(NewFilter != SelectedFilter)
		{
			m_lFilters[NewFilter].Switch();
			m_lFilters[SelectedFilter].Switch();
			// try to restore selected server
			ToBeSelectedServer = clamp(m_lFilters[NewFilter].m_aSelectedServers[BrowserType], 0, m_lFilters[NewFilter].NumSortedServers()-1);
		}

		if(ToBeSelectedServer > -1 && ToBeSelectedServer < m_lFilters[NewFilter].NumSortedServers())
		{
			m_aSelectedFilters[BrowserType] = NewFilter;
			if(m_lFilters[NewFilter].m_aSelectedServers[BrowserType] != ToBeSelectedServer)
			{
				m_lFilters[NewFilter].m_aSelectedServers[BrowserType] = ToBeSelectedServer;
				m_ShowServerDetails = true;
				m_AddressSelection |= ADDR_SELECTION_REVEAL;
			}

			m_AddressSelection |= ADDR_SELECTION_UPDATE_ADDRESS;
		}
	}

	// display important messages in the middle of the screen so no user misses it
	{
		const char *pImportantMessage = NULL;
		if(m_ActivePage == PAGE_INTERNET && ServerBrowser()->IsRefreshingMasters())
			pImportantMessage = Localize("Refreshing master servers");
		else if(SelectedFilter == -1)
			pImportantMessage = Localize("No filter category is selected");
		else if(ServerBrowser()->IsRefreshing() && !NumServers)
			pImportantMessage = Localize("Fetching server info");
		else if(!ServerBrowser()->NumServers())
			pImportantMessage = Localize("No servers found");
		else if(ServerBrowser()->NumServers() && !NumServers)
			pImportantMessage = Localize("No servers match your filter criteria");

		if(pImportantMessage)
		{
			CUIRect MsgBox = View;
			MsgBox.y += View.h/3;
			UI()->DoLabel(&MsgBox, pImportantMessage, 16.0f, CUI::ALIGN_CENTER);
		}
	}

	// scrollbar
	static CScrollRegion s_ScrollRegion(this);
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ScrollParams.m_SliderMinHeight = 5;
	ScrollParams.m_ScrollSpeed = 10;
	View.w += ScrollParams.m_ScrollbarWidth;
	s_ScrollRegion.Begin(&View, &ScrollOffset, &ScrollParams);
	View.y += ScrollOffset.y;

	for(int FilterIndex = 0; FilterIndex < m_lFilters.size(); FilterIndex++)
	{
		CBrowserFilter *pFilter = &m_lFilters[FilterIndex];

		// filter header
		CUIRect Row;
		View.HSplitTop(20.0f, &Row, &View);
		s_ScrollRegion.AddRect(Row);

		// render header
		RenderFilterHeader(Row, FilterIndex);

		if(pFilter->Extended())
		{
			for (int ServerIndex = 0; ServerIndex < pFilter->NumSortedServers(); ServerIndex++)
			{
				const CServerInfo *pItem = pFilter->SortedGet(ServerIndex);

				// select server if address changed and match found
				bool IsSelected = m_aSelectedFilters[BrowserType] == FilterIndex && pFilter->m_aSelectedServers[BrowserType] == ServerIndex;
				if((m_AddressSelection&ADDR_SELECTION_CHANGE) && !str_comp(pItem->m_aAddress, g_Config.m_UiServerAddress))
				{
					if(!IsSelected)
					{
						m_ShowServerDetails = true;
						m_aSelectedFilters[BrowserType] = FilterIndex;
						pFilter->m_aSelectedServers[BrowserType] = ServerIndex;
						IsSelected = true;
					}
					m_AddressSelection &= ~(ADDR_SELECTION_CHANGE|ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND|ADDR_SELECTION_RESET_ADDRESS_IF_NOT_FOUND);
				}

				float ItemHeight = GetListHeaderHeight();
				if(!m_SidebarActive && IsSelected && m_ShowServerDetails)
				{
					ItemHeight *= 6.0f;
				}
				View.HSplitTop(ItemHeight, &Row, &View);

				s_ScrollRegion.AddRect(Row);
				if(IsSelected && (m_AddressSelection&ADDR_SELECTION_REVEAL)) // new selection (hotkeys or address input)
				{
					s_ScrollRegion.ScrollHere(CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
					m_AddressSelection &= ~ADDR_SELECTION_REVEAL;
				}

				// make sure that only those in view can be selected
				if(!s_ScrollRegion.IsRectClipped(Row))
				{
					if(IsSelected)
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

				if(int ReturnValue = DoBrowserEntry(pFilter->ID(ServerIndex), Row, pItem, pFilter, IsSelected))
				{
					m_ShowServerDetails = !m_ShowServerDetails || ReturnValue == 2 || pFilter->m_aSelectedServers[BrowserType] != ServerIndex; // click twice on line => fold server details
					m_aSelectedFilters[BrowserType] = FilterIndex;
					pFilter->m_aSelectedServers[BrowserType] = ServerIndex;
					m_AddressSelection &= ~(ADDR_SELECTION_CHANGE|ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND|ADDR_SELECTION_RESET_ADDRESS_IF_NOT_FOUND);
					if(g_Config.m_UiAutoswitchInfotab)
						m_SidebarTab = 0;
					UpdateServerBrowserAddress(); // update now instead of using flag because of connect
					if(Input()->MouseDoubleClick())
						Client()->Connect(g_Config.m_UiServerAddress);
				}
			}

			if(m_AddressSelection&ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND)
			{
				pFilter->m_aSelectedServers[BrowserType] = -1;
				m_AddressSelection &= ~ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND;
			}
			if(m_AddressSelection&ADDR_SELECTION_RESET_ADDRESS_IF_NOT_FOUND)
			{
				g_Config.m_UiServerAddress[0] = '\0';
				m_AddressSelection &= ~ADDR_SELECTION_RESET_ADDRESS_IF_NOT_FOUND;
			}
		}

		if(FilterIndex < m_lFilters.size()-1)
			View.HSplitTop(SpacingH, &Row, &View);

	}

	if(m_AddressSelection&ADDR_SELECTION_UPDATE_ADDRESS)
	{
		UpdateServerBrowserAddress();
		m_AddressSelection &= ~ADDR_SELECTION_UPDATE_ADDRESS;
	}
	str_copy(m_aLastServerAddress, g_Config.m_UiServerAddress, sizeof(m_aLastServerAddress));

	s_ScrollRegion.End();

	// bottom
	float SpacingW = 3.0f;
	float ButtonWidth = (Status.w/6.0f)-(SpacingW*5.0)/6.0f;
	float FontSize = ButtonHeight*ms_FontmodHeight*0.8f;

	// cut view
	CUIRect Left, Label, EditBox, Button;
	Status.VSplitLeft(ButtonWidth*3.5f+SpacingH*2.0f, &Left, &Status);

	// render quick search and host address
	Left.HSplitTop(((ButtonHeight*3.0f+SpacingH*2.0f)-(ButtonHeight*2.0f+SpacingH))/2.0f, 0, &Left);
	Left.HSplitTop(ButtonHeight, &Label, &Left);
	Label.VSplitLeft(2.0f, 0, &Label);
	Label.VSplitRight(ButtonWidth*2.0f+SpacingH, &Label, &EditBox);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Search:"), FontSize, CUI::ALIGN_LEFT);
	EditBox.VSplitRight(EditBox.h, &EditBox, &Button);
	static float s_ClearOffset = 0.0f;
	if(DoEditBox(&g_Config.m_BrFilterString, &EditBox, g_Config.m_BrFilterString, sizeof(g_Config.m_BrFilterString), FontSize, &s_ClearOffset, false, CUI::CORNER_ALL))
	{
		Client()->ServerBrowserUpdate();
		ServerBrowserFilterOnUpdate();
	}

	// clear button
	{
		static CButtonContainer s_ClearButton;
		if(DoButton_SpriteID(&s_ClearButton, IMAGE_TOOLICONS, SPRITE_TOOL_X_A, false, &Button, CUI::CORNER_ALL, 5.0f, true))
		{
			g_Config.m_BrFilterString[0] = 0;
			UI()->SetActiveItem(&g_Config.m_BrFilterString);
			Client()->ServerBrowserUpdate();
		}
	}

	Left.HSplitTop(SpacingH, 0, &Left);
	Left.HSplitTop(ButtonHeight, &Label, 0);
	Label.VSplitLeft(2.0f, 0, &Label);
	Label.VSplitRight(ButtonWidth*2.0f+SpacingH, &Label, &EditBox);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Host address:"), FontSize, CUI::ALIGN_LEFT);
	static float s_AddressOffset = 0.0f;
	if(DoEditBox(&g_Config.m_UiServerAddress, &EditBox, g_Config.m_UiServerAddress, sizeof(g_Config.m_UiServerAddress), FontSize, &s_AddressOffset, false, CUI::CORNER_ALL))
	{
		m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND | ADDR_SELECTION_REVEAL;
	}

	// render status
	if(ServerBrowser()->IsRefreshing() && m_ActivePage != PAGE_LAN)
	{
		char aBuf[128];
		Status.HSplitTop(ButtonHeight + SpacingH, 0, &Status);
		str_format(aBuf, sizeof(aBuf), Localize("%d%% loaded"), ServerBrowser()->LoadingProgression());
		Status.y += 2.0f;
		UI()->DoLabel(&Status, aBuf, 14.0f, CUI::ALIGN_CENTER);
	}
	else
	{
		// todo: move this to a helper function
		static float RenderOffset = 0.0f;
		if(RenderOffset == 0.0f)
		{
			char aChar[2] = "0";
			RenderOffset = TextRender()->TextWidth(0, 12.0f, aChar, -1, -1.0f);
		}

		float OffsetServer = 0.0f, OffsetPlayer = 0.0f;
		int Num = ServerBrowser()->NumServers();
		if(Num < 1000)
			OffsetServer += RenderOffset;
		if(Num < 100)
			OffsetServer += RenderOffset;
		if(Num < 10)
			OffsetServer += RenderOffset;
		int NumPlayers = ServerBrowser()->NumClients();;
		if(NumPlayers < 1000)
			OffsetPlayer += RenderOffset;
		if(NumPlayers < 100)
			OffsetPlayer += RenderOffset;
		if(NumPlayers < 10)
			OffsetPlayer += RenderOffset;
		char aBuf[128];
		Status.VSplitLeft(20.0f, 0, &Status);
		Status.HSplitTop(ButtonHeight/1.5f, 0, &Status);
		Status.HSplitTop(ButtonHeight, &Label, &Status);
		str_format(aBuf, sizeof(aBuf), Localize("%d servers"), ServerBrowser()->NumServers());
		Label.y += 2.0f;
		Label.x += OffsetServer;
		UI()->DoLabel(&Label, aBuf, 14.0f, CUI::ALIGN_LEFT);
		Status.HSplitTop(SpacingH, 0, &Status);
		Status.HSplitTop(ButtonHeight, &Label, 0);
		str_format(aBuf, sizeof(aBuf), Localize("%d players"), NumPlayers);
		Label.y += 2.0f;
		Label.x += OffsetPlayer;
		UI()->DoLabel(&Label, aBuf, 14.0f, CUI::ALIGN_LEFT);
	}
}

void CMenus::RenderServerbrowserSidebar(CUIRect View)
{
	CUIRect Header, Button;

	// background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), CUI::CORNER_ALL, 5.0f);

	// handle Tab key
	if(m_TabPressed)
	{
		if(Input()->KeyIsPressed(KEY_LSHIFT) || Input()->KeyIsPressed(KEY_RSHIFT))
		{
			m_SidebarTab--;
			if(m_SidebarTab < 0) m_SidebarTab = 2;
		}
		else
			m_SidebarTab = (m_SidebarTab+1)%3;
	}

	// header
	View.HSplitTop(GetListHeaderHeight(), &Header, &View);
	float Width = Header.w;
	Header.VSplitLeft(Width*0.30f, &Button, &Header);
	static CButtonContainer s_TabInfo;
	if(DoButton_SpriteID(&s_TabInfo, IMAGE_SIDEBARICONS, m_SidebarTab!=0?SPRITE_SIDEBAR_INFO_A: SPRITE_SIDEBAR_INFO_B, m_SidebarTab==0 , &Button, CUI::CORNER_TL, 5.0f, true))
	{
		m_SidebarTab = 0;
	}
	Header.VSplitLeft(Width*0.30f, &Button, &Header);
	static CButtonContainer s_TabFilter;
	if(DoButton_SpriteID(&s_TabFilter, IMAGE_SIDEBARICONS, m_SidebarTab!=1?SPRITE_SIDEBAR_FILTER_A: SPRITE_SIDEBAR_FILTER_B, m_SidebarTab==1, &Button, 0, 0.0f, true))
	{
		m_SidebarTab = 1;
	}
	static CButtonContainer s_TabFriends;
	if(DoButton_SpriteID(&s_TabFriends, IMAGE_SIDEBARICONS, m_SidebarTab!=2?SPRITE_SIDEBAR_FRIEND_A:SPRITE_SIDEBAR_FRIEND_B, m_SidebarTab == 2, &Header, CUI::CORNER_TR, 5.0f, true))
	{
		m_SidebarTab = 2;
	}

	// tabs
	switch(m_SidebarTab)
	{
	case 0:
		RenderServerbrowserInfoTab(View);
		break;
	case 1:
		RenderServerbrowserFilterTab(View);
		break;
	case 2:
		RenderServerbrowserFriendTab(View);
	}
}

void CMenus::RenderServerbrowserFriendTab(CUIRect View)
{
	CUIRect Button, Icon, Label, Rect;
	CUIRect BottomArea;
	const float FontSize = 10.0f;
	static bool s_ListExtended[NUM_FRIEND_TYPES] = { 1, 1, 0 };

	View.HSplitBottom(3*GetListHeaderHeight(), &View, &BottomArea);

	// calculate friends
	// todo: optimize this
	m_pDeleteFriend = 0;
	m_lFriendList[0].clear();
	m_lFriendList[1].clear();
	m_lFriendList[2].clear();
	for(int f = 0; f < m_pClient->Friends()->NumFriends(); ++f)
	{
		const CContactInfo *pFriendInfo = m_pClient->Friends()->GetFriend(f);
		CFriendItem FriendItem;
		FriendItem.m_pServerInfo = 0;
		str_copy(FriendItem.m_aName, pFriendInfo->m_aName, sizeof(FriendItem.m_aName));
		str_copy(FriendItem.m_aClan, pFriendInfo->m_aClan, sizeof(FriendItem.m_aClan));
		FriendItem.m_FriendState = pFriendInfo->m_aName[0] ? CContactInfo::CONTACT_PLAYER : CContactInfo::CONTACT_CLAN;
		FriendItem.m_IsPlayer = false;
		m_lFriendList[2].add(FriendItem);
	}

	for(int ServerIndex = 0; ServerIndex < ServerBrowser()->NumServers(); ++ServerIndex)
	{
		const CServerInfo *pEntry = ServerBrowser()->Get(ServerIndex);
		if(pEntry->m_FriendState == CContactInfo::CONTACT_NO)
			continue;
				
		for(int j = 0; j < pEntry->m_NumClients; ++j)
		{
			if(pEntry->m_aClients[j].m_FriendState == CContactInfo::CONTACT_NO)
				continue;
			
			CFriendItem FriendItem;
			FriendItem.m_pServerInfo = pEntry;
			str_copy(FriendItem.m_aName, pEntry->m_aClients[j].m_aName, sizeof(FriendItem.m_aName));
			str_copy(FriendItem.m_aClan, pEntry->m_aClients[j].m_aClan, sizeof(FriendItem.m_aClan));
			FriendItem.m_FriendState = pEntry->m_aClients[j].m_FriendState;
			FriendItem.m_IsPlayer = !(pEntry->m_aClients[j].m_PlayerType&CServerInfo::CClient::PLAYERFLAG_SPEC);

			if(pEntry->m_aClients[j].m_FriendState == CContactInfo::CONTACT_PLAYER)
				m_lFriendList[0].add(FriendItem);
			else
				m_lFriendList[1].add(FriendItem);

			for(int f = 0; f < m_lFriendList[2].size(); ++f)
			{
				if((!m_lFriendList[2][f].m_aName[0] || !str_comp(m_lFriendList[2][f].m_aName, pEntry->m_aClients[j].m_aName)) && !str_comp(m_lFriendList[2][f].m_aClan, pEntry->m_aClients[j].m_aClan))
					m_lFriendList[2].remove_index(f--);
			}
		}
	}

	// scrollbar
	static CScrollRegion s_ScrollRegion(this);
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarWidth = 14;
	ScrollParams.m_ScrollbarMargin = 5;
	ScrollParams.m_ScrollSpeed = 15;
	s_ScrollRegion.Begin(&View, &ScrollOffset, &ScrollParams);
	View.y += ScrollOffset.y;

	// show lists
	// only ~10 buttons will be displayed at once, a sliding window of 20 buttons ought to be enough
	static CButtonContainer s_FriendJoinButtons[20];
	int ButtonId = 0;
	for(int i = 0; i < NUM_FRIEND_TYPES; ++i)
	{
		CUIRect Header;
		char aBuf[64] = { 0 };
		View.HSplitTop(GetListHeaderHeight(), &Header, &View);
		if(s_ListExtended[i])
		{
			// entries
			for(int f = 0; f < m_lFriendList[i].size(); ++f, ++ButtonId)
			{
				if(i == FRIEND_OFF)
					View.HSplitTop(8.0f + GetListHeaderHeight(), &Rect, &View);
				else
					View.HSplitTop(20.0f + GetListHeaderHeight(), &Rect, &View);
				s_ScrollRegion.AddRect(Rect);
				if(i == FRIEND_PLAYER_ON)
					RenderTools()->DrawUIRect(&Rect, vec4(0.5f, 1.0f, 0.5f, 0.30f), CUI::CORNER_ALL, 5.0f);
				else if(i == FRIEND_CLAN_ON)
					RenderTools()->DrawUIRect(&Rect, vec4(0.5f, 0.5f, 1.5f, 0.30f), CUI::CORNER_ALL, 5.0f);
				else
					RenderTools()->DrawUIRect(&Rect, vec4(1.0f, 0.5f, 0.5f, 0.30f), CUI::CORNER_ALL, 5.0f);
				Rect.VMargin(2.0f, &Rect);
				Rect.HMargin(2.0f, &Rect);
				Rect.VSplitRight(45.0f, &Rect, &Icon);
				Rect.HSplitTop(20.0f, &Button, 0);
				// name
				Rect.HSplitTop(10.0f, &Button, &Rect);
				Button.VSplitLeft(2.0f, 0, &Button);
				UI()->DoLabel(&Button, m_lFriendList[i][f].m_aName, FontSize - 2, CUI::ALIGN_LEFT);
				// clan
				Rect.HSplitTop(10.0f, &Button, &Rect);
				Button.VSplitLeft(2.0f, 0, &Button);
				UI()->DoLabel(&Button, m_lFriendList[i][f].m_aClan, FontSize - 2, CUI::ALIGN_LEFT);
				// info
				if(m_lFriendList[i][f].m_pServerInfo)
				{
					Rect.HSplitTop(GetListHeaderHeight(), &Button, &Rect);
					Button.VSplitLeft(2.0f, 0, &Button);
					if(m_lFriendList[i][f].m_IsPlayer)
						str_format(aBuf, sizeof(aBuf), Localize("Playing '%s' on '%s'", "Playing '(gametype)' on '(map)'"), m_lFriendList[i][f].m_pServerInfo->m_aGameType, m_lFriendList[i][f].m_pServerInfo->m_aMap);
					else
						str_format(aBuf, sizeof(aBuf), Localize("Watching '%s' on '%s'", "Watching '(gametype)' on '(map)'"), m_lFriendList[i][f].m_pServerInfo->m_aGameType, m_lFriendList[i][f].m_pServerInfo->m_aMap);
					Button.HMargin(2.0f, &Button);
					UI()->DoLabel(&Button, aBuf, FontSize - 2, CUI::ALIGN_LEFT);
				}
				// delete button
				Icon.HSplitTop(14.0f, &Rect, 0);
				Rect.VSplitRight(12.0f, 0, &Icon);
				Icon.HMargin((Icon.h - Icon.w) / 2, &Icon);
				if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_X_B, &Icon))
				{
					m_pDeleteFriend = &m_lFriendList[i][f];
				}
				// join button
				Rect.VSplitRight(15.0f, &Button, 0);
				if(m_lFriendList[i][f].m_pServerInfo)
				{
					Button.Margin((Button.h - GetListHeaderHeight() + 2.0f) / 2, &Button);
					if(DoButton_Menu(&(s_FriendJoinButtons[ButtonId%20]), Localize("Join", "Join a server"), 0, &Button) )
					{
						str_copy(g_Config.m_UiServerAddress, m_lFriendList[i][f].m_pServerInfo->m_aAddress, sizeof(g_Config.m_UiServerAddress));
						Client()->Connect(g_Config.m_UiServerAddress);
					}
				}
				if(f < m_lFriendList[i].size()-1)
					View.HSplitTop(2.0f, 0, &View);
			}
		}
		View.HSplitTop(2.0f, 0, &View);
		
		// header
		RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		Header.VSplitLeft(Header.h, &Icon, &Label);
		vec4 Color = UI()->MouseInside(&Header) ? vec4(1.0f, 1.0f, 1.0f, 1.0f) : vec4(0.6f, 0.6f, 0.6f, 1.0f);
		DoIconColor(IMAGE_MENUICONS, s_ListExtended[i] ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED, &Icon, Color);
		int ListSize = m_lFriendList[i].size();
		switch(i)
		{
		case 0: str_format(aBuf, sizeof(aBuf), Localize("Online players (%d)"), ListSize); break;
		case 1: str_format(aBuf, sizeof(aBuf), Localize("Online clanmates (%d)"), ListSize); break;
		case 2: str_format(aBuf, sizeof(aBuf), Localize("Offline (%d)", "friends (server browser)"), ListSize); break;
		}
		Label.HMargin(2.0f, &Label);
		UI()->DoLabel(&Label, aBuf, FontSize, CUI::ALIGN_LEFT);
		static int s_HeaderButton[NUM_FRIEND_TYPES] = { 0 };
		if(UI()->DoButtonLogic(&s_HeaderButton[i], "", 0, &Header))
		{
			s_ListExtended[i] ^= 1;
		}
	}
	s_ScrollRegion.End();

	// add friend
	BottomArea.HSplitTop(GetListHeaderHeight(), &Button, &BottomArea);
	Button.VSplitLeft(50.0f, &Label, &Button);
	UI()->DoLabel(&Label, Localize("Name"), FontSize, CUI::ALIGN_LEFT);
	static char s_aName[MAX_NAME_LENGTH] = { 0 };
	static float s_OffsetName = 0.0f;
	DoEditBox(&s_aName, &Button, s_aName, sizeof(s_aName), Button.h*ms_FontmodHeight*0.8f, &s_OffsetName);

	BottomArea.HSplitTop(GetListHeaderHeight(), &Button, &BottomArea);
	Button.VSplitLeft(50.0f, &Label, &Button);
	UI()->DoLabel(&Label, Localize("Clan"), FontSize, CUI::ALIGN_LEFT);
	static char s_aClan[MAX_CLAN_LENGTH] = { 0 };
	static float s_OffsetClan = 0.0f;
	DoEditBox(&s_aClan, &Button, s_aClan, sizeof(s_aClan), Button.h*ms_FontmodHeight*0.8f, &s_OffsetClan);

	BottomArea.HSplitTop(GetListHeaderHeight(), &Button, &BottomArea);
	RenderTools()->DrawUIRect(&Button, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	if(s_aName[0] || s_aClan[0])
		Button.VSplitLeft(Button.h, &Icon, &Label);
	else
		Label = Button;
	Label.HMargin(2.0f, &Label);
	const char *pButtonText = (!s_aName[0] && !s_aClan[0]) ? Localize("Add friend/clan") : s_aName[0] ? Localize("Add friend") : Localize("Add clan");
	UI()->DoLabel(&Label, pButtonText, FontSize, CUI::ALIGN_CENTER);
	if(s_aName[0] || s_aClan[0])
		DoIcon(IMAGE_FRIENDICONS, UI()->MouseInside(&Button)?SPRITE_FRIEND_PLUS_A:SPRITE_FRIEND_PLUS_B, &Icon);
	static CButtonContainer s_AddFriend;
	if((s_aName[0] || s_aClan[0]) && UI()->DoButtonLogic(&s_AddFriend, "", 0, &Button))
	{
		m_pClient->Friends()->AddFriend(s_aName, s_aClan);
		FriendlistOnUpdate();
		Client()->ServerBrowserUpdate();
		s_aName[0] = 0;
		s_aClan[0] = 0;
	}

	// delete friend
	if(m_pDeleteFriend)
	{
		m_Popup = POPUP_REMOVE_FRIEND;
	}
}

void CMenus::RenderServerbrowserFilterTab(CUIRect View)
{
	CUIRect ServerFilter = View, FilterHeader, Button, Icon, Label;
	const float FontSize = 10.0f;
	const float LineSize = 16.0f;

	// new filter
	ServerFilter.HSplitBottom(LineSize, &ServerFilter, &Button);
	Button.VSplitLeft(60.0f, &Icon, &Button);
	static char s_aFilterName[32] = { 0 };
	static float s_FilterOffset = 0.0f;
	static int s_EditFilter = 0;
	DoEditBox(&s_EditFilter, &Icon, s_aFilterName, sizeof(s_aFilterName), FontSize, &s_FilterOffset, false, CUI::CORNER_L);
	RenderTools()->DrawUIRect(&Button, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_R, 5.0f);
	Button.VSplitLeft(Button.h, &Icon, &Label);
	Label.HMargin(2.0f, &Label);
	UI()->DoLabel(&Label, Localize("New filter"), FontSize, CUI::ALIGN_LEFT);
	if(s_aFilterName[0])
		DoIcon(IMAGE_FRIENDICONS, UI()->MouseInside(&Button) ? SPRITE_FRIEND_PLUS_A : SPRITE_FRIEND_PLUS_B, &Icon);
	static CButtonContainer s_AddFilter;
	if(s_aFilterName[0] && UI()->DoButtonLogic(&s_AddFilter, "", 0, &Button))
	{
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_CUSTOM, s_aFilterName, ServerBrowser()));
		s_aFilterName[0] = 0;
	}

	CBrowserFilter *pFilter = GetSelectedBrowserFilter();
	if(!pFilter)
		return;

	CServerFilterInfo FilterInfo;
	pFilter->GetFilter(&FilterInfo);

	// server filter
	ServerFilter.HSplitTop(GetListHeaderHeight(), &FilterHeader, &ServerFilter);
	RenderTools()->DrawUIRect(&FilterHeader, vec4(1, 1, 1, 0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerFilter, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	FilterHeader.HMargin(2.0f, &FilterHeader);
	UI()->DoLabel(&FilterHeader, Localize("Server filter"), FontSize + 2.0f, CUI::ALIGN_CENTER);

	int NewSortHash = FilterInfo.m_SortHash;
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterEmpty = 0;
	if(DoButton_CheckBox(&s_BrFilterEmpty, Localize("Has people playing"), FilterInfo.m_SortHash&IServerBrowser::FILTER_EMPTY, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_EMPTY;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterSpectators = 0;
	if(DoButton_CheckBox(&s_BrFilterSpectators, Localize("Count players only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_SPECTATORS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterFull = 0;
	if(DoButton_CheckBox(&s_BrFilterFull, Localize("Server not full"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FULL, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_FULL;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterFriends = 0;
	if(DoButton_CheckBox(&s_BrFilterFriends, Localize("Show friends only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FRIENDS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_FRIENDS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterBots = 0;
	if(DoButton_CheckBox(&s_BrFilterBots, Localize("Hide bots"), FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_BOTS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPw = 0;
	if(DoButton_CheckBox(&s_BrFilterPw, Localize("No password"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PW, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PW;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterCompatversion = 0;
	if(DoButton_CheckBox(&s_BrFilterCompatversion, Localize("Compatible version"), FilterInfo.m_SortHash&IServerBrowser::FILTER_COMPAT_VERSION, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_COMPAT_VERSION;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	const bool Locked = pFilter->Custom() == CBrowserFilter::FILTER_STANDARD;
	static int s_BrFilterPure = 0;
	if(DoButton_CheckBox(&s_BrFilterPure, Localize("Standard gametype"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE, &Button, Locked))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PURE;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPureMap = 0;
	if(DoButton_CheckBox(&s_BrFilterPureMap, Localize("Standard map"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE_MAP, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PURE_MAP;

	bool UpdateFilter = false;
	if(FilterInfo.m_SortHash != NewSortHash)
	{
		FilterInfo.m_SortHash = NewSortHash;
		UpdateFilter = true;
	}

	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	UI()->DoLabel(&Button, Localize("Game types:"), FontSize, CUI::ALIGN_LEFT);
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	RenderTools()->DrawUIRect(&Button, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 2.0f);
	Button.HMargin(2.0f, &Button);
	UI()->ClipEnable(&Button);

	float Length = 0.0f;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
	{
		if(FilterInfo.m_aGametype[i][0])
		{
			Length += TextRender()->TextWidth(0, FontSize, FilterInfo.m_aGametype[i], -1, -1.0f) + 14.0f;
		}
	}
	static float s_ScrollValue = 0.0f;
	bool NeedScrollbar = (Button.w - Length) < 0.0f;
	Button.x += min(0.0f, Button.w - Length) * s_ScrollValue;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
	{
		if(FilterInfo.m_aGametype[i][0])
		{
			float CurLength = TextRender()->TextWidth(0, FontSize, FilterInfo.m_aGametype[i], -1, -1.0f) + 12.0f;
			Button.VSplitLeft(CurLength, &Icon, &Button);
			RenderTools()->DrawUIRect(&Icon, vec4(0.75, 0.75, 0.75, 0.25f), CUI::CORNER_ALL, 3.0f);
			UI()->DoLabel(&Icon, FilterInfo.m_aGametype[i], FontSize, CUI::ALIGN_LEFT);
			Icon.VSplitRight(10.0f, 0, &Icon);
			if(DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_X_B, &Icon))
			{
				// remove gametype entry
				if((i == CServerFilterInfo::MAX_GAMETYPES - 1) || !FilterInfo.m_aGametype[i + 1][0])
					FilterInfo.m_aGametype[i][0] = 0;
				else
				{
					int j = i;
					for(; j < CServerFilterInfo::MAX_GAMETYPES - 1 && FilterInfo.m_aGametype[j + 1][0]; ++j)
						str_copy(FilterInfo.m_aGametype[j], FilterInfo.m_aGametype[j + 1], sizeof(FilterInfo.m_aGametype[j]));
					FilterInfo.m_aGametype[j][0] = 0;
				}
				UpdateFilter = true;
			}
			Button.VSplitLeft(2.0f, 0, &Button);
		}
	}

	UI()->ClipDisable();

	if(NeedScrollbar)
	{
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		s_ScrollValue = DoScrollbarH(&s_ScrollValue, &Button, s_ScrollValue);
	}
	else
		ServerFilter.HSplitTop(4.f, &Button, &ServerFilter); // Leave some space in between edit boxes
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);

	Button.VSplitLeft(60.0f, &Button, &Icon);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static char s_aGametype[16] = { 0 };
	static float s_Offset = 0.0f;
	static int s_EditGametype = 0;
	Button.VSplitRight(Button.h, &Label, &Button);
	DoEditBox(&s_EditGametype, &Label, s_aGametype, sizeof(s_aGametype), FontSize, &s_Offset);
	RenderTools()->DrawUIRect(&Button, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_R, 5.0f);
	DoIcon(IMAGE_FRIENDICONS, UI()->MouseInside(&Button) ? SPRITE_FRIEND_PLUS_A : SPRITE_FRIEND_PLUS_B, &Button);
	static CButtonContainer s_AddGametype;
	if(s_aGametype[0] && UI()->DoButtonLogic(&s_AddGametype, "", 0, &Button))
	{
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			if(!FilterInfo.m_aGametype[i][0])
			{
				str_copy(FilterInfo.m_aGametype[i], s_aGametype, sizeof(FilterInfo.m_aGametype[i]));
				UpdateFilter = true;
				s_aGametype[0] = 0;
				break;
			}
		}
	}
	Icon.VSplitLeft(10.0f, 0, &Icon);
	Icon.VSplitLeft(40.0f, &Button, 0);
	static CButtonContainer s_ClearGametypes;
	if(DoButton_MenuTabTop(&s_ClearGametypes, Localize("Clear", "clear gametype filters"), false, &Button))
	{
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			FilterInfo.m_aGametype[i][0] = 0;
		}
		UpdateFilter = true;
	}

	if(!NeedScrollbar)
		ServerFilter.HSplitTop(LineSize - 4.f, &Button, &ServerFilter);

	{
		int Value = FilterInfo.m_Ping, Min = 20, Max = 999;

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s %d", Localize("Maximum ping:"), Value);
		UI()->DoLabel(&Button, aBuf, FontSize, CUI::ALIGN_LEFT);

		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);

		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		Button.VMargin(4.0f, &Button);
		static int s_BrFilterPing = 0;
		Value = LogarithmicScrollbarScale.ToAbsolute(DoScrollbarH(&s_BrFilterPing, &Button, LogarithmicScrollbarScale.ToRelative(Value, Min, Max)), Min, Max);
		if(Value != FilterInfo.m_Ping) {
			FilterInfo.m_Ping = Value;
			UpdateFilter = true;
		}
	}

	// server address
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	UI()->DoLabel(&Button, Localize("Server address:"), FontSize, CUI::ALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	static float OffsetAddr = 0.0f;
	static int s_BrFilterServerAddress = 0;
	if(DoEditBox(&s_BrFilterServerAddress, &Button, FilterInfo.m_aAddress, sizeof(FilterInfo.m_aAddress), FontSize, &OffsetAddr))
		UpdateFilter = true;

	// player country
	{
		CUIRect Rect;
		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		UI()->DoLabel(&Button, Localize("Country:"), FontSize, CUI::ALIGN_LEFT);
		Button.VSplitRight(60.0f, 0, &Rect);
		Rect.VSplitLeft(16.0f, &Button, &Rect);
		static int s_BrFilterCountry = 0;
		if(DoButton_CheckBox(&s_BrFilterCountry, "", FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY, &Button))
		{
			FilterInfo.m_SortHash ^= IServerBrowser::FILTER_COUNTRY;
			UpdateFilter = true;
		}
		Rect.w = Rect.h * 2;
		vec4 Color(1.0f, 1.0f, 1.0f, FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY ? 1.0f : 0.5f);
		m_pClient->m_pCountryFlags->Render(FilterInfo.m_Country, &Color, Rect.x, Rect.y, Rect.w, Rect.h);

		static int s_BrFilterCountryIndex = 0;
		if((FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY) && UI()->DoButtonLogic(&s_BrFilterCountryIndex, "", 0, &Rect))
			m_Popup = POPUP_COUNTRY;
	}

	// level
	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(LineSize + 2, &Button, &ServerFilter);
	UI()->DoLabel(&Button, Localize("Difficulty:"), FontSize, CUI::ALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	Button.y -= 2.0f;
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton1;
	if(DoButton_SpriteID(&s_LevelButton1, IMAGE_LEVELICONS, FilterInfo.IsLevelFiltered(CServerInfo::LEVEL_CASUAL) ? SPRITE_LEVEL_A_B : SPRITE_LEVEL_A_ON, false, &Icon, CUI::CORNER_L, 5.0f, true))
	{
		FilterInfo.ToggleLevel(CServerInfo::LEVEL_CASUAL);
		UpdateFilter = true;
	}
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton2;
	if(DoButton_SpriteID(&s_LevelButton2, IMAGE_LEVELICONS, FilterInfo.IsLevelFiltered(CServerInfo::LEVEL_NORMAL) ? SPRITE_LEVEL_B_B : SPRITE_LEVEL_B_ON, false, &Icon, 0, 5.0f, true))
	{
		FilterInfo.ToggleLevel(CServerInfo::LEVEL_NORMAL);
		UpdateFilter = true;
	}
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton3;
	if(DoButton_SpriteID(&s_LevelButton3, IMAGE_LEVELICONS, FilterInfo.IsLevelFiltered(CServerInfo::LEVEL_COMPETITIVE) ? SPRITE_LEVEL_C_B : SPRITE_LEVEL_C_ON, false, &Icon, CUI::CORNER_R, 5.0f, true))
	{
		FilterInfo.ToggleLevel(CServerInfo::LEVEL_COMPETITIVE);
		UpdateFilter = true;
	}

	if(UpdateFilter)
	{
		pFilter->SetFilter(&FilterInfo);
	}

	// reset filter
	ServerFilter.HSplitBottom(LineSize, &ServerFilter, 0);
	ServerFilter.HSplitBottom(LineSize, &ServerFilter, &Button);
	Button.VMargin((Button.w - 80.0f) / 2, &Button);
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset filter"), 0, &Button))
	{
		pFilter->Reset();
		UpdateFilter = true;
	}

	if(UpdateFilter)
	{
		ServerBrowserFilterOnUpdate();
	}
}

void CMenus::RenderServerbrowserInfoTab(CUIRect View)
{
	CBrowserFilter *pFilter = GetSelectedBrowserFilter();
	if(pFilter)
	{
		const CServerInfo *pItem = pFilter->SortedGet(pFilter->m_aSelectedServers[ServerBrowser()->GetType()]);
		RenderServerbrowserServerDetail(View, pItem);
	}
}

void CMenus::RenderDetailInfo(CUIRect View, const CServerInfo *pInfo)
{
	CUIRect ServerHeader;
	const float FontSize = 10.0f;
	View.HSplitTop(GetListHeaderHeight(), &ServerHeader, &View);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1, 1, 1, 0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&View, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	ServerHeader.HMargin(2.0f, &ServerHeader);
	UI()->DoLabel(&ServerHeader, Localize("Server details"), FontSize + 2.0f, CUI::ALIGN_CENTER);

	if(pInfo)
	{
		CUIRect Row;
		// Localize("Map:"); Localize("Game type:"); Localize("Version:"); Localize("Difficulty:"); Localize("Casual", "Server difficulty"); Localize("Normal", "Server difficulty"); Localize("Competitive", "Server difficulty");
		static CLocConstString s_aLabels[] = {
			"Map:",		
			"Game type:",
			"Version:",
			"Difficulty:" };
		static CLocConstString s_aDifficulty[] = {
			"Casual",
			"Normal",
			"Competitive" };

		CUIRect LeftColumn, RightColumn;
		View.VMargin(2.0f, &View);
		View.VSplitLeft(70.0f, &LeftColumn, &RightColumn);

		for(unsigned int i = 0; i < sizeof(s_aLabels) / sizeof(s_aLabels[0]); i++)
		{
			LeftColumn.HSplitTop(15.0f, &Row, &LeftColumn);
			UI()->DoLabel(&Row, s_aLabels[i], FontSize, CUI::ALIGN_LEFT, Row.w, false);
		}

		// map
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabel(&Row, pInfo->m_aMap, FontSize, CUI::ALIGN_LEFT, Row.w, false);

		// game type
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		CUIRect Icon;
		Row.VSplitLeft(Row.h, &Icon, &Row);
		Icon.y -= 2.0f;
		DoGameIcon(pInfo->m_aGameType, &Icon);
		UI()->DoLabel(&Row, pInfo->m_aGameType, FontSize, CUI::ALIGN_LEFT, Row.w, false);

		// version
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		UI()->DoLabel(&Row, pInfo->m_aVersion, FontSize, CUI::ALIGN_LEFT, Row.w, false);

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
		UI()->DoLabel(&Row, s_aDifficulty[pInfo->m_ServerLevel], FontSize, CUI::ALIGN_LEFT, Row.w, false);
	}
}

void CMenus::RenderDetailScoreboard(CUIRect View, const CServerInfo *pInfo, int RowCount, vec4 TextColor)
{
	CBrowserFilter *pFilter = GetSelectedBrowserFilter();
	CServerFilterInfo FilterInfo;
	if(pFilter)
		pFilter->GetFilter(&FilterInfo);

	TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);

	// server scoreboard
	CTextCursor Cursor;
	const float FontSize = 10.0f;
	int ActColumn = 0;
	RenderTools()->DrawUIRect(&View, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	View.Margin(2.0f, &View);
	
	if(pInfo)
	{
		int Count = 0;

		CUIRect Scroll;

		float RowWidth = (RowCount == 0) ? View.w : (View.w * 0.25f);
		float LineHeight = 20.0f;

		static CScrollRegion s_ScrollRegion(this);
		vec2 ScrollOffset(0, 0);
		CScrollRegionParams ScrollParams;
		ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
		ScrollParams.m_ScrollbarBgColor = vec4(0,0,0,0);
		ScrollParams.m_ScrollbarWidth = 5;
		ScrollParams.m_ScrollbarMargin = 1;
		ScrollParams.m_ScrollSpeed = 15;
		s_ScrollRegion.Begin(&View, &ScrollOffset, &ScrollParams);
		View.y += ScrollOffset.y;
		if(RowCount != 0)
		{
			float Width = RowWidth * ((pInfo->m_NumClients+RowCount-1) / RowCount);
			static float s_ScrollValue = 0.0f;
			if(Width > View.w)
			{
				View.HSplitBottom(8.0f, &View, &Scroll);
				Scroll.VMargin(5.0f, &Scroll);
				s_ScrollValue = DoScrollbarH(&s_ScrollValue, &Scroll, s_ScrollValue);
				View.x += (View.w - Width) * s_ScrollValue;
				LineHeight = 0.25f*View.h;
			}
		}
		
		CUIRect Row = View;

		for(int i = 0; i < pInfo->m_NumClients; i++)
		{
			if(pFilter && (FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS) && (pInfo->m_aClients[i].m_PlayerType&CServerInfo::CClient::PLAYERFLAG_BOT))
				continue;

			CUIRect Name, Clan, Score, Flag, Icon;

			if(RowCount > 0 && Count % RowCount == 0)
			{
				View.VSplitLeft(RowWidth, &Row, &View);
				ActColumn++;
			}
	
			Row.HSplitTop(LineHeight, &Name, &Row);
			s_ScrollRegion.AddRect(Name);
			RenderTools()->DrawUIRect(&Name, vec4(1.0f, 1.0f, 1.0f, (Count % 2 + 1)*0.05f), CUI::CORNER_ALL, 4.0f);

			// friend
			if(UI()->DoButtonLogic(&pInfo->m_aClients[i], "", 0, &Name))
			{
				if(pInfo->m_aClients[i].m_FriendState == CContactInfo::CONTACT_PLAYER)
					m_pClient->Friends()->RemoveFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
				else
					m_pClient->Friends()->AddFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
				FriendlistOnUpdate();
				Client()->ServerBrowserUpdate();
			}
			Name.VSplitLeft(Name.h-8.0f, &Icon, &Name);
			Icon.HMargin(4.0f, &Icon);
			if(pInfo->m_aClients[i].m_FriendState != CContactInfo::CONTACT_NO)
				DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART_A, &Icon);

			Name.VSplitLeft(2.0f, 0, &Name);
			Name.VSplitLeft(25.0f, &Score, &Name);
			Name.VSplitRight(2*(Name.h-8.0f), &Name, &Flag);
			Flag.HMargin(4.0f, &Flag);
			Name.HSplitTop(LineHeight*0.5f, &Name, &Clan);

			// score
			if(!(pInfo->m_aClients[i].m_PlayerType&CServerInfo::CClient::PLAYERFLAG_SPEC))
			{
				char aTemp[16];
				FormatScore(aTemp, sizeof(aTemp), pInfo->m_Flags&IServerBrowser::FLAG_TIMESCORE, &pInfo->m_aClients[i]);
				TextRender()->SetCursor(&Cursor, Score.x, Score.y + (Score.h - FontSize-2) / 4.0f, FontSize-2, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Score.w;
				TextRender()->TextEx(&Cursor, aTemp, -1);
			}

			// name
			TextRender()->SetCursor(&Cursor, Name.x, Name.y, FontSize - 2, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Name.w;
			const char *pName = pInfo->m_aClients[i].m_aName;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the part that matches
				const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pName, (int)(s - pName));
					TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextColor.a);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);
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
				// highlight the part that matches
				const char *s = str_find_nocase(pClan, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pClan, (int)(s - pClan));
					TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextColor.a);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);
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

			++Count;
		}
		s_ScrollRegion.End();
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
	ServerScoreboard.HSplitTop(GetListHeaderHeight(), &ServerHeader, &ServerScoreboard);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1, 1, 1, 0.25f), CUI::CORNER_T, 4.0f);
	//RenderTools()->DrawUIRect(&View, vec4(0, 0, 0, 0.15f), CUI::CORNER_B, 4.0f);
	ServerHeader.HMargin(2.0f, &ServerHeader);
	UI()->DoLabel(&ServerHeader, Localize("Scoreboard"), FontSize + 2.0f, CUI::ALIGN_CENTER);
	RenderDetailScoreboard(ServerScoreboard, pInfo, 0);
}

void CMenus::FriendlistOnUpdate()
{
	// fill me
}

void CMenus::RenderServerbrowserBottomBox(CUIRect MainView)
{
	// same size like tabs in top but variables not really needed
	float Spacing = 3.0f;
	float ButtonWidth = MainView.w/2.0f-Spacing/2.0f;

	// render background
	RenderTools()->DrawUIRect4(&MainView, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

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

void CMenus::DoGameIcon(const char *pName, const CUIRect *pRect)
{
	char aNameBuf[128];
	str_copy(aNameBuf, pName, sizeof(aNameBuf));
	str_sanitize_filename(aNameBuf);

	// get texture
	IGraphics::CTextureHandle Tex = m_GameIconDefault;
	for(int i = 0; i < m_lGameIcons.size(); ++i)
	{
		if(!str_comp_nocase(aNameBuf, m_lGameIcons[i].m_Name))
		{
			Tex = m_lGameIcons[i].m_IconTexture;
			break;
		}
	}

	// draw icon
	Graphics()->TextureSet(Tex);
	Graphics()->QuadsBegin();
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

int CMenus::GameIconScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	const char *pSuffix = str_endswith(pName, ".png");
	if(IsDir || !pSuffix)
	{
		return 0;
	}
	char aGameIconName[128];
	str_truncate(aGameIconName, sizeof(aGameIconName), pName, pSuffix - pName);

	// add new game icon
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "ui/gametypes/%s", pName);
	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType) || Info.m_Width != CGameIcon::GAMEICON_SIZE || (Info.m_Height != CGameIcon::GAMEICON_SIZE && Info.m_Height != CGameIcon::GAMEICON_OLDHEIGHT))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load gametype icon '%s'", aGameIconName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
		return 0;
	}
	CGameIcon GameIcon(aGameIconName);
	str_format(aBuf, sizeof(aBuf), "loaded gametype icon '%s'", aGameIconName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);

	GameIcon.m_IconTexture = pSelf->Graphics()->LoadTextureRaw(CGameIcon::GAMEICON_SIZE, CGameIcon::GAMEICON_SIZE, Info.m_Format, Info.m_pData, Info.m_Format, IGraphics::TEXLOAD_LINEARMIPMAPS);
	pSelf->m_lGameIcons.add(GameIcon);
	if(!str_comp_nocase(aGameIconName, "mod"))
		pSelf->m_GameIconDefault = GameIcon.m_IconTexture;
	return 0;
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
	static bool s_Init = true;
	if(s_Init)
	{
		Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/gametypes", GameIconScan, this);
		s_Init = false;
	}

	CUIRect ServerList, Sidebar, BottomBox, SidebarButton;

	if(Client()->State() == IClient::STATE_OFFLINE)
		MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitBottom(80.0f, &MainView, &BottomBox);
	MainView.VSplitRight(20.0f, &ServerList, &SidebarButton);
	
	if(m_SidebarActive)
		ServerList.VSplitRight(150.0f, &ServerList, &Sidebar);

	// server list
	RenderServerbrowserServerList(ServerList);

	// sidebar
	if(m_SidebarActive)
		RenderServerbrowserSidebar(Sidebar);

	// sidebar button
	SidebarButton.HMargin(150.0f, &SidebarButton);
	static CButtonContainer s_SidebarButton;
	if(DoButton_SpriteID(&s_SidebarButton, IMAGE_ARROWICONS, m_SidebarActive?SPRITE_ARROW_RIGHT_A:SPRITE_ARROW_LEFT_A, false, &SidebarButton, CUI::CORNER_R, 5.0f, true))
	{
		m_SidebarActive ^= 1;
	}

	// back button
	BottomBox.HSplitTop(20.0f, 0, &BottomBox);
	MainView = BottomBox;
	RenderBackButton(MainView);

	// connect box
	float Spacing = 3.0f;
	float ButtonWidth = (BottomBox.w/6.0f)-(Spacing*5.0)/6.0f;
	BottomBox.VSplitRight(20.0f, &BottomBox, 0);
	BottomBox.VSplitRight(ButtonWidth*2.0f+Spacing, 0, &BottomBox);

	RenderServerbrowserBottomBox(BottomBox);

	// render overlay if there is any
	RenderServerbrowserOverlay();
}

void CMenus::UpdateServerBrowserAddress()
{
	const CServerInfo *pItem = GetSelectedServerInfo();
	if(pItem == NULL)
	{
		g_Config.m_UiServerAddress[0] = '\0';
	}
	else
	{
		str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
	}
}

void CMenus::ServerBrowserFilterOnUpdate()
{
	m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND;
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
	CMenus *pMenus = (CMenus*)pUserData;
	pMenus->ServerBrowserFilterOnUpdate();
}
