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
} s_Popups;

void CMenus::InvokePopupMenu(void *pID, int Flags, float x, float y, float Width, float Height, int (*pfnFunc)(CMenus *pMenu, CUIRect Rect), void *pExtra)
{
	if(m_PopupActive)
		return;
	if(x + Width > UI()->Screen()->w)
		x = UI()->Screen()->w - Width;
	if(y + Height > UI()->Screen()->h)
		y = UI()->Screen()->h - Height;
	s_Popups.m_pId = pID;
	s_Popups.m_IsMenu = Flags;
	s_Popups.m_Rect.x = x;
	s_Popups.m_Rect.y = y;
	s_Popups.m_Rect.w = Width;
	s_Popups.m_Rect.h = Height;
	s_Popups.m_pfnFunc = pfnFunc;
	s_Popups.m_pExtra = pExtra;
	m_PopupActive = true;
}

void CMenus::DoPopupMenu()
{
	if(m_PopupActive)
	{
		bool Inside = UI()->MouseInside(&s_Popups.m_Rect);
		UI()->SetHotItem(&s_Popups.m_pId);

		if(UI()->CheckActiveItem(&s_Popups.m_pId))
		{
			if(!UI()->MouseButton(0))
			{
				if(!Inside)
				m_PopupActive = false;
				UI()->SetActiveItem(0);
			}
		}
		else if(UI()->HotItem() == &s_Popups.m_pId)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(&s_Popups.m_pId);
		}

		int Corners = CUI::CORNER_ALL;
		if(s_Popups.m_IsMenu)
			Corners = CUI::CORNER_R|CUI::CORNER_B;

		CUIRect r = s_Popups.m_Rect;
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f,0.75f), Corners, 3.0f);
		r.Margin(1.0f, &r);
		RenderTools()->DrawUIRect(&r, vec4(0,0,0,0.75f), Corners, 3.0f);
		r.Margin(4.0f, &r);

		if(s_Popups.m_pfnFunc(this, r))
			m_PopupActive = false;

		if(Input()->KeyPress(KEY_ESCAPE))
			m_PopupActive = false;
	}
}

int CMenus::PopupFilter(CMenus *pMenus, CUIRect View)
{
	CUIRect ServerFilter = View, FilterHeader;
	const float FontSize = 12.0f;

	// slected filter
	CBrowserFilter *pFilter = &pMenus->m_lFilters[pMenus->m_SelectedFilter];
	CServerFilterInfo FilterInfo;
	pFilter->GetFilter(&FilterInfo);

	// server filter
	ServerFilter.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFilter);
	pMenus->RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	pMenus->RenderTools()->DrawUIRect(&ServerFilter, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	pMenus->UI()->DoLabelScaled(&FilterHeader, Localize("Server filter"), FontSize+2.0f, CUI::ALIGN_CENTER);
	CUIRect Button;

	ServerFilter.VSplitLeft(5.0f, 0, &ServerFilter);
	ServerFilter.Margin(3.0f, &ServerFilter);
	ServerFilter.VMargin(5.0f, &ServerFilter);

	int NewSortHash = FilterInfo.m_SortHash;
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterEmpty = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterEmpty, Localize("Has people playing"), FilterInfo.m_SortHash&IServerBrowser::FILTER_EMPTY, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_EMPTY;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterSpectators = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterSpectators, Localize("Count players only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_SPECTATORS;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterFull = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterFull, Localize("Server not full"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FULL, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_FULL;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterFriends = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterFriends, Localize("Show friends only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FRIENDS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_FRIENDS;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPw = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPw, Localize("No password"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PW, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PW;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterCompatversion = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterCompatversion, Localize("Compatible version"), FilterInfo.m_SortHash&IServerBrowser::FILTER_COMPAT_VERSION, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_COMPAT_VERSION;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPure = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPure, Localize("Standard gametype"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PURE;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterPureMap = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPureMap, Localize("Standard map"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE_MAP, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PURE_MAP;

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static int s_BrFilterGametypeStrict = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterGametypeStrict, Localize("Strict gametype filter"), FilterInfo.m_SortHash&IServerBrowser::FILTER_GAMETYPE_STRICT, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_GAMETYPE_STRICT;
	
	if(FilterInfo.m_SortHash != NewSortHash)
	{
		FilterInfo.m_SortHash = NewSortHash;
		pFilter->SetFilter(&FilterInfo);
	}

	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	pMenus->UI()->DoLabelScaled(&Button, Localize("Game types:"), FontSize, CUI::ALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static float Offset = 0.0f;
	static int s_BrFilterGametype = 0;
	if(pMenus->DoEditBox(&s_BrFilterGametype, &Button, FilterInfo.m_aGametype, sizeof(FilterInfo.m_aGametype), FontSize, &Offset))
		pFilter->SetFilter(&FilterInfo);

	{
		ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
		CUIRect EditBox;
		Button.VSplitRight(60.0f, &Button, &EditBox);

		pMenus->UI()->DoLabelScaled(&Button, Localize("Maximum ping:"), FontSize, CUI::ALIGN_LEFT);

		char aBuf[5];
		str_format(aBuf, sizeof(aBuf), "%d", FilterInfo.m_Ping);
		static float Offset = 0.0f;
		static int s_BrFilterPing = 0;
		pMenus->DoEditBox(&s_BrFilterPing, &EditBox, aBuf, sizeof(aBuf), FontSize, &Offset);
		int NewPing = clamp(str_toint(aBuf), 0, 999);
		if(NewPing != FilterInfo.m_Ping)
		{
			FilterInfo.m_Ping = NewPing;
			pFilter->SetFilter(&FilterInfo);
		}
	}

	// server address
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	pMenus->UI()->DoLabelScaled(&Button, Localize("Server address:"), FontSize, CUI::ALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	static float OffsetAddr = 0.0f;
	static int s_BrFilterServerAddress = 0;
	if(pMenus->DoEditBox(&s_BrFilterServerAddress, &Button, FilterInfo.m_aAddress, sizeof(FilterInfo.m_aAddress), FontSize, &OffsetAddr))
		pFilter->SetFilter(&FilterInfo);

	// player country
	{
		CUIRect Rect;
		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(26.0f, &Button, &ServerFilter);
		Button.VSplitRight(60.0f, &Button, &Rect);
		Button.HMargin(3.0f, &Button);
		static int s_BrFilterCountry = 0;
		if(pMenus->DoButton_CheckBox(&s_BrFilterCountry, Localize("Player country:"), FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY, &Button))
		{
			FilterInfo.m_SortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_COUNTRY;
			pFilter->SetFilter(&FilterInfo);
		}

		float OldWidth = Rect.w;
		Rect.w = Rect.h*2;
		Rect.x += (OldWidth-Rect.w)/2.0f;
		vec4 Color(1.0f, 1.0f, 1.0f, FilterInfo.m_SortHash^IServerBrowser::FILTER_COUNTRY?1.0f: 0.5f);
		pMenus->m_pClient->m_pCountryFlags->Render(FilterInfo.m_Country, &Color, Rect.x, Rect.y, Rect.w, Rect.h);

		static int s_BrFilterCountryIndex = 0;
		if(FilterInfo.m_SortHash^IServerBrowser::FILTER_COUNTRY && pMenus->UI()->DoButtonLogic(&s_BrFilterCountryIndex, "", 0, &Rect))
			pMenus->m_Popup = POPUP_COUNTRY;
	}

	return 0;
}
