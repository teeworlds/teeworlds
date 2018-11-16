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
#include <engine/textrender.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

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
	CUIRect ServerFilter = View, FilterHeader, Button, Icon, Label;
	const float FontSize = 10.0f;
	const float LineSize = 16.0f;

	// new filter
	ServerFilter.HSplitBottom(LineSize, &ServerFilter, &Button);
	Button.VSplitLeft(60.0f, &Icon, &Button);
	static char s_aFilterName[32] = { 0 };
	static float s_FilterOffset = 0.0f;
	static int s_EditFilter = 0;
	pMenus->DoEditBox(&s_EditFilter, &Icon, s_aFilterName, sizeof(s_aFilterName), FontSize, &s_FilterOffset, false, CUI::CORNER_L);
	pMenus->RenderTools()->DrawUIRect(&Button, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_R, 5.0f);
	Button.VSplitLeft(Button.h, &Icon, &Label);
	Label.HMargin(2.0f, &Label);
	pMenus->UI()->DoLabelScaled(&Label, Localize("New filter"), FontSize, CUI::ALIGN_LEFT);
	if(s_aFilterName[0])
		pMenus->DoIcon(IMAGE_FRIENDICONS, pMenus->UI()->MouseInside(&Button)?SPRITE_FRIEND_PLUS_A:SPRITE_FRIEND_PLUS_B, &Icon);
	static CButtonContainer s_AddFilter;
	if(s_aFilterName[0] && pMenus->UI()->DoButtonLogic(&s_AddFilter, "", 0, &Button))
	{
		CServerFilterInfo NewFilterInfo;
		pMenus->m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_CUSTOM, s_aFilterName, pMenus->ServerBrowser()));
		s_aFilterName[0] = 0;
	}

	// slected filter
	CBrowserFilter *pFilter = 0;
	for(int i = 0; i < pMenus->m_lFilters.size(); ++i)
	{
		if(pMenus->m_lFilters[i].Extended())
		{
			pFilter = &pMenus->m_lFilters[i];
			pMenus->m_SelectedFilter = i;
			break;
		}
	}
	if(!pFilter)
		return 0;

	CServerFilterInfo FilterInfo;
	pFilter->GetFilter(&FilterInfo);

	// server filter
	ServerFilter.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFilter);
	pMenus->RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	pMenus->RenderTools()->DrawUIRect(&ServerFilter, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	FilterHeader.HMargin(2.0f, &FilterHeader);
	pMenus->UI()->DoLabel(&FilterHeader, Localize("Server filter"), FontSize+2.0f, CUI::ALIGN_CENTER);

	//ServerFilter.VSplitLeft(5.0f, 0, &ServerFilter);
	//ServerFilter.Margin(3.0f, &ServerFilter);
	//ServerFilter.VMargin(5.0f, &ServerFilter);

	int NewSortHash = FilterInfo.m_SortHash;
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterEmpty = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterEmpty, Localize("Has people playing"), FilterInfo.m_SortHash&IServerBrowser::FILTER_EMPTY, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_EMPTY;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterSpectators = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterSpectators, Localize("Count players only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_SPECTATORS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterFull = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterFull, Localize("Server not full"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FULL, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_FULL;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterFriends = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterFriends, Localize("Show friends only"), FilterInfo.m_SortHash&IServerBrowser::FILTER_FRIENDS, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_FRIENDS;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPw = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPw, Localize("No password"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PW, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PW;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterCompatversion = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterCompatversion, Localize("Compatible version"), FilterInfo.m_SortHash&IServerBrowser::FILTER_COMPAT_VERSION, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_COMPAT_VERSION;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPure = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPure, Localize("Standard gametype"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE, &Button) && pFilter->Custom() != CBrowserFilter::FILTER_STANDARD)
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PURE;

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	static int s_BrFilterPureMap = 0;
	if(pMenus->DoButton_CheckBox(&s_BrFilterPureMap, Localize("Standard map"), FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE_MAP, &Button))
		NewSortHash = FilterInfo.m_SortHash^IServerBrowser::FILTER_PURE_MAP;

	if(FilterInfo.m_SortHash != NewSortHash)
	{
		FilterInfo.m_SortHash = NewSortHash;
		pFilter->SetFilter(&FilterInfo);
	}

	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	pMenus->UI()->DoLabelScaled(&Button, Localize("Game types:"), FontSize, CUI::ALIGN_LEFT);
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
	pMenus->RenderTools()->DrawUIRect(&Button, vec4(0.0, 0.0, 0.0, 0.25f), CUI::CORNER_ALL, 2.0f);
	Button.HMargin(2.0f, &Button);
	pMenus->UI()->ClipEnable(&Button);

	float Length = 0.0f;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
	{
		if(FilterInfo.m_aGametype[i][0])
		{
			Length += pMenus->TextRender()->TextWidth(0, FontSize, FilterInfo.m_aGametype[i], -1) + 14.0f;
		}
	}
	static float s_ScrollValue = 0.0f;
	bool NeedScrollbar = (Button.w - Length) < 0.0f;
	Button.x += min(0.0f, Button.w - Length) * s_ScrollValue;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
	{
		if(FilterInfo.m_aGametype[i][0])
		{
			float CurLength = pMenus->TextRender()->TextWidth(0, FontSize, FilterInfo.m_aGametype[i], -1) + 12.0f;
			Button.VSplitLeft(CurLength, &Icon, &Button);
			pMenus->RenderTools()->DrawUIRect(&Icon, vec4(0.75, 0.75, 0.75, 0.25f), CUI::CORNER_ALL, 3.0f);
			pMenus->UI()->DoLabelScaled(&Icon, FilterInfo.m_aGametype[i], FontSize, CUI::ALIGN_LEFT);
			Icon.VSplitRight(10.0f, 0, &Icon);
			if(pMenus->DoButton_SpriteClean(IMAGE_TOOLICONS, SPRITE_TOOL_X_B, &Icon))
			{
				// remove gametype entry
				if((i == CServerFilterInfo::MAX_GAMETYPES-1) || !FilterInfo.m_aGametype[i+1][0])
					FilterInfo.m_aGametype[i][0] = 0;
				else
				{
					int j = i;
					for(; j < CServerFilterInfo::MAX_GAMETYPES-1 && FilterInfo.m_aGametype[j+1][0]; ++j)
						str_copy(FilterInfo.m_aGametype[j], FilterInfo.m_aGametype[j+1], sizeof(FilterInfo.m_aGametype[j]));
					FilterInfo.m_aGametype[j][0] = 0;
				}
				pFilter->SetFilter(&FilterInfo);
			}
			Button.VSplitLeft(2.0f, 0, &Button);
		}
	}

	pMenus->UI()->ClipDisable();

	if(NeedScrollbar)
	{
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		s_ScrollValue = pMenus->DoScrollbarH(&s_ScrollValue, &Button, s_ScrollValue);
	}
	else
		ServerFilter.HSplitTop(4.f, &Button, &ServerFilter); // Leave some space in between edit boxes
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);

	Button.VSplitLeft(60.0f, &Button, &Icon);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static char s_aGametype[16] = {0};
	static float s_Offset = 0.0f;
	static int s_EditGametype = 0;
	Button.VSplitRight(Button.h, &Label, &Button);
	pMenus->DoEditBox(&s_EditGametype, &Label, s_aGametype, sizeof(s_aGametype), FontSize, &s_Offset);
	pMenus->RenderTools()->DrawUIRect(&Button, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_R, 5.0f);
	pMenus->DoIcon(IMAGE_FRIENDICONS, pMenus->UI()->MouseInside(&Button)?SPRITE_FRIEND_PLUS_A:SPRITE_FRIEND_PLUS_B, &Button);
	static CButtonContainer s_AddGametype;
	if(s_aGametype[0] && pMenus->UI()->DoButtonLogic(&s_AddGametype, "", 0, &Button))
	{
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			if(!FilterInfo.m_aGametype[i][0])
			{
				str_copy(FilterInfo.m_aGametype[i], s_aGametype, sizeof(FilterInfo.m_aGametype[i]));
				pFilter->SetFilter(&FilterInfo);
				s_aGametype[0] = 0;
				break;
			}
		}
	}
	Icon.VSplitLeft(10.0f, 0, &Icon);
	Icon.VSplitLeft(40.0f, &Button, 0);
	static CButtonContainer s_ClearGametypes;
	if(pMenus->DoButton_MenuTabTop(&s_ClearGametypes, Localize("Clear"), false, &Button))
	{
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			FilterInfo.m_aGametype[i][0] = 0;
		}
		pFilter->SetFilter(&FilterInfo);
	}

	if(!NeedScrollbar)
		ServerFilter.HSplitTop(LineSize-4.f, &Button, &ServerFilter);

	{
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
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
	ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
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
		ServerFilter.HSplitTop(LineSize, &Button, &ServerFilter);
		pMenus->UI()->DoLabelScaled(&Button, Localize("Country:"), FontSize, CUI::ALIGN_LEFT);
		Button.VSplitRight(60.0f, 0, &Rect);
		Rect.VSplitLeft(16.0f, &Button, &Rect);
		static int s_BrFilterCountry = 0;
		if(pMenus->DoButton_CheckBox(&s_BrFilterCountry, "", FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY, &Button))
		{
			FilterInfo.m_SortHash ^= IServerBrowser::FILTER_COUNTRY;
			pFilter->SetFilter(&FilterInfo);
		}
		Rect.w = Rect.h*2;
		vec4 Color(1.0f, 1.0f, 1.0f, FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY?1.0f: 0.5f);
		pMenus->m_pClient->m_pCountryFlags->Render(FilterInfo.m_Country, &Color, Rect.x, Rect.y, Rect.w, Rect.h);

		static int s_BrFilterCountryIndex = 0;
		if((FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY) && pMenus->UI()->DoButtonLogic(&s_BrFilterCountryIndex, "", 0, &Rect))
			pMenus->m_Popup = POPUP_COUNTRY;
	}

	// level
	ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(LineSize+2, &Button, &ServerFilter);
	pMenus->UI()->DoLabelScaled(&Button, Localize("Difficulty"), FontSize, CUI::ALIGN_LEFT);
	Button.VSplitRight(60.0f, 0, &Button);
	Button.y -= 2.0f;
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton1;
	if(pMenus->DoButton_SpriteID(&s_LevelButton1, IMAGE_LEVELICONS, (FilterInfo.m_ServerLevel & 1) ? SPRITE_LEVEL_A_OFF : SPRITE_LEVEL_A_ON, false, &Icon, CUI::CORNER_L, 5.0f, true))
	{
		FilterInfo.m_ServerLevel ^= 1;
		pFilter->SetFilter(&FilterInfo);
	}
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton2;
	if(pMenus->DoButton_SpriteID(&s_LevelButton2, IMAGE_LEVELICONS, (FilterInfo.m_ServerLevel & 2) ? SPRITE_LEVEL_B_OFF : SPRITE_LEVEL_B_ON, false, &Icon, 0, 5.0f, true))
	{
		FilterInfo.m_ServerLevel ^= 2;
		pFilter->SetFilter(&FilterInfo);
	}
	Button.VSplitLeft(Button.h, &Icon, &Button);
	static CButtonContainer s_LevelButton3;
	if(pMenus->DoButton_SpriteID(&s_LevelButton3, IMAGE_LEVELICONS, (FilterInfo.m_ServerLevel & 4) ? SPRITE_LEVEL_C_OFF : SPRITE_LEVEL_C_ON, false, &Icon, CUI::CORNER_R, 5.0f, true))
	{
		FilterInfo.m_ServerLevel ^= 4;
		pFilter->SetFilter(&FilterInfo);
	}

	// reset filter
	ServerFilter.HSplitBottom(LineSize, &ServerFilter, 0);
	ServerFilter.HSplitBottom(LineSize, &ServerFilter, &Button);
	Button.VMargin((Button.w-80.0f)/2, &Button);
	static CButtonContainer s_ResetButton;
	if(pMenus->DoButton_Menu(&s_ResetButton, Localize("Reset filter"), 0, &Button))
	{
		pFilter->Reset();
	}

	return 0;
}
