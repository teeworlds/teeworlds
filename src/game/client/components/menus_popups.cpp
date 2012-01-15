/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/tl/array.h>

#include <engine/shared/config.h>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/serverbrowser.h>

#include "countryflags.h"
#include "menus.h"


// popup menu handling
static struct
{
	CUIRect m_Rect;
	void *m_pId;
	int (*m_pfnFunc)(CMenus *pMenu, CUIRect Rect);
	int m_IsMenu;
	void *m_pExtra;
} s_Popups[8];

static int g_NumPopups = 0;

void CMenus::InvokePopupMenu(void *pID, int Flags, float x, float y, float Width, float Height, int (*pfnFunc)(CMenus *pMenu, CUIRect Rect), void *pExtra)
{
	if(x + Width > UI()->Screen()->w)
		x -= Width;
	if(y + Height > UI()->Screen()->h)
		y -= Height;
	s_Popups[g_NumPopups].m_pId = pID;
	s_Popups[g_NumPopups].m_IsMenu = Flags;
	s_Popups[g_NumPopups].m_Rect.x = x;
	s_Popups[g_NumPopups].m_Rect.y = y;
	s_Popups[g_NumPopups].m_Rect.w = Width;
	s_Popups[g_NumPopups].m_Rect.h = Height;
	s_Popups[g_NumPopups].m_pfnFunc = pfnFunc;
	s_Popups[g_NumPopups].m_pExtra = pExtra;
	g_NumPopups++;
}

void CMenus::DoPopupMenu()
{
	for(int i = 0; i < g_NumPopups; i++)
	{
		bool Inside = UI()->MouseInside(&s_Popups[i].m_Rect);
		UI()->SetHotItem(&s_Popups[i].m_pId);

		if(UI()->ActiveItem() == &s_Popups[i].m_pId)
		{
			if(!UI()->MouseButton(0))
			{
				if(!Inside)
					g_NumPopups--;
				UI()->SetActiveItem(0);
			}
		}
		else if(UI()->HotItem() == &s_Popups[i].m_pId)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(&s_Popups[i].m_pId);
		}

		int Corners = CUI::CORNER_ALL;
		if(s_Popups[i].m_IsMenu)
			Corners = CUI::CORNER_R|CUI::CORNER_B;

		CUIRect r = s_Popups[i].m_Rect;
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f,0.75f), Corners, 3.0f);
		r.Margin(1.0f, &r);
		RenderTools()->DrawUIRect(&r, vec4(0,0,0,0.75f), Corners, 3.0f);
		r.Margin(4.0f, &r);

		if(s_Popups[i].m_pfnFunc(this, r))
			g_NumPopups--;

		if(Input()->KeyDown(KEY_ESCAPE))
			g_NumPopups--;
	}
}

int CMenus::PopupFilter(CMenus *pMenus, CUIRect View)
{
	CUIRect ServerFilter = View, FilterHeader;
	const float FontSize = 12.0f;

	// slected filter
	CBrowserFilter *pFilter = &pMenus->m_lFilters[pMenus->m_SelectedFilter];
	int SortHash = 0;
	int Ping = 0;
	int Country = 0;
	char aGametype[32];
	char aServerAddress[16];
	pFilter->GetFilter(&SortHash, &Ping, &Country, aGametype, aServerAddress);

	// server filter
	ServerFilter.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFilter);
	pMenus->RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	pMenus->RenderTools()->DrawUIRect(&ServerFilter, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	pMenus->UI()->DoLabelScaled(&FilterHeader, Localize("Server filter"), FontSize+2.0f, 0);
	CUIRect Button;

	ServerFilter.VSplitLeft(5.0f, 0, &ServerFilter);
	ServerFilter.Margin(3.0f, &ServerFilter);
	ServerFilter.VMargin(5.0f, &ServerFilter);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterEmpty = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterEmpty, Localize("Has people playing"), SortHash&IServerBrowser::FILTER_EMPTY, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_EMPTY, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterSpectators = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterSpectators, Localize("Count players only"), SortHash&IServerBrowser::FILTER_SPECTATORS, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_SPECTATORS, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterFull = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterFull, Localize("Server not full"), SortHash&IServerBrowser::FILTER_FULL, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_FULL, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterFriends = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterFriends, Localize("Show friends only"), SortHash&IServerBrowser::FILTER_FRIENDS, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_FRIENDS, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPw = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPw, Localize("No password"), SortHash&IServerBrowser::FILTER_PW, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_PW, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterCompatversion = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterCompatversion, Localize("Compatible version"), SortHash&IServerBrowser::FILTER_COMPAT_VERSION, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_COMPAT_VERSION, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPure = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPure, Localize("Standard gametype"), SortHash&IServerBrowser::FILTER_PURE, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_PURE, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPureMap = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPureMap, Localize("Standard map"), SortHash&IServerBrowser::FILTER_PURE_MAP, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_PURE_MAP, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterGametypeStrict = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterGametypeStrict, Localize("Strict gametype filter"), SortHash&IServerBrowser::FILTER_GAMETYPE_STRICT, &Button))
		pFilter->SetFilter(SortHash^IServerBrowser::FILTER_GAMETYPE_STRICT, Ping, Country, aGametype, aServerAddress);

	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	pMenus->UI()->DoLabelScaled(&Button, Localize("Game types:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static float Offset = 0.0f;
	static int s_BrFilterGametype = 0;
	if(pMenus->DoEditBox(&s_BrFilterGametype, &Button, aGametype, sizeof(aGametype), FontSize, &Offset))
		pFilter->SetFilter(SortHash, Ping, Country, aGametype, aServerAddress);

	{
		ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
		CUIRect EditBox;
		Button.VSplitRight(60.0f, &Button, &EditBox);

		pMenus->UI()->DoLabelScaled(&Button, Localize("Maximum ping:"), FontSize, -1);

		char aBuf[5];
		str_format(aBuf, sizeof(aBuf), "%d", Ping);
		static float Offset = 0.0f;
		static int s_BrFilterPing = 0;
		pMenus->DoEditBox(&s_BrFilterPing, &EditBox, aBuf, sizeof(aBuf), FontSize, &Offset);
		int NewPing = clamp(str_toint(aBuf), 0, 999);
		if(NewPing != Ping)
			pFilter->SetFilter(SortHash, NewPing, Country, aGametype, aServerAddress);
	}

	// server address
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	pMenus->UI()->DoLabelScaled(&Button, Localize("Server address:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	static float OffsetAddr = 0.0f;
	static int s_BrFilterServerAddress = 0;
	if(pMenus->DoEditBox(&s_BrFilterServerAddress, &Button, aServerAddress, sizeof(aServerAddress), FontSize, &OffsetAddr))
		pFilter->SetFilter(SortHash, Ping, Country, aGametype, aServerAddress);

	// player country
	{
		CUIRect Rect;
		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(26.0f, &Button, &ServerFilter);
		Button.VSplitRight(60.0f, &Button, &Rect);
		Button.HMargin(3.0f, &Button);
		static int s_BrFilterCountry = 0;
		if(pMenus->DoButton_CheckBox(&s_BrFilterCountry, Localize("Player country:"), SortHash&IServerBrowser::FILTER_COUNTRY, &Button))
			pFilter->SetFilter(SortHash^IServerBrowser::FILTER_COUNTRY, Ping, Country, aGametype, aServerAddress);

		float OldWidth = Rect.w;
		Rect.w = Rect.h*2;
		Rect.x += (OldWidth-Rect.w)/2.0f;
		vec4 Color(1.0f, 1.0f, 1.0f, SortHash^IServerBrowser::FILTER_COUNTRY?1.0f: 0.5f);
		pMenus->m_pClient->m_pCountryFlags->Render(Country, &Color, Rect.x, Rect.y, Rect.w, Rect.h);

		static int s_BrFilterCountryIndex = 0;
		if(SortHash^IServerBrowser::FILTER_COUNTRY && pMenus->UI()->DoButtonLogic(&s_BrFilterCountryIndex, "", 0, &Rect))
			pMenus->m_Popup = POPUP_COUNTRY;
	}

	return 0;
}
