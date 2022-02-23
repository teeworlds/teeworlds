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
#include <engine/shared/jsonwriter.h>
#include <engine/client/contacts.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/version.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>

#include "menus.h"

CMenus::CColumn CMenus::ms_aBrowserCols[] = {  // Localize("Server"); Localize("Type"); Localize("Map"); Localize("Players"); Localize("Ping"); - these strings are localized within CLocConstString
	{COL_BROWSER_FLAG,		-1,									" ",		-1, 4*16.0f+3*2.0f, 0, {0}, {0}, TEXTALIGN_CENTER},
	{COL_BROWSER_NAME,		IServerBrowser::SORT_NAME,			"Server",	0, 310.0f, 0, {0}, {0}, TEXTALIGN_CENTER},
	{COL_BROWSER_GAMETYPE,	IServerBrowser::SORT_GAMETYPE,		"Type",		1, 70.0f,  0, {0}, {0}, TEXTALIGN_CENTER},
	{COL_BROWSER_MAP,		IServerBrowser::SORT_MAP,			"Map",		1, 100.0f, 0, {0}, {0}, TEXTALIGN_CENTER},
	{COL_BROWSER_PLAYERS,	IServerBrowser::SORT_NUMPLAYERS,	"Players",	1, 50.0f,  0, {0}, {0}, TEXTALIGN_CENTER},
	{COL_BROWSER_PING,		IServerBrowser::SORT_PING,			"Ping",		1, 40.0f,  0, {0}, {0}, TEXTALIGN_CENTER},
};

CServerFilterInfo CMenus::CBrowserFilter::ms_FilterStandard = {IServerBrowser::FILTER_COMPAT_VERSION|IServerBrowser::FILTER_PURE|IServerBrowser::FILTER_PURE_MAP, 999, -1, 0, {{0}}, {0}, {0}};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterRace = {IServerBrowser::FILTER_COMPAT_VERSION, 999, -1, 0, {{"Race"}}, {false}, {0}};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterFavorites = {IServerBrowser::FILTER_COMPAT_VERSION|IServerBrowser::FILTER_FAVORITE, 999, -1, 0, {{0}}, {0}, {0}};
CServerFilterInfo CMenus::CBrowserFilter::ms_FilterAll = {IServerBrowser::FILTER_COMPAT_VERSION, 999, -1, 0, {{0}}, {0}, {0}};

static CLocConstString s_aDifficultyLabels[] = {
	"Casual",
	"Normal",
	"Competitive" };
static int s_aDifficultySpriteIds[] = {
	SPRITE_LEVEL_A_ON,
	SPRITE_LEVEL_B_ON,
	SPRITE_LEVEL_C_ON };

vec3 TextHighlightColor = vec3(0.4f, 0.4f, 1.0f);

// filters
CMenus::CBrowserFilter::CBrowserFilter(int Custom, const char* pName, IServerBrowser *pServerBrowser)
	: m_DeleteButtonContainer(true), m_UpButtonContainer(true), m_DownButtonContainer(true)
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
	case CBrowserFilter::FILTER_RACE:
		m_Filter = m_pServerBrowser->AddFilter(&ms_FilterRace);
		break;
	case CBrowserFilter::FILTER_FAVORITES:
		m_Filter = m_pServerBrowser->AddFilter(&ms_FilterFavorites);
		break;
	default:
		m_Filter = m_pServerBrowser->AddFilter(&ms_FilterAll);
	}
}

void CMenus::CBrowserFilter::Reset()
{
	switch(m_Custom)
	{
	case CBrowserFilter::FILTER_STANDARD:
		SetFilter(&ms_FilterStandard);
		break;
	case CBrowserFilter::FILTER_RACE:
		SetFilter(&ms_FilterRace);
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
	char *pFileData = (char *)mem_alloc(FileSize);
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
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "failed to load filters from '%s': %s", pFilename, aError);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
		return;
	}

	// extract settings data
	const json_value &rSettingsEntry = (*pJsonData)["settings"];
	if(rSettingsEntry["sidebar_active"].type == json_integer)
		m_SidebarActive = rSettingsEntry["sidebar_active"].u.integer;
	if(rSettingsEntry["sidebar_tab"].type == json_integer)
		m_SidebarTab = clamp(int(rSettingsEntry["sidebar_tab"].u.integer), int(SIDEBAR_TAB_INFO), int(NUM_SIDEBAR_TABS-1));

	const int AllFilterIndex = CBrowserFilter::NUM_FILTERS - 2; // -2 because custom filters have index 0 but come last in the list
	if(rSettingsEntry["filters"].type == json_array)
	{
		for(unsigned i = 0; i < IServerBrowser::NUM_TYPES; ++i)
		{
			if(i < rSettingsEntry["filters"].u.array.length && rSettingsEntry["filters"][i].type == json_integer)
				m_aSelectedFilters[i] = rSettingsEntry["filters"][i].u.integer;
			else
				m_aSelectedFilters[i] = AllFilterIndex; // default to "all" if not set for all filters
		}
	}
	else
	{
		for(unsigned i = 0; i < IServerBrowser::NUM_TYPES; ++i)
			m_aSelectedFilters[i] = AllFilterIndex; // default to "all" if not set
	}

	// extract filter data
	const json_value &rFilterEntry = (*pJsonData)["filter"];
	for(unsigned i = 0; i < rFilterEntry.u.array.length; ++i)
	{
		char *pName = rFilterEntry[i].u.object.values[0].name;
		const json_value &rStart = *(rFilterEntry[i].u.object.values[0].value);
		if(rStart.type != json_object)
			continue;

		int Type = CBrowserFilter::FILTER_CUSTOM;
		if(rStart["type"].type == json_integer)
			Type = rStart["type"].u.integer;

		// filter setting
		CServerFilterInfo FilterInfo;
		for(int j = 0; j < CServerFilterInfo::MAX_GAMETYPES; ++j)
		{
			FilterInfo.m_aGametype[j][0] = 0;
			FilterInfo.m_aGametypeExclusive[j] = false;
		}
		const json_value &rSubStart = rStart["settings"];
		if(rSubStart.type == json_object)
		{
			if(rSubStart["filter_hash"].type == json_integer)
				FilterInfo.m_SortHash = rSubStart["filter_hash"].u.integer;

			const json_value &rGametypeEntry = rSubStart["filter_gametype"];
			if(rGametypeEntry.type == json_array) // legacy: all entries are inclusive
			{
				for(unsigned j = 0; j < rGametypeEntry.u.array.length && j < CServerFilterInfo::MAX_GAMETYPES; ++j)
				{
					if(rGametypeEntry[j].type == json_string)
					{
						str_copy(FilterInfo.m_aGametype[j], rGametypeEntry[j].u.string.ptr, sizeof(FilterInfo.m_aGametype[j]));
						FilterInfo.m_aGametypeExclusive[j] = false;
					}
				}
			}
			else if(rGametypeEntry.type == json_object)
			{
				for(unsigned j = 0; j < rGametypeEntry.u.object.length && j < CServerFilterInfo::MAX_GAMETYPES; ++j)
				{
					const json_value &rValue = *(rGametypeEntry.u.object.values[j].value);
					if(rValue.type == json_boolean)
					{
						str_copy(FilterInfo.m_aGametype[j], rGametypeEntry.u.object.values[j].name, sizeof(FilterInfo.m_aGametype[j]));
						FilterInfo.m_aGametypeExclusive[j] = rValue.u.boolean;
					}
				}
			}

			if(rSubStart["filter_ping"].type == json_integer)
				FilterInfo.m_Ping = rSubStart["filter_ping"].u.integer;
			if(rSubStart["filter_serverlevel"].type == json_integer)
				FilterInfo.m_ServerLevel = rSubStart["filter_serverlevel"].u.integer;
			if(rSubStart["filter_address"].type == json_string)
				str_copy(FilterInfo.m_aAddress, rSubStart["filter_address"].u.string.ptr, sizeof(FilterInfo.m_aAddress));
			if(rSubStart["filter_country"].type == json_integer)
				FilterInfo.m_Country = rSubStart["filter_country"].u.integer;
		}

		m_lFilters.add(CBrowserFilter(Type, pName, ServerBrowser()));

		if(Type == CBrowserFilter::FILTER_STANDARD) // make sure the pure filter is enabled in the Teeworlds-filter
			FilterInfo.m_SortHash |= IServerBrowser::FILTER_PURE;
		else if(Type == CBrowserFilter::FILTER_RACE) // make sure Race gametype is included in Race-filter
		{
			str_copy(FilterInfo.m_aGametype[0], "Race", sizeof(FilterInfo.m_aGametype[0]));
			FilterInfo.m_aGametypeExclusive[0] = false;
		}

		m_lFilters[i].SetFilter(&FilterInfo);
	}

	// clean up
	json_value_free(pJsonData);

	CBrowserFilter *pSelectedFilter = GetSelectedBrowserFilter();
	if(pSelectedFilter)
		pSelectedFilter->Switch();
}

void CMenus::SaveFilters()
{
	IOHANDLE File = Storage()->OpenFile("ui_settings.json", IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;

	CJsonWriter Writer(File);

	Writer.BeginObject(); // root

	// settings
	Writer.WriteAttribute("settings");
	Writer.BeginObject();
	{
		Writer.WriteAttribute("sidebar_active");
		Writer.WriteIntValue(m_SidebarActive);

		Writer.WriteAttribute("sidebar_tab");
		Writer.WriteIntValue(m_SidebarTab);

		Writer.WriteAttribute("filters");
		Writer.BeginArray();
		for(int i = 0; i < IServerBrowser::NUM_TYPES; i++)
			Writer.WriteIntValue(m_aSelectedFilters[i]);
		Writer.EndArray();
	}
	Writer.EndObject();

	// filter
	Writer.WriteAttribute("filter");
	Writer.BeginArray();
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		// part start
		Writer.BeginObject();
		Writer.WriteAttribute(m_lFilters[i].Name());
		Writer.BeginObject();
		{
			Writer.WriteAttribute("type");
			Writer.WriteIntValue(m_lFilters[i].Custom());

			// filter setting
			CServerFilterInfo FilterInfo;
			m_lFilters[i].GetFilter(&FilterInfo);

			Writer.WriteAttribute("settings");
			Writer.BeginObject();
			{
				Writer.WriteAttribute("filter_hash");
				Writer.WriteIntValue(FilterInfo.m_SortHash);

				Writer.WriteAttribute("filter_gametype");
				Writer.BeginObject();
				for(unsigned j = 0; j < CServerFilterInfo::MAX_GAMETYPES && FilterInfo.m_aGametype[j][0]; ++j)
				{
					Writer.WriteAttribute(FilterInfo.m_aGametype[j]);
					Writer.WriteBoolValue(FilterInfo.m_aGametypeExclusive[j]);
				}
				Writer.EndObject();

				Writer.WriteAttribute("filter_ping");
				Writer.WriteIntValue(FilterInfo.m_Ping);

				Writer.WriteAttribute("filter_serverlevel");
				Writer.WriteIntValue(FilterInfo.m_ServerLevel);

				Writer.WriteAttribute("filter_address");
				Writer.WriteStrValue(FilterInfo.m_aAddress);

				Writer.WriteAttribute("filter_country");
				Writer.WriteIntValue(FilterInfo.m_Country);
			}
			Writer.EndObject();
		}
		Writer.EndObject();
		Writer.EndObject();
	}
	Writer.EndArray();

	Writer.EndObject(); // end root
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

void CMenus::MoveFilter(bool Up, int Filter)
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

void CMenus::InitDefaultFilters()
{
	int Filters = 0;
	for(int i = 0; i < m_lFilters.size(); i++)
		Filters |= 1 << m_lFilters[i].Custom();

	const bool UseDefaultFilters = Filters == 0;

	if((Filters & (1 << CBrowserFilter::FILTER_STANDARD)) == 0)
	{
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_STANDARD, "Teeworlds", ServerBrowser()));
		for(int Pos = m_lFilters.size() - 1; Pos > 0; --Pos)
			MoveFilter(true, Pos);
	}

	if((Filters & (1 << CBrowserFilter::FILTER_RACE)) == 0)
	{
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_RACE, Localize("Race"), ServerBrowser()));
		for(int Pos = m_lFilters.size() - 1; Pos > 1; --Pos)
			MoveFilter(true, Pos);
	}

	if((Filters & (1 << CBrowserFilter::FILTER_FAVORITES)) == 0)
	{
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_FAVORITES, Localize("Favorites"), ServerBrowser()));
		for(int Pos = m_lFilters.size() - 1; Pos > 2; --Pos)
			MoveFilter(true, Pos);
	}

	if((Filters & (1 << CBrowserFilter::FILTER_ALL)) == 0)
	{
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_ALL, Localize("All"), ServerBrowser()));
		for(int Pos = m_lFilters.size() - 1; Pos > 3; --Pos)
			MoveFilter(true, Pos);
	}

	// expand the all filter tab by default
	if(UseDefaultFilters)
	{
		const int AllFilterIndex = m_lFilters.size()-1;
		for(unsigned i = 0; i < IServerBrowser::NUM_TYPES; ++i)
			m_aSelectedFilters[i] = AllFilterIndex; // default to "all" if not set
		m_lFilters[AllFilterIndex].Switch();
	}
}

// 1 = browser entry click, 2 = server info click
int CMenus::DoBrowserEntry(const void *pID, CUIRect View, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected, bool ShowServerInfo, CScrollRegion *pScroll)
{
	// logic
	int ReturnValue = 0;

	const bool Hovered = UI()->MouseHovered(&View);
	bool Highlighted = Hovered && (!pScroll || !pScroll->IsAnimating());

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
		{
			if(Hovered)
				ReturnValue = 1;
			UI()->SetActiveItem(0);
		}
	}
	if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
			UI()->SetActiveItem(pID);
	}

	if(Highlighted)
	{
		UI()->SetHotItem(pID);
		View.Draw(vec4(1.0f, 1.0f, 1.0f, 0.5f));
	}
	else if(Selected)
	{
		View.Draw(vec4(0.8f, 0.8f, 0.8f, 0.5f));
	}

	const float FontSize = 12.0f;
	const float TextAlpha = (pEntry->m_NumClients == pEntry->m_MaxClients) ? 0.5f : 1.0f;
	vec4 TextBaseColor = vec4(1.0f, 1.0f, 1.0f, TextAlpha);
	vec4 TextBaseOutlineColor = vec4(0.0, 0.0, 0.0, 0.3f);
	vec4 ServerInfoTextBaseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 HighlightColor = vec4(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextAlpha);
	if(Selected || Highlighted)
	{
		TextBaseColor = vec4(0.0f, 0.0f, 0.0f, TextAlpha);
		ServerInfoTextBaseColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		TextBaseOutlineColor = vec4(0.8f, 0.8f, 0.8f, 0.25f);
	}

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
				UI()->DoTooltip(&pEntry->m_Flags, &Icon, Localize("This server is protected by a password."));
			}

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			DoIcon(IMAGE_LEVELICONS, s_aDifficultySpriteIds[pEntry->m_ServerLevel], &Icon);
			UI()->DoTooltip(&pEntry->m_ServerLevel, &Icon, s_aDifficultyLabels[pEntry->m_ServerLevel]);

			Rect.VSplitLeft(Rect.h, &Icon, &Rect);
			Icon.Margin(2.0f, &Icon);
			DoIcon(IMAGE_BROWSEICONS, pEntry->m_Favorite ? SPRITE_BROWSE_STAR_A : SPRITE_BROWSE_STAR_B, &Icon);
			if(UI()->DoButtonLogic(&pEntry->m_Favorite, &Icon))
			{
				if(!pEntry->m_Favorite)
					ServerBrowser()->AddFavorite(pEntry);
				else
					ServerBrowser()->RemoveFavorite(pEntry);
			}
			UI()->DoTooltip(&pEntry->m_Favorite, &Icon, pEntry->m_Favorite ? Localize("Click to remove server from favorites.") : Localize("Click to add server to favorites."));

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
				UI()->DoTooltip(&pEntry->m_FriendState, &Icon, Localize("Friends are online on this server."));
			}
		}
		else if(ID == COL_BROWSER_NAME)
		{
			TextRender()->TextColor(TextBaseColor);
			TextRender()->TextSecondaryColor(TextBaseOutlineColor);
			Button.y += (Button.h - FontSize/CUI::ms_FontmodHeight)/2.0f;
			UI()->DoLabelHighlighted(&Button, pEntry->m_aName, (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME) ? Config()->m_BrFilterString : 0, FontSize, TextBaseColor, HighlightColor);
		}
		else if(ID == COL_BROWSER_MAP)
		{
			TextRender()->TextColor(TextBaseColor);
			Button.y += (Button.h - FontSize/CUI::ms_FontmodHeight)/2.0f;
			UI()->DoLabelHighlighted(&Button, pEntry->m_aMap, (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_MAPNAME) ? Config()->m_BrFilterString : 0, FontSize, TextBaseColor, HighlightColor);
		}
		else if(ID == COL_BROWSER_PLAYERS)
		{
			TextRender()->TextColor(TextBaseColor);
			TextRender()->TextSecondaryColor(TextBaseOutlineColor);
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
			static float s_RenderOffset = 0.0f;
			if(s_RenderOffset == 0.0f)
				s_RenderOffset = TextRender()->TextWidth(FontSize, "0", -1);

			str_format(aTemp, sizeof(aTemp), "%d/%d", Num, Max);
			if(Config()->m_BrFilterString[0] && (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_PLAYER))
				TextRender()->TextColor(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextAlpha);
			Button.y += (Button.h - FontSize/CUI::ms_FontmodHeight)/2.0f;

			if(Num < 100)
				Button.x += s_RenderOffset;
			if(Num < 10)
				Button.x += s_RenderOffset;
			if(!Num)
				TextRender()->TextColor(CUI::ms_TransparentTextColor);
			UI()->DoLabel(&Button, aTemp, FontSize, TEXTALIGN_LEFT);
			Button.x += TextRender()->TextWidth(FontSize, aTemp, -1);
		}
		else if(ID == COL_BROWSER_PING)
		{
			const int Ping = pEntry->m_Latency;

			vec4 Color;
			if(Selected || Highlighted)
			{
				Color = TextBaseColor;
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

			str_format(aTemp, sizeof(aTemp), "%d", Ping);
			TextRender()->TextColor(Color);
			TextRender()->TextSecondaryColor(TextBaseOutlineColor);
			Button.y += (Button.h - FontSize/CUI::ms_FontmodHeight)/2.0f;
			Button.w -= 4.0f;
			UI()->DoLabel(&Button, aTemp, FontSize, TEXTALIGN_RIGHT);
		}
		else if(ID == COL_BROWSER_GAMETYPE)
		{
			// gametype icon
			CUIRect Icon;
			Button.VSplitLeft(Button.h, &Icon, &Button);
			Icon.y -= 0.5f;
			DoGameIcon(pEntry->m_aGameType, &Icon);

			// gametype text
			TextRender()->TextColor(TextBaseColor);
			TextRender()->TextSecondaryColor(TextBaseOutlineColor);
			Button.y += (Button.h - FontSize/CUI::ms_FontmodHeight)/2.0f;
			UI()->DoLabelHighlighted(&Button, pEntry->m_aGameType, (pEntry->m_QuickSearchHit&IServerBrowser::QUICK_GAMETYPE) ? Config()->m_BrFilterString : 0, FontSize, TextBaseColor, HighlightColor);
		}
	}

	// show server info
	if(ShowServerInfo)
	{
		View.HSplitTop(ms_aBrowserCols[0].m_Rect.h, 0, &View);

		if(ReturnValue && UI()->MouseHovered(&View))
			ReturnValue++;

		CUIRect Info, Scoreboard;
		View.VSplitLeft(160.0f, &Info, &Scoreboard);
		RenderDetailInfo(Info, pEntry, ServerInfoTextBaseColor, TextBaseOutlineColor);
		RenderDetailScoreboard(Scoreboard, pEntry, 4, ServerInfoTextBaseColor, TextBaseOutlineColor);
	}

	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);

	return ReturnValue;
}

void CMenus::RenderFilterHeader(CUIRect View, int FilterIndex)
{
	CBrowserFilter *pFilter = &m_lFilters[FilterIndex];

	float ButtonHeight = 20.0f;
	float Spacing = 3.0f;
	bool Switch = false;

	View.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Button, EditButtons;
	if(UI()->DoButtonLogic(pFilter, &View))
	{
		Switch = true; // switch later, to make sure we haven't clicked one of the filter buttons (edit...)
	}
	vec4 Color = UI()->MouseHovered(&View) ? vec4(1.0f, 1.0f, 1.0f, 1.0f) : vec4(0.6f, 0.6f, 0.6f, 1.0f);
	View.VSplitLeft(20.0f, &Button, &View);
	Button.Margin(2.0f, &Button);
	DoIcon(IMAGE_MENUICONS, pFilter->Extended() ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED, &Button, &Color);

	// split buttons from label
	View.VSplitLeft(Spacing, 0, &View);
	View.VSplitRight((ButtonHeight+Spacing)*4.0f, &View, &EditButtons);

	View.VSplitLeft(20.0f, 0, &View); // little space
	UI()->DoLabel(&View, pFilter->Name(), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_ML);

	View.VSplitRight(20.0f, &View, 0); // little space
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("%d servers, %d players"), pFilter->NumSortedServers(), pFilter->NumPlayers());
	UI()->DoLabel(&View, aBuf, ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_RIGHT);

	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(pFilter->Custom() == CBrowserFilter::FILTER_CUSTOM)
	{
		if(DoButton_SpriteID(&pFilter->m_DeleteButtonContainer, IMAGE_TOOLICONS, SPRITE_TOOL_X_A, false, &Button))
		{
			m_RemoveFilterIndex = FilterIndex;
			str_format(aBuf, sizeof(aBuf), Localize("Are you sure that you want to remove the filter '%s' from the server browser?"), pFilter->Name());
			PopupConfirm(Localize("Remove filter"), aBuf, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmRemoveFilter);
		}
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_X_B, &Button);

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(FilterIndex > 0)
	{
		if(DoButton_SpriteID(&pFilter->m_UpButtonContainer, IMAGE_TOOLICONS, SPRITE_TOOL_UP_A, false, &Button))
		{
			MoveFilter(true, FilterIndex);
			Switch = false;
		}
	}
	else
		DoIcon(IMAGE_TOOLICONS, SPRITE_TOOL_UP_B, &Button);

	EditButtons.VSplitRight(Spacing, &EditButtons, 0);
	EditButtons.VSplitRight(ButtonHeight, &EditButtons, &Button);
	Button.Margin(2.0f, &Button);
	if(FilterIndex < m_lFilters.size() - 1)
	{
		if(DoButton_SpriteID(&pFilter->m_DownButtonContainer, IMAGE_TOOLICONS, SPRITE_TOOL_DOWN_A, false, &Button))
		{
			MoveFilter(false, FilterIndex);
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

void CMenus::PopupConfirmRemoveFilter()
{
	// remove filter
	if(m_RemoveFilterIndex)
	{
		RemoveFilter(m_RemoveFilterIndex);
	}
}

void CMenus::PopupConfirmCountryFilter()
{
	CBrowserFilter *pFilter = GetSelectedBrowserFilter();
	CServerFilterInfo FilterInfo;
	pFilter->GetFilter(&FilterInfo);

	if(m_PopupCountrySelection != -2)
		FilterInfo.m_Country = m_PopupCountrySelection;

	pFilter->SetFilter(&FilterInfo);
}

static void FormatScore(char *pBuf, int BufSize, bool TimeScore, const CServerInfo::CClient *pClient)
{
	if(TimeScore)
		FormatTime(pBuf, BufSize, pClient->m_Score * 1000, 0);
	else
		str_format(pBuf, BufSize, "%d", pClient->m_Score);
}

void CMenus::RenderServerbrowserServerList(CUIRect View)
{
	CUIRect Headers, Status;

	const float SpacingH = 2.0f;
	const float ButtonHeight = 20.0f;
	const float HeaderHeight = UI()->GetListHeaderHeight();
	const float HeightFactor = UI()->GetListHeaderHeightFactor();

	// background
	View.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, (Client()->State() == IClient::STATE_OFFLINE) ? CUIRect::CORNER_ALL : CUIRect::CORNER_B|CUIRect::CORNER_TR);

	// make room for scrollbar
	{
		CUIRect Scroll;
		View.VSplitRight(20.0f, &View, &Scroll);
	}

	View.HSplitTop(HeaderHeight, &Headers, &View);
	View.HSplitBottom(ButtonHeight*3.0f+SpacingH*2.0f, &View, &Status);

	Headers.VSplitRight(2.f, &Headers, 0); // some margin on the right

	// do layout
	for(int i = 0; i < NUM_BROWSER_COLS; i++)
	{
		if(ms_aBrowserCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(ms_aBrowserCols[i].m_Width*HeightFactor, &ms_aBrowserCols[i].m_Rect, &Headers);

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
			Headers.VSplitRight(ms_aBrowserCols[i].m_Width*HeightFactor, &Headers, &ms_aBrowserCols[i].m_Rect);
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

		if(DoButton_GridHeader(ms_aBrowserCols[i].m_Caption, ms_aBrowserCols[i].m_Caption, Config()->m_BrSort == ms_aBrowserCols[i].m_Sort, ms_aBrowserCols[i].m_Align, &ms_aBrowserCols[i].m_Rect))
		{
			if(ms_aBrowserCols[i].m_Sort != -1)
			{
				if(Config()->m_BrSort == ms_aBrowserCols[i].m_Sort)
					Config()->m_BrSortOrder ^= 1;
				else
					Config()->m_BrSortOrder = 0;
				Config()->m_BrSort = ms_aBrowserCols[i].m_Sort;
			}
			ServerBrowserSortingOnUpdate();
		}
	}

	// list background
	View.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f, CUIRect::CORNER_L);
	{
		int Column = COL_BROWSER_PING;
		switch(Config()->m_BrSort)
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
		Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.05f));
	}

	// update selection based on address if it changed
	if(ServerBrowser()->WasUpdated(true))
		m_AddressSelection |= ADDR_SELECTION_CHANGE;

	const int BrowserType = ServerBrowser()->GetType();
	int ToBeSelectedFilter = -2; // -2 to not restore, -1 to restore all filters closed
	if(m_LastBrowserType == -1)
		m_LastBrowserType = BrowserType;
	else if(BrowserType != m_LastBrowserType)
	{
		// restore selected filter and server when changing browser page
		m_LastBrowserType = BrowserType;
		ToBeSelectedFilter = m_aSelectedFilters[BrowserType];
		if(ToBeSelectedFilter != -1)
		{
			if(m_aSelectedServers[BrowserType] == -1)
				m_AddressSelection |= ADDR_SELECTION_CHANGE;
			else
				m_AddressSelection |= ADDR_SELECTION_REVEAL | ADDR_SELECTION_UPDATE_ADDRESS;
		}
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
		m_aSelectedServers[BrowserType] = -1;
		if(SelectedFilter != -1)
		{
			m_AddressSelection |= ADDR_SELECTION_CHANGE;
		}
	}

	const bool CtrlPressed = Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL);

	// handle arrow hotkeys
	const int LastSelectedFilter = m_aSelectedFilters[BrowserType];
	const int LastSelectedServer = m_aSelectedServers[BrowserType];
	if(SelectedFilter > -1)
	{
		int NewFilter = SelectedFilter;
		int ToBeSelectedServer = -1;

		if(UI()->ConsumeHotkey(CUI::HOTKEY_DOWN))
		{
			if(!CtrlPressed)
			{
				ToBeSelectedServer = m_aSelectedServers[BrowserType] < 0 ? 0 : (m_aSelectedServers[BrowserType] + 1);
				if(ToBeSelectedServer >= m_lFilters[SelectedFilter].NumSortedServers())
					ToBeSelectedServer = m_lFilters[SelectedFilter].NumSortedServers() - 1;
			}
			else if(SelectedFilter + 1 < m_lFilters.size())
			{
				// move to next filter
				NewFilter = SelectedFilter + 1;
			}
		}
		else if(UI()->ConsumeHotkey(CUI::HOTKEY_UP))
		{
			if(!CtrlPressed)
			{
				ToBeSelectedServer = m_aSelectedServers[BrowserType] < 0 ? 0 : (m_aSelectedServers[BrowserType] - 1);
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
			m_aSelectedServers[BrowserType] = -1;
			m_AddressSelection |= ADDR_SELECTION_CHANGE;
		}

		if(ToBeSelectedServer > -1 && ToBeSelectedServer < m_lFilters[NewFilter].NumSortedServers())
		{
			m_aSelectedFilters[BrowserType] = NewFilter;
			if(m_aSelectedServers[BrowserType] != ToBeSelectedServer)
			{
				m_aSelectedServers[BrowserType] = ToBeSelectedServer;
				m_ShowServerDetails = false; // close server details when scrolling with arrows, as this leads to flicking
				m_AddressSelection |= ADDR_SELECTION_REVEAL;
			}

			m_AddressSelection |= ADDR_SELECTION_UPDATE_ADDRESS;
		}
	}

	// scrollbar
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ScrollParams.m_SliderMinHeight = 5;
	ScrollParams.m_ScrollUnit = 60.0f; // 3 rows per scroll
	View.w += ScrollParams.m_ScrollbarWidth;
	s_ScrollRegion.Begin(&View, &ScrollOffset, &ScrollParams);
	View.y += ScrollOffset.y;

	const char *pAddress = GetServerBrowserAddress();
	for(int FilterIndex = 0; FilterIndex < m_lFilters.size(); FilterIndex++)
	{
		CBrowserFilter *pFilter = &m_lFilters[FilterIndex];

		// filter header
		CUIRect Header;
		View.HSplitTop(20.0f, &Header, &View);
		s_ScrollRegion.AddRect(Header);

		// render header
		RenderFilterHeader(Header, FilterIndex);

		if(pFilter->Extended())
		{
			for (int ServerIndex = 0; ServerIndex < pFilter->NumSortedServers(); ServerIndex++)
			{
				const CServerInfo *pItem = pFilter->SortedGet(ServerIndex);

				// select server if address changed and match found
				bool IsSelected = m_aSelectedFilters[BrowserType] == FilterIndex && m_aSelectedServers[BrowserType] == ServerIndex;
				if(m_AddressSelection&ADDR_SELECTION_CHANGE)
				{
					if (!str_comp(pItem->m_aAddress, pAddress))
					{
						if(!IsSelected)
						{
							m_ShowServerDetails = true;
							m_aSelectedFilters[BrowserType] = FilterIndex;
							m_aSelectedServers[BrowserType] = ServerIndex;
							IsSelected = true;
						}
						m_AddressSelection &= ~(ADDR_SELECTION_CHANGE|ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND);
					}
					else
					{
						IsSelected = false;
					}
				}

				const bool ShowServerInfo = !m_SidebarActive && m_ShowServerDetails && IsSelected;
				const float ItemHeight = HeaderHeight * (ShowServerInfo ? 6.0f : 1.0f);

				CUIRect Row;
				View.HSplitTop(ItemHeight, &Row, &View);
				s_ScrollRegion.AddRect(Row);

				if(IsSelected && (m_AddressSelection&ADDR_SELECTION_REVEAL)) // new selection (hotkeys or address input)
				{
					s_ScrollRegion.ScrollHere();
					m_AddressSelection &= ~ADDR_SELECTION_REVEAL;
				}
				else if(s_ScrollRegion.IsRectClipped(Row))
				{
					// only those in the view are rendered and can be selected
					continue;
				}

				// Prevent flickering entry background and text by drawing it as selected for one more frame after the selection has changed
				const bool WasSelected = LastSelectedFilter >= 0 && LastSelectedServer >= 0 && FilterIndex == LastSelectedFilter && ServerIndex == LastSelectedServer;
				if(int ReturnValue = DoBrowserEntry(pFilter->ID(ServerIndex), Row, pItem, pFilter, IsSelected || WasSelected, ShowServerInfo, &s_ScrollRegion))
				{
					m_ShowServerDetails = !m_ShowServerDetails || ReturnValue == 2 || m_aSelectedServers[BrowserType] != ServerIndex; // click twice on line => fold server details
					m_aSelectedFilters[BrowserType] = FilterIndex;
					m_aSelectedServers[BrowserType] = ServerIndex;
					m_AddressSelection &= ~(ADDR_SELECTION_CHANGE|ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND);
					if(Config()->m_UiAutoswitchInfotab)
						m_SidebarTab = SIDEBAR_TAB_INFO;
					UpdateServerBrowserAddress(); // update now instead of using flag because of connect
					if(Input()->MouseDoubleClick())
						Client()->Connect(GetServerBrowserAddress());
				}
			}

			if((m_AddressSelection&ADDR_SELECTION_CHANGE) && (m_AddressSelection&ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND))
			{
				m_aSelectedServers[BrowserType] = -1;
				m_AddressSelection &= ~(ADDR_SELECTION_CHANGE|ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND);
			}
		}

		if(FilterIndex < m_lFilters.size()-1)
		{
			CUIRect Space;
			View.HSplitTop(SpacingH, &Space, &View);
			s_ScrollRegion.AddRect(Space);
		}
	}

	if(m_AddressSelection&ADDR_SELECTION_UPDATE_ADDRESS)
	{
		UpdateServerBrowserAddress();
		m_AddressSelection &= ~ADDR_SELECTION_UPDATE_ADDRESS;
	}

	// display important messages in the middle of the screen so no user misses it
	if(!NumServers)
	{
		CUIRect MsgBox = View;
		if(MsgBox.h < 50.0f)
			MsgBox.h = 50.0f;
		s_ScrollRegion.AddRect(MsgBox);
		if(!s_ScrollRegion.IsRectClipped(MsgBox))
		{
			char aBuf[128];
			const char *pImportantMessage = 0;
			if(m_ActivePage == PAGE_INTERNET && ServerBrowser()->IsRefreshingMasters())
				pImportantMessage = Localize("Refreshing master servers");
			else if(SelectedFilter == -1)
				pImportantMessage = Localize("No filter category is selected");
			else if(ServerBrowser()->IsRefreshing())
				pImportantMessage = Localize("Fetching server info");
			else if(!ServerBrowser()->NumServers())
			{
				if(BrowserType == IServerBrowser::TYPE_INTERNET)
					pImportantMessage = Localize("No servers found");
				else if(BrowserType == IServerBrowser::TYPE_LAN)
				{
					str_format(aBuf, sizeof(aBuf), Localize("No local servers found (ports %d–%d)"), IServerBrowser::LAN_PORT_BEGIN, IServerBrowser::LAN_PORT_END);
					pImportantMessage = aBuf;
				}
			}
			else
				pImportantMessage = Localize("No servers match your filter criteria");

			if(pImportantMessage)
			{
				MsgBox.y += MsgBox.h/3.0f;
				UI()->DoLabel(&MsgBox, pImportantMessage, 16.0f, TEXTALIGN_CENTER);
			}
		}
	}

	s_ScrollRegion.End();

	// bottom
	float SpacingW = 3.0f;
	float ButtonWidth = (Status.w/6.0f)-(SpacingW*5.0)/6.0f;
	float FontSize = ButtonHeight*CUI::ms_FontmodHeight*0.8f;

	// cut view
	CUIRect Left, Label, EditBox, Button;
	Status.VSplitLeft(ButtonWidth*3.5f+SpacingH*2.0f, &Left, &Status);

	// render quick search and host address
	Left.HSplitTop(((ButtonHeight*3.0f+SpacingH*2.0f)-(ButtonHeight*2.0f+SpacingH))/2.0f, 0, &Left);
	Left.HSplitTop(ButtonHeight, &Label, &Left);
	Label.VSplitLeft(2.0f, 0, &Label);
	Label.VSplitRight(ButtonWidth*2.0f+SpacingH, &Label, &EditBox);
	UI()->DoLabel(&Label, Localize("Search:"), FontSize, TEXTALIGN_ML);
	EditBox.VSplitRight(EditBox.h, &EditBox, &Button);
	static CLineInput s_FilterInput(Config()->m_BrFilterString, sizeof(Config()->m_BrFilterString));
	if(UI()->DoEditBox(&s_FilterInput, &EditBox, FontSize, CUIRect::CORNER_L))
	{
		Client()->ServerBrowserUpdate();
		ServerBrowserFilterOnUpdate();
	}

	// clear button
	{
		static CButtonContainer s_ClearButton;
		if(DoButton_SpriteID(&s_ClearButton, IMAGE_TOOLICONS, SPRITE_TOOL_X_A, false, &Button, CUIRect::CORNER_R, 5.0f, true))
		{
			Config()->m_BrFilterString[0] = 0;
			UI()->SetActiveItem(&Config()->m_BrFilterString);
			Client()->ServerBrowserUpdate();
		}
	}

	Left.HSplitTop(SpacingH, 0, &Left);
	Left.HSplitTop(ButtonHeight, &Label, 0);
	Label.VSplitLeft(2.0f, 0, &Label);
	Label.VSplitRight(ButtonWidth*2.0f+SpacingH, &Label, &EditBox);
	UI()->DoLabel(&Label, Localize("Host address:"), FontSize, TEXTALIGN_ML);

	if(BrowserType == IServerBrowser::TYPE_INTERNET)
	{
		static CLineInput s_AddressInput(Config()->m_UiServerAddress, sizeof(Config()->m_UiServerAddress));
		if(UI()->DoEditBox(&s_AddressInput, &EditBox, FontSize))
		{
			m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND | ADDR_SELECTION_REVEAL;
		}
	}
	else if(BrowserType == IServerBrowser::TYPE_LAN)
	{
		static CLineInput s_AddressInput(Config()->m_UiServerAddressLan, sizeof(Config()->m_UiServerAddressLan));
		if(UI()->DoEditBox(&s_AddressInput, &EditBox, FontSize))
		{
			m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND | ADDR_SELECTION_REVEAL;
		}
	}

	// render status
	if(ServerBrowser()->IsRefreshing() && m_ActivePage != PAGE_LAN)
	{
		char aBuf[128];
		Status.HSplitTop(ButtonHeight + SpacingH, 0, &Status);
		str_format(aBuf, sizeof(aBuf), Localize("%d%% loaded"), ServerBrowser()->LoadingProgression());
		UI()->DoLabel(&Status, aBuf, 14.0f, TEXTALIGN_MC);
	}
	else
	{
		// todo: move this to a helper function
		static float RenderOffset = 0.0f;
		if(RenderOffset == 0.0f)
			RenderOffset = TextRender()->TextWidth(FontSize, "0", -1);

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
		Label.x += OffsetServer;
		UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);

		Status.HSplitTop(SpacingH, 0, &Status);
		Status.HSplitTop(ButtonHeight, &Label, 0);
		str_format(aBuf, sizeof(aBuf), Localize("%d players"), NumPlayers);
		Label.x += OffsetPlayer;
		UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);
	}
}

void CMenus::RenderServerbrowserSidebar(CUIRect View)
{
	CUIRect Header, Button;

	// background
	View.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f));

	// handle Tab key
	if(UI()->ConsumeHotkey(CUI::HOTKEY_TAB))
	{
		if(Input()->KeyIsPressed(KEY_LSHIFT) || Input()->KeyIsPressed(KEY_RSHIFT))
		{
			m_SidebarTab--;
			if(m_SidebarTab < SIDEBAR_TAB_INFO) m_SidebarTab = NUM_SIDEBAR_TABS-1;
		}
		else
			m_SidebarTab = (m_SidebarTab+1)%NUM_SIDEBAR_TABS;
	}

	// header
	View.HSplitTop(UI()->GetListHeaderHeight(), &Header, &View);
	float Width = Header.w;
	Header.VSplitLeft(Width*0.30f, &Button, &Header);
	static CButtonContainer s_TabInfo;
	if(DoButton_SpriteID(&s_TabInfo, IMAGE_SIDEBARICONS, m_SidebarTab != SIDEBAR_TAB_INFO ? SPRITE_SIDEBAR_INFO_A : SPRITE_SIDEBAR_INFO_B, m_SidebarTab == SIDEBAR_TAB_INFO, &Button, CUIRect::CORNER_TL, 5.0f, true))
	{
		m_SidebarTab = SIDEBAR_TAB_INFO;
	}
	Header.VSplitLeft(Width*0.30f, &Button, &Header);
	static CButtonContainer s_TabFilter;
	if(DoButton_SpriteID(&s_TabFilter, IMAGE_SIDEBARICONS, m_SidebarTab != SIDEBAR_TAB_FILTER ? SPRITE_SIDEBAR_FILTER_A : SPRITE_SIDEBAR_FILTER_B, m_SidebarTab == SIDEBAR_TAB_FILTER, &Button, 0, 0.0f, true))
	{
		m_SidebarTab = SIDEBAR_TAB_FILTER;
	}
	static CButtonContainer s_TabFriends;
	if(DoButton_SpriteID(&s_TabFriends, IMAGE_SIDEBARICONS, m_SidebarTab != SIDEBAR_TAB_FRIEND ? SPRITE_SIDEBAR_FRIEND_A : SPRITE_SIDEBAR_FRIEND_B, m_SidebarTab == SIDEBAR_TAB_FRIEND, &Header, CUIRect::CORNER_TR, 5.0f, true))
	{
		m_SidebarTab = SIDEBAR_TAB_FRIEND;
	}

	// tabs
	switch(m_SidebarTab)
	{
	case SIDEBAR_TAB_INFO:
		RenderServerbrowserInfoTab(View);
		break;
	case SIDEBAR_TAB_FILTER:
		RenderServerbrowserFilterTab(View);
		break;
	case SIDEBAR_TAB_FRIEND:
		RenderServerbrowserFriendTab(View);
	}
}

void CMenus::RenderServerbrowserFriendTab(CUIRect View)
{
	CUIRect Button, Icon, Label, Rect;
	CUIRect BottomArea;
	const float FontSize = 10.0f;
	static bool s_ListExtended[NUM_FRIEND_TYPES] = { 1, 1, 0 };
	static vec3 s_ListColor[NUM_FRIEND_TYPES] = { vec3(0.5f, 1.0f, 0.5f), vec3(0.4f, 0.4f, 1.0f), vec3(1.0f, 0.5f, 0.5f) };
	const float HeaderHeight = UI()->GetListHeaderHeight();
	const float SpacingH = 2.0f;

	View.HSplitBottom(3*HeaderHeight+2*SpacingH, &View, &BottomArea);

	// calculate friends
	// todo: optimize this
	m_pDeleteFriend = 0;
	m_lFriendList[FRIEND_PLAYER_ON].clear();
	m_lFriendList[FRIEND_CLAN_ON].clear();
	m_lFriendList[FRIEND_OFF].clear();
	for(int f = 0; f < m_pClient->Friends()->NumFriends(); ++f)
	{
		const CContactInfo *pFriendInfo = m_pClient->Friends()->GetFriend(f);
		CFriendItem FriendItem;
		FriendItem.m_pServerInfo = 0;
		str_copy(FriendItem.m_aName, pFriendInfo->m_aName, sizeof(FriendItem.m_aName));
		str_copy(FriendItem.m_aClan, pFriendInfo->m_aClan, sizeof(FriendItem.m_aClan));
		FriendItem.m_FriendState = pFriendInfo->m_aName[0] ? CContactInfo::CONTACT_PLAYER : CContactInfo::CONTACT_CLAN;
		FriendItem.m_IsPlayer = false;
		m_lFriendList[FRIEND_OFF].add(FriendItem);
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

			m_lFriendList[pEntry->m_aClients[j].m_FriendState == CContactInfo::CONTACT_PLAYER ? FRIEND_PLAYER_ON : FRIEND_CLAN_ON].add(FriendItem);

			for(int f = 0; f < m_lFriendList[FRIEND_OFF].size(); ++f)
			{
				if((!m_lFriendList[FRIEND_OFF][f].m_aName[0] || !str_comp(m_lFriendList[FRIEND_OFF][f].m_aName, pEntry->m_aClients[j].m_aName))
					&& !str_comp(m_lFriendList[FRIEND_OFF][f].m_aClan, pEntry->m_aClients[j].m_aClan))
				{
					m_lFriendList[FRIEND_OFF].remove_index(f--);
				}
			}
		}
	}

	// scrollbar
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarWidth = 14;
	ScrollParams.m_ScrollbarMargin = 5;
	ScrollParams.m_ScrollUnit = 40.0f; // various sized content, 40 units per scroll
	s_ScrollRegion.Begin(&View, &ScrollOffset, &ScrollParams);
	View.y += ScrollOffset.y;

	// show lists
	// only ~10 buttons will be displayed at once, a sliding window of 20 buttons ought to be enough
	static char s_aFriendButtons[20];
	static char s_aFriendDeleteButtons[20];
	int ButtonId = 0;
	for(int i = 0; i < NUM_FRIEND_TYPES; ++i)
	{
		CUIRect Header;
		char aBuf[64] = { 0 };
		View.HSplitTop(HeaderHeight, &Header, &View);
		s_ScrollRegion.AddRect(Header);
		if(s_ListExtended[i])
		{
			// entries
			for(int f = 0; f < m_lFriendList[i].size(); ++f, ++ButtonId)
			{
				View.HSplitTop((i == FRIEND_OFF ? 8.0f : 20.0f) + HeaderHeight, &Rect, &View);
				s_ScrollRegion.AddRect(Rect);
				if(f < m_lFriendList[i].size()-1)
				{
					CUIRect Space;
					View.HSplitTop(SpacingH, &Space, &View);
					s_ScrollRegion.AddRect(Space);
				}
				if(s_ScrollRegion.IsRectClipped(Rect))
					continue;

				const bool Inside = UI()->MouseHovered(&Rect);
				bool ButtonResult = UI()->DoButtonLogic(&(s_aFriendButtons[ButtonId%20]), &Rect);
				if(m_lFriendList[i][f].m_pServerInfo)
					UI()->DoTooltip(&(s_aFriendButtons[ButtonId%20]), &Rect, Localize("Double click to join your friend."));
				Rect.Draw(vec4(s_ListColor[i].r, s_ListColor[i].g, s_ListColor[i].b, Inside ? 0.5f : 0.3f));
				Rect.Margin(2.0f, &Rect);
				Rect.VSplitRight(50.0f, &Rect, &Icon);

				Rect.HSplitTop(20.0f, &Button, 0);
				// name
				Rect.HSplitTop(10.0f, &Button, &Rect);
				Button.VSplitLeft(2.0f, 0, &Button);
				UI()->DoLabel(&Button, m_lFriendList[i][f].m_aName, FontSize - 2, TEXTALIGN_LEFT);
				// clan
				Rect.HSplitTop(10.0f, &Button, &Rect);
				Button.VSplitLeft(2.0f, 0, &Button);
				UI()->DoLabel(&Button, m_lFriendList[i][f].m_aClan, FontSize - 2, TEXTALIGN_LEFT);
				// info
				if(m_lFriendList[i][f].m_pServerInfo)
				{
					Rect.HSplitTop(HeaderHeight, &Button, &Rect);
					Button.VSplitLeft(2.0f, 0, &Button);
					if(m_lFriendList[i][f].m_IsPlayer)
						str_format(aBuf, sizeof(aBuf), Localize("Playing '%s' on '%s'", "Playing '(gametype)' on '(map)'"), m_lFriendList[i][f].m_pServerInfo->m_aGameType, m_lFriendList[i][f].m_pServerInfo->m_aMap);
					else
						str_format(aBuf, sizeof(aBuf), Localize("Watching '%s' on '%s'", "Watching '(gametype)' on '(map)'"), m_lFriendList[i][f].m_pServerInfo->m_aGameType, m_lFriendList[i][f].m_pServerInfo->m_aMap);
					UI()->DoLabel(&Button, aBuf, FontSize - 2, TEXTALIGN_ML);
				}
				// delete button
				Icon.HSplitTop(14.0f, &Rect, 0);
				Rect.VSplitRight(12.0f, 0, &Icon);
				Icon.HMargin((Icon.h - Icon.w) / 2, &Icon);
				DoIcon(IMAGE_TOOLICONS, UI()->MouseHovered(&Icon) ? SPRITE_TOOL_X_A : SPRITE_TOOL_X_B, &Icon);
				if(UI()->DoButtonLogic(&(s_aFriendDeleteButtons[ButtonId%20]), &Icon))
				{
					m_pDeleteFriend = &m_lFriendList[i][f];
					ButtonResult = false;
				}
				UI()->DoTooltip(&(s_aFriendDeleteButtons[ButtonId%20]), &Icon, Localize("Click to delete this friend."));
				// handle click and double click on item
				if(ButtonResult && m_lFriendList[i][f].m_pServerInfo)
				{
					SetServerBrowserAddress(m_lFriendList[i][f].m_pServerInfo->m_aAddress);
					m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND | ADDR_SELECTION_REVEAL;
					if(Input()->MouseDoubleClick())
					{
						Client()->Connect(GetServerBrowserAddress());
					}
				}
			}
		}

		{
			CUIRect Space;
			View.HSplitTop(SpacingH, &Space, &View);
			s_ScrollRegion.AddRect(Space);
		}

		// header
		Header.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
		Header.VSplitLeft(Header.h, &Icon, &Label);
		vec4 Color = UI()->MouseHovered(&Header) ? vec4(1.0f, 1.0f, 1.0f, 1.0f) : vec4(0.6f, 0.6f, 0.6f, 1.0f);
		DoIcon(IMAGE_MENUICONS, s_ListExtended[i] ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED, &Icon, &Color);
		int ListSize = m_lFriendList[i].size();
		switch(i)
		{
		case 0: str_format(aBuf, sizeof(aBuf), Localize("Online players (%d)"), ListSize); break;
		case 1: str_format(aBuf, sizeof(aBuf), Localize("Online clanmates (%d)"), ListSize); break;
		case 2: str_format(aBuf, sizeof(aBuf), Localize("Offline (%d)", "friends (server browser)"), ListSize); break;
		}
		UI()->DoLabel(&Label, aBuf, FontSize, TEXTALIGN_ML);
		static int s_HeaderButton[NUM_FRIEND_TYPES] = { 0 };
		if(UI()->DoButtonLogic(&s_HeaderButton[i], &Header))
		{
			s_ListExtended[i] ^= 1;
		}
	}
	s_ScrollRegion.End();

	// add friend
	BottomArea.HSplitTop(HeaderHeight, &Button, &BottomArea);
	BottomArea.HSplitTop(SpacingH, 0, &BottomArea);
	Button.VSplitLeft(50.0f, &Label, &Button);
	UI()->DoLabel(&Label, Localize("Name"), FontSize, TEXTALIGN_ML);
	static char s_aName[MAX_NAME_ARRAY_SIZE] = { 0 };
	static CLineInput s_NameInput(s_aName, sizeof(s_aName), MAX_NAME_LENGTH);
	UI()->DoEditBox(&s_NameInput, &Button, Button.h*CUI::ms_FontmodHeight*0.8f);

	BottomArea.HSplitTop(HeaderHeight, &Button, &BottomArea);
	BottomArea.HSplitTop(SpacingH, 0, &BottomArea);
	Button.VSplitLeft(50.0f, &Label, &Button);
	UI()->DoLabel(&Label, Localize("Clan"), FontSize, TEXTALIGN_ML);
	static char s_aClan[MAX_CLAN_ARRAY_SIZE] = { 0 };
	static CLineInput s_ClanInput(s_aClan, sizeof(s_aClan), MAX_CLAN_LENGTH);
	UI()->DoEditBox(&s_ClanInput, &Button, Button.h*CUI::ms_FontmodHeight*0.8f);

	BottomArea.HSplitTop(HeaderHeight, &Button, &BottomArea);
	Button.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f));
	if(s_aName[0] || s_aClan[0])
		Button.VSplitLeft(Button.h, &Icon, &Label);
	else
		Label = Button;

	const char *pButtonText = (!s_aName[0] && !s_aClan[0]) ? Localize("Add friend/clan") : s_aName[0] ? Localize("Add friend") : Localize("Add clan");
	UI()->DoLabel(&Label, pButtonText, FontSize, TEXTALIGN_MC);
	if(s_aName[0] || s_aClan[0])
		DoIcon(IMAGE_FRIENDICONS, UI()->MouseHovered(&Button) ? SPRITE_FRIEND_PLUS_A : SPRITE_FRIEND_PLUS_B, &Icon);
	static CButtonContainer s_AddFriend;
	if((s_aName[0] || s_aClan[0]) && UI()->DoButtonLogic(&s_AddFriend, &Button))
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
		const bool IsPlayer = m_pDeleteFriend->m_FriendState == CContactInfo::CONTACT_PLAYER;
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf),
			IsPlayer ? Localize("Are you sure that you want to remove the player '%s' from your friends list?") : Localize("Are you sure that you want to remove the clan '%s' from your friends list?"),
			IsPlayer ? m_pDeleteFriend->m_aName : m_pDeleteFriend->m_aClan);
		PopupConfirm(Localize("Remove friend"), aBuf, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmRemoveFriend);
	}
}

void CMenus::PopupConfirmRemoveFriend()
{
	m_pClient->Friends()->RemoveFriend(m_pDeleteFriend->m_FriendState == CContactInfo::CONTACT_PLAYER ? m_pDeleteFriend->m_aName : "", m_pDeleteFriend->m_aClan);
	FriendlistOnUpdate();
	Client()->ServerBrowserUpdate();
	m_pDeleteFriend = 0;
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
	static CLineInput s_FilterInput(s_aFilterName, sizeof(s_aFilterName));
	UI()->DoEditBox(&s_FilterInput, &Icon, FontSize, CUIRect::CORNER_L);
	Button.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), 5.0f, CUIRect::CORNER_R);
	Button.VSplitLeft(Button.h, &Icon, &Label);
	UI()->DoLabel(&Label, Localize("New filter"), FontSize, TEXTALIGN_ML);
	if(s_aFilterName[0])
	{
		DoIcon(IMAGE_FRIENDICONS, UI()->MouseHovered(&Button) ? SPRITE_FRIEND_PLUS_A : SPRITE_FRIEND_PLUS_B, &Icon);
		static CButtonContainer s_AddFilter;
		if(UI()->DoButtonLogic(&s_AddFilter, &Button))
		{
			CBrowserFilter *pSelectedFilter = GetSelectedBrowserFilter();
			if(pSelectedFilter)
				pSelectedFilter->Switch();
			m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_CUSTOM, s_aFilterName, ServerBrowser()));
			m_lFilters[m_lFilters.size()-1].Switch();
			s_aFilterName[0] = 0;
			Client()->ServerBrowserUpdate();
		}
	}

	CBrowserFilter *pFilter = GetSelectedBrowserFilter();
	if(!pFilter)
		return;

	CServerFilterInfo FilterInfo;
	pFilter->GetFilter(&FilterInfo);

	// server filter
	ServerFilter.HSplitTop(UI()->GetListHeaderHeight(), &FilterHeader, &ServerFilter);
	FilterHeader.Draw(vec4(1, 1, 1, 0.25f), 4.0f, CUIRect::CORNER_T);
	ServerFilter.Draw(vec4(0, 0, 0, 0.15f), 4.0f, CUIRect::CORNER_B);
	UI()->DoLabel(&FilterHeader, Localize("Server filter"), FontSize + 2.0f, TEXTALIGN_MC);

	int NewSortHash = FilterInfo.m_SortHash;
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterEmpty = 0;
	if(DoButton_CheckBox(&s_BrFilterEmpty, Localize("Has people playing"), FilterInfo.m_SortHash&IServerBrowser::FILTER_EMPTY, &Button))
		NewSortHash ^= IServerBrowser::FILTER_EMPTY;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterSpectators = 0;
	if(DoButton_CheckBox(&s_BrFilterSpectators, Localize("Count players only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS, &Button))
		NewSortHash ^= IServerBrowser::FILTER_SPECTATORS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterFull = 0;
	if(DoButton_CheckBox(&s_BrFilterFull, Localize("Server not full"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FULL, &Button))
		NewSortHash ^= IServerBrowser::FILTER_FULL;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterFriends = 0;
	if(DoButton_CheckBox(&s_BrFilterFriends, Localize("Show friends only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FRIENDS, &Button))
		NewSortHash ^= IServerBrowser::FILTER_FRIENDS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterBots = 0;
	if(DoButton_CheckBox(&s_BrFilterBots, Localize("Hide bots"), FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS, &Button))
		NewSortHash ^= IServerBrowser::FILTER_BOTS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPw = 0;
	if(DoButton_CheckBox(&s_BrFilterPw, Localize("No password"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PW, &Button))
		NewSortHash ^= IServerBrowser::FILTER_PW;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterCompatversion = 0;
	if(DoButton_CheckBox(&s_BrFilterCompatversion, Localize("Compatible version"), FilterInfo.m_SortHash&IServerBrowser::FILTER_COMPAT_VERSION, &Button))
		NewSortHash ^= IServerBrowser::FILTER_COMPAT_VERSION;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	const bool Locked = pFilter->Custom() == CBrowserFilter::FILTER_STANDARD;
	static int s_BrFilterPure = 0;
	if(DoButton_CheckBox(&s_BrFilterPure, Localize("Standard gametype"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE, &Button, Locked))
		NewSortHash ^= IServerBrowser::FILTER_PURE;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPureMap = 0;
	if(DoButton_CheckBox(&s_BrFilterPureMap, Localize("Standard map"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE_MAP, &Button))
		NewSortHash ^= IServerBrowser::FILTER_PURE_MAP;

	bool UpdateFilter = false;
	if(FilterInfo.m_SortHash != NewSortHash)
	{
		FilterInfo.m_SortHash = NewSortHash;
		UpdateFilter = true;
	}

	// game types filter
	{
		ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		UI()->DoLabel(&Button, Localize("Game types:"), FontSize, TEXTALIGN_LEFT);
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		Button.Draw(vec4(0.0, 0.0, 0.0, 0.25f), 2.0f);

		Button.HMargin(2.0f, &Button);
		UI()->ClipEnable(&Button);

		const float Spacing = 2.0f;
		const float IconWidth = 10.0f;

		float Length = 0.0f;
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			if(!FilterInfo.m_aGametype[i][0])
				break;
			Length += TextRender()->TextWidth(FontSize, FilterInfo.m_aGametype[i], -1) + IconWidth + 2*Spacing;
		}
		static float s_ScrollValue = 0.0f;
		const bool NeedScrollbar = (Button.w - Length) < 0.0f;
		Button.x += minimum(0.0f, Button.w - Length) * s_ScrollValue;
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			if(!FilterInfo.m_aGametype[i][0])
				break;
			const float ItemLength = TextRender()->TextWidth(FontSize, FilterInfo.m_aGametype[i], -1) + IconWidth + Spacing;
			CUIRect FilterItem;
			Button.VSplitLeft(ItemLength, &FilterItem, &Button);
			FilterItem.Draw(FilterInfo.m_aGametypeExclusive[i] ? vec4(0.75f, 0.25f, 0.25f, 0.25f) : vec4(0.25f, 0.75f, 0.25f, 0.25f), 3.0f);
			FilterItem.VSplitLeft(Spacing, 0, &FilterItem);
			UI()->DoLabel(&FilterItem, FilterInfo.m_aGametype[i], FontSize, TEXTALIGN_LEFT);
			FilterItem.VSplitRight(IconWidth, 0, &FilterItem);
			DoIcon(IMAGE_TOOLICONS, UI()->MouseHovered(&FilterItem) ? SPRITE_TOOL_X_A : SPRITE_TOOL_X_B, &FilterItem);
			if(UI()->DoButtonLogic(&FilterInfo.m_aGametype[i], &FilterItem))
			{
				// remove gametype entry
				if((i == CServerFilterInfo::MAX_GAMETYPES - 1) || !FilterInfo.m_aGametype[i + 1][0])
				{
					FilterInfo.m_aGametype[i][0] = 0;
					FilterInfo.m_aGametypeExclusive[i] = false;
				}
				else
				{
					int j = i;
					for(; j < CServerFilterInfo::MAX_GAMETYPES - 1 && FilterInfo.m_aGametype[j + 1][0]; ++j)
					{
						str_copy(FilterInfo.m_aGametype[j], FilterInfo.m_aGametype[j + 1], sizeof(FilterInfo.m_aGametype[j]));
						FilterInfo.m_aGametypeExclusive[j] = FilterInfo.m_aGametypeExclusive[j + 1];
					}
					FilterInfo.m_aGametype[j][0] = 0;
					FilterInfo.m_aGametypeExclusive[j] = false;
				}
				UpdateFilter = true;
			}
			Button.VSplitLeft(Spacing, 0, &Button);
		}

		UI()->ClipDisable();

		if(NeedScrollbar)
		{
			ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
			s_ScrollValue = UI()->DoScrollbarH(&s_ScrollValue, &Button, s_ScrollValue);
		}
		else
			ServerFilter.HSplitTop(4.f, &Button, &ServerFilter); // Leave some space in between edit boxes

		CUIRect ButtonLine;
		ServerFilter.HSplitTop(LineSize, &ButtonLine, &ServerFilter);

		CUIRect EditBox, AddIncButton, AddExlButton, ClearButton;
		ButtonLine.VSplitRight(40.0f, &ButtonLine, &ClearButton);
		ButtonLine.VSplitRight(10.0f, &ButtonLine, 0);
		ButtonLine.VSplitRight(ButtonLine.h, &ButtonLine, &AddExlButton);
		ButtonLine.VSplitRight(ButtonLine.h, &EditBox, &AddIncButton);

		static char s_aGametype[16] = { 0 };
		static CLineInput s_GametypeInput(s_aGametype, sizeof(s_aGametype));
		UI()->DoEditBox(&s_GametypeInput, &EditBox, FontSize, CUIRect::CORNER_L);

		static CButtonContainer s_AddInclusiveGametype;
		if(DoButton_Menu(&s_AddInclusiveGametype, "+", 0, &AddIncButton, 0, 0) && s_aGametype[0])
		{
			for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
			{
				if(!FilterInfo.m_aGametype[i][0])
				{
					str_copy(FilterInfo.m_aGametype[i], s_aGametype, sizeof(FilterInfo.m_aGametype[i]));
					FilterInfo.m_aGametypeExclusive[i] = false;
					UpdateFilter = true;
					s_aGametype[0] = 0;
					break;
				}
			}
		}
		UI()->DoTooltip(&s_AddInclusiveGametype, &AddIncButton, Localize("Include servers with this gametype."));

		static CButtonContainer s_AddExclusiveGametype;
		if(DoButton_Menu(&s_AddExclusiveGametype, "-", 0, &AddExlButton, 0, CUIRect::CORNER_R) && s_aGametype[0])
		{
			for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
			{
				if(!FilterInfo.m_aGametype[i][0])
				{
					str_copy(FilterInfo.m_aGametype[i], s_aGametype, sizeof(FilterInfo.m_aGametype[i]));
					FilterInfo.m_aGametypeExclusive[i] = true;
					UpdateFilter = true;
					s_aGametype[0] = 0;
					break;
				}
			}
		}
		UI()->DoTooltip(&s_AddExclusiveGametype, &AddExlButton, Localize("Exclude servers with this gametype."));

		static CButtonContainer s_ClearGametypes;
		if(DoButton_MenuTabTop(&s_ClearGametypes, Localize("Clear", "clear gametype filters"), false, &ClearButton))
		{
			for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
			{
				FilterInfo.m_aGametype[i][0] = 0;
				FilterInfo.m_aGametypeExclusive[i] = false;
			}
			UpdateFilter = true;
		}

		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	}

	// ping
	ServerFilter.HSplitTop(LineSize - 4.f, &Button, &ServerFilter);
	{
		int Value = FilterInfo.m_Ping, Min = 20, Max = 999;

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s %d", Localize("Maximum ping:"), Value);
		UI()->DoLabel(&Button, aBuf, FontSize, TEXTALIGN_LEFT);

		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);

		Button.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
		Button.VMargin(4.0f, &Button);
		static int s_BrFilterPing = 0;
		Value = LogarithmicScrollbarScale.ToAbsolute(UI()->DoScrollbarH(&s_BrFilterPing, &Button, LogarithmicScrollbarScale.ToRelative(Value, Min, Max)), Min, Max);
		if(Value != FilterInfo.m_Ping)
		{
			FilterInfo.m_Ping = Value;
			UpdateFilter = true;
		}
	}

	// server address
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	UI()->DoLabel(&Button, Localize("Server address:"), FontSize, TEXTALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	static char s_aAddressFilter[sizeof(FilterInfo.m_aAddress)];
	static CLineInput s_AddressInput(s_aAddressFilter, sizeof(s_aAddressFilter));
	if(UI()->DoEditBox(&s_AddressInput, &Button, FontSize))
	{
		str_copy(FilterInfo.m_aAddress, s_aAddressFilter, sizeof(FilterInfo.m_aAddress));
		UpdateFilter = true;
	}

	// player country
	{
		CUIRect Rect;
		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		UI()->DoLabel(&Button, Localize("Flag:"), FontSize, TEXTALIGN_LEFT);
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
		if((FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY) && UI()->DoButtonLogic(&s_BrFilterCountryIndex, &Rect))
			PopupCountry(FilterInfo.m_Country, &CMenus::PopupConfirmCountryFilter);
	}

	// level
	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(LineSize + 2, &Button, &ServerFilter);
	UI()->DoLabel(&Button, Localize("Difficulty:"), FontSize, TEXTALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	Button.y -= 2.0f;
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton1;
	if(DoButton_SpriteID(&s_LevelButton1, IMAGE_LEVELICONS, FilterInfo.IsLevelFiltered(CServerInfo::LEVEL_CASUAL) ? SPRITE_LEVEL_A_B : SPRITE_LEVEL_A_ON, false, &Icon, CUIRect::CORNER_L, 5.0f, true))
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
	if(DoButton_SpriteID(&s_LevelButton3, IMAGE_LEVELICONS, FilterInfo.IsLevelFiltered(CServerInfo::LEVEL_COMPETITIVE) ? SPRITE_LEVEL_C_B : SPRITE_LEVEL_C_ON, false, &Icon, CUIRect::CORNER_R, 5.0f, true))
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
	const CServerInfo *pInfo = GetSelectedServerInfo();
	if(pInfo)
	{
		RenderServerbrowserServerDetail(View, pInfo);
	}
}

void CMenus::RenderDetailInfo(CUIRect View, const CServerInfo *pInfo, const vec4 &TextColor, const vec4 &TextOutlineColor)
{
	const float FontSize = 10.0f;
	const float RowHeight = 15.0f;

	TextRender()->TextColor(TextColor);
	TextRender()->TextSecondaryColor(TextOutlineColor);

	CUIRect ServerHeader;
	View.HSplitTop(UI()->GetListHeaderHeight(), &ServerHeader, &View);
	ServerHeader.Draw(vec4(1, 1, 1, 0.25f), 5.0f, CUIRect::CORNER_T);
	View.Draw(vec4(0, 0, 0, 0.15f), 5.0f, CUIRect::CORNER_B);
	UI()->DoLabel(&ServerHeader, Localize("Server details"), FontSize + 2.0f, TEXTALIGN_MC);

	if(!pInfo)
		return;

	CUIRect Row;
	// Localize("Map:"); Localize("Game type:"); Localize("Version:"); Localize("Difficulty:"); Localize("Casual", "Server difficulty"); Localize("Normal", "Server difficulty"); Localize("Competitive", "Server difficulty");
	static CLocConstString s_aLabels[] = {
		"Map:",
		"Game type:",
		"Version:",
		"Difficulty:" };

	CUIRect LeftColumn, RightColumn;
	View.VMargin(2.0f, &View);
	View.VSplitLeft(70.0f, &LeftColumn, &RightColumn);

	for(unsigned int i = 0; i < sizeof(s_aLabels) / sizeof(s_aLabels[0]); i++)
	{
		LeftColumn.HSplitTop(RowHeight, &Row, &LeftColumn);
		UI()->DoLabel(&Row, s_aLabels[i], FontSize, TEXTALIGN_LEFT, Row.w, false);
	}

	// map
	RightColumn.HSplitTop(RowHeight, &Row, &RightColumn);
	UI()->DoLabel(&Row, pInfo->m_aMap, FontSize, TEXTALIGN_LEFT, Row.w, false);

	// game type
	RightColumn.HSplitTop(RowHeight, &Row, &RightColumn);
	CUIRect Icon;
	Row.VSplitLeft(Row.h, &Icon, &Row);
	Icon.y -= 2.0f;
	DoGameIcon(pInfo->m_aGameType, &Icon);
	UI()->DoLabel(&Row, pInfo->m_aGameType, FontSize, TEXTALIGN_LEFT, Row.w, false);

	// version
	RightColumn.HSplitTop(RowHeight, &Row, &RightColumn);
	UI()->DoLabel(&Row, pInfo->m_aVersion, FontSize, TEXTALIGN_LEFT, Row.w, false);

	// difficulty
	RightColumn.HSplitTop(RowHeight, &Row, &RightColumn);
	Row.VSplitLeft(Row.h, &Icon, &Row);
	Icon.y -= 2.0f;
	DoIcon(IMAGE_LEVELICONS, s_aDifficultySpriteIds[pInfo->m_ServerLevel], &Icon);
	UI()->DoLabel(&Row, s_aDifficultyLabels[pInfo->m_ServerLevel], FontSize, TEXTALIGN_LEFT, Row.w, false);
}

void CMenus::RenderDetailScoreboard(CUIRect View, const CServerInfo *pInfo, int RowCount, const vec4 &TextColor, const vec4 &TextOutlineColor)
{
	View.Draw(vec4(0, 0, 0, 0.15f), 5.0f, RowCount > 0 ? CUIRect::CORNER_B|CUIRect::CORNER_TL : CUIRect::CORNER_B);
	View.Margin(2.0f, &View);

	if(!pInfo)
		return;

	CBrowserFilter *pFilter = GetSelectedBrowserFilter();
	CServerFilterInfo FilterInfo;
	if(pFilter)
		pFilter->GetFilter(&FilterInfo);

	TextRender()->TextColor(TextColor);
	TextRender()->TextSecondaryColor(TextOutlineColor);

	static const CServerInfo *s_pLastInfo = pInfo;
	const bool ResetScroll = s_pLastInfo != pInfo;
	s_pLastInfo = pInfo;

	const float RowWidth = (RowCount == 0) ? View.w : (View.w * 0.25f);
	const float FontSize = Config()->m_UiWideview ? 8.0f : 7.0f;
	const vec4 HighlightColor = vec4(TextHighlightColor.r, TextHighlightColor.g, TextHighlightColor.b, TextColor.a);
	float LineHeight = 20.0f;

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollbarWidth = 5;
	ScrollParams.m_ScrollbarMargin = 1;
	ScrollParams.m_ScrollUnit = 60.0f; // 3 players per scroll
	s_ScrollRegion.Begin(&View, &ScrollOffset, &ScrollParams);
	View.y += ScrollOffset.y;
	if(RowCount > 0)
	{
		const float Width = RowWidth * ((pInfo->m_NumClients+RowCount-1) / RowCount);
		static float s_ScrollValue = 0.0f;
		if(ResetScroll)
		{
			s_ScrollValue = 0.0f;
		}
		if(Width > View.w)
		{
			CUIRect Scroll;
			View.HSplitBottom(14.0f, &View, &Scroll);
			Scroll.VMargin(5.0f, &Scroll);
			s_ScrollValue = UI()->DoScrollbarH(&s_ScrollValue, &Scroll, s_ScrollValue);
			View.x += (View.w - Width) * s_ScrollValue;
			LineHeight = 0.25f*View.h;
		}
	}

	CUIRect Row = View;

	int Count = 0;
	for(int i = 0; i < pInfo->m_NumClients; i++)
	{
		if(pFilter && (FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS) && (pInfo->m_aClients[i].m_PlayerType&CServerInfo::CClient::PLAYERFLAG_BOT))
			continue;

		CUIRect Name, Clan, Score, Flag, Icon;

		if(RowCount > 0 && Count % RowCount == 0)
			View.VSplitLeft(RowWidth, &Row, &View);

		Row.HSplitTop(LineHeight, &Name, &Row);
		s_ScrollRegion.AddRect(Name);
		if(Count == 0 && ResetScroll)
			s_ScrollRegion.ScrollHere(CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);

		Count++;
		if(s_ScrollRegion.IsRectClipped(Name))
			continue;

		Name.Draw(vec4(1.0f, 1.0f, 1.0f, (Count % 2 ? 1 : 2)*0.05f));

		// friend
		if(UI()->DoButtonLogic(&pInfo->m_aClients[i], &Name))
		{
			if(pInfo->m_aClients[i].m_FriendState == CContactInfo::CONTACT_PLAYER)
				m_pClient->Friends()->RemoveFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
			else
				m_pClient->Friends()->AddFriend(pInfo->m_aClients[i].m_aName, pInfo->m_aClients[i].m_aClan);
			FriendlistOnUpdate();
			Client()->ServerBrowserUpdate();
		}
		UI()->DoTooltip(&pInfo->m_aClients[i], &Name, pInfo->m_aClients[i].m_FriendState == CContactInfo::CONTACT_PLAYER
			? Localize("Click to remove the player from your friends.") : Localize("Click to add the player to your friends."));
		Name.VSplitLeft(Name.h-8.0f, &Icon, &Name);
		Icon.HMargin(4.0f, &Icon);
		if(pInfo->m_aClients[i].m_FriendState != CContactInfo::CONTACT_NO)
			DoIcon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART_A, &Icon);

		Name.VSplitLeft(2.0f, 0, &Name);
		Name.VSplitLeft(25.0f, &Score, &Name);
		Name.VSplitRight(2*(Name.h-8.0f), &Name, &Flag);
		Name.HSplitTop(LineHeight*0.5f, &Name, &Clan);

		// score
		if(!(pInfo->m_aClients[i].m_PlayerType&CServerInfo::CClient::PLAYERFLAG_SPEC))
		{
			Score.y += (Score.h - FontSize/CUI::ms_FontmodHeight)/2.0f;
			char aTemp[16];
			FormatScore(aTemp, sizeof(aTemp), pInfo->m_Flags&IServerBrowser::FLAG_TIMESCORE, &pInfo->m_aClients[i]);
			UI()->DoLabel(&Score, aTemp, FontSize, TEXTALIGN_LEFT);
		}

		// name
		UI()->DoLabelHighlighted(&Name, pInfo->m_aClients[i].m_aName, Config()->m_BrFilterString, FontSize, TextColor, HighlightColor);

		// clan
		UI()->DoLabelHighlighted(&Clan, pInfo->m_aClients[i].m_aClan, Config()->m_BrFilterString, FontSize, TextColor, HighlightColor);

		// flag
		Flag.HMargin(4.0f, &Flag);
		Flag.w = Flag.h*2;
		vec4 FlagColor(1.0f, 1.0f, 1.0f, 0.5f);
		m_pClient->m_pCountryFlags->Render(pInfo->m_aClients[i].m_Country, &FlagColor, Flag.x, Flag.y, Flag.w, Flag.h);
	}

	s_ScrollRegion.End();
}

void CMenus::RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo)
{
	CUIRect ServerHeader, ServerDetails, ServerScoreboard;

	// split off a piece to use for scoreboard
	View.HSplitTop(80.0f, &ServerDetails, &ServerScoreboard);

	// server details
	RenderDetailInfo(ServerDetails, pInfo, CUI::ms_DefaultTextColor, CUI::ms_DefaultTextOutlineColor);

	// server scoreboard
	ServerScoreboard.HSplitTop(UI()->GetListHeaderHeight(), &ServerHeader, &ServerScoreboard);
	ServerHeader.Draw(vec4(1, 1, 1, 0.25f), 4.0f, CUIRect::CORNER_T);
	UI()->DoLabel(&ServerHeader, Localize("Scoreboard"), 12.0f, TEXTALIGN_MC);
	RenderDetailScoreboard(ServerScoreboard, pInfo, 0, CUI::ms_DefaultTextColor, CUI::ms_DefaultTextOutlineColor);
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
	RenderBackgroundShadow(&MainView, true);

	// back to main menu
	CUIRect Button;
	MainView.HSplitTop(25.0f, &MainView, 0);
	MainView.VSplitLeft(ButtonWidth, &Button, &MainView);
	static CButtonContainer s_RefreshButton;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Button) || (UI()->KeyPress(KEY_R) && (Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))))
	{
		if(m_MenuPage == PAGE_INTERNET)
			ServerBrowser()->Refresh(IServerBrowser::REFRESHFLAG_INTERNET);
		else if(m_MenuPage == PAGE_LAN)
			ServerBrowser()->Refresh(IServerBrowser::REFRESHFLAG_LAN);
	}

	MainView.VSplitLeft(Spacing, 0, &MainView); // little space
	MainView.VSplitLeft(ButtonWidth, &Button, &MainView);
	static CButtonContainer s_JoinButton;
	if(DoButton_Menu(&s_JoinButton, Localize("Connect"), 0, &Button) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
	{
		Client()->Connect(GetServerBrowserAddress());
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
	char aBuf[IO_MAX_PATH_LENGTH];
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
	mem_free(Info.m_pData);
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
	if(DoButton_SpriteID(&s_SidebarButton, IMAGE_ARROWICONS, m_SidebarActive?SPRITE_ARROW_RIGHT_A:SPRITE_ARROW_LEFT_A, false, &SidebarButton, CUIRect::CORNER_R, 5.0f, true))
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
}

void CMenus::UpdateServerBrowserAddress()
{
	const CServerInfo *pItem = GetSelectedServerInfo();
	SetServerBrowserAddress(pItem ? pItem->m_aAddress : "");
}

const char *CMenus::GetServerBrowserAddress()
{
	const int Type = ServerBrowser()->GetType();
	if(Type == IServerBrowser::TYPE_INTERNET)
		return Config()->m_UiServerAddress;
	else if(Type == IServerBrowser::TYPE_LAN)
		return Config()->m_UiServerAddressLan;
	return 0;
}

void CMenus::SetServerBrowserAddress(const char *pAddress)
{
	const int Type = ServerBrowser()->GetType();
	if(Type == IServerBrowser::TYPE_INTERNET)
		str_copy(Config()->m_UiServerAddress, pAddress, sizeof(Config()->m_UiServerAddress));
	else if(Type == IServerBrowser::TYPE_LAN)
		str_copy(Config()->m_UiServerAddressLan, pAddress, sizeof(Config()->m_UiServerAddressLan));
}

void CMenus::ServerBrowserFilterOnUpdate()
{
	m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND;
}

void CMenus::ServerBrowserSortingOnUpdate()
{
	m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND;
}

void CMenus::ConchainConnect(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	if(pResult->NumArguments() == 1)
	{
		CMenus *pMenus = (CMenus*)pUserData;
		pMenus->SetServerBrowserAddress(pResult->GetString(0));
		pMenus->m_AddressSelection |= ADDR_SELECTION_CHANGE | ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND;
	}
	pfnCallback(pResult, pCallbackUserData);
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

void CMenus::ConchainServerbrowserSortingUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CMenus *pMenus = (CMenus*)pUserData;
	pMenus->ServerBrowserSortingOnUpdate();
}
