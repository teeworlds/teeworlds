/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/shared/config.h>

#include "gameclient.h"
#include "ui_listbox.h"

CListBox::CListBox()
{
	m_ScrollOffset = vec2(0,0);
	m_ListBoxUpdateScroll = false;
}

void CListBox::DoBegin(const CUIRect *pRect)
{
	// setup the variables
	m_ListBoxView = *pRect;
}

void CListBox::DoHeader(const CUIRect *pRect, const char *pTitle,
							   float HeaderHeight, float Spacing)
{
	CUIRect Header;
	CUIRect View = *pRect;

	// background
	View.HSplitTop(HeaderHeight+Spacing, &Header, 0);
	Header.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, m_BackgroundCorners&CUIRect::CORNER_T);

	// draw header
	View.HSplitTop(HeaderHeight, &Header, &View);
	UI()->DoLabel(&Header, pTitle, Header.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	View.HSplitTop(Spacing, &Header, &View);

	// setup the variables
	m_ListBoxView = View;
}

void CListBox::DoSpacing(float Spacing)
{
	CUIRect View = m_ListBoxView;
	View.HSplitTop(Spacing, 0, &View);
	m_ListBoxView = View;
}

bool CListBox::DoFilter(float FilterHeight, float Spacing)
{
	CUIRect Filter;
	CUIRect View = m_ListBoxView;

	// background
	View.HSplitTop(FilterHeight+Spacing, &Filter, 0);
	Filter.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, CUIRect::CORNER_NONE);

	// draw filter
	View.HSplitTop(FilterHeight, &Filter, &View);
	Filter.Margin(Spacing, &Filter);

	float FontSize = Filter.h*CUI::ms_FontmodHeight*0.8f;

	CUIRect Label, EditBox;
	Filter.VSplitLeft(Filter.w/5.0f, &Label, &EditBox);
	Label.y += Spacing;
	UI()->DoLabel(&Label, Localize("Search:"), FontSize, TEXTALIGN_CENTER);
	bool Changed = UI()->DoEditBox(&m_FilterInput, &EditBox, FontSize);

	View.HSplitTop(Spacing, &Filter, &View);

	m_ListBoxView = View;

	return Changed;
}

void CListBox::DoFooter(const char *pBottomText, float FooterHeight)
{
	m_pBottomText = pBottomText;
	m_FooterHeight = FooterHeight;
}

void CListBox::DoStart(float RowHeight, int NumItems, int ItemsPerRow, int RowsPerScroll, 
							int SelectedIndex, const CUIRect *pRect, bool Background, bool *pActive, int BackgroundCorners)
{
	CUIRect View;
	if(pRect)
		View = *pRect;
	else
		View = m_ListBoxView;

	// background
	m_BackgroundCorners = BackgroundCorners;
	if(Background)
		View.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, m_BackgroundCorners&CUIRect::CORNER_B);

	// draw footers
	if(m_pBottomText)
	{
		CUIRect Footer;
		View.HSplitBottom(m_FooterHeight, &View, &Footer);
		Footer.VSplitLeft(10.0f, 0, &Footer);
		UI()->DoLabel(&Footer, m_pBottomText, Footer.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);
	}

	// setup the variables
	m_ListBoxView = View;
	m_ListBoxSelectedIndex = SelectedIndex;
	m_ListBoxNewSelected = SelectedIndex;
	m_ListBoxNewSelOffset = 0;
	m_ListBoxItemIndex = 0;
	m_ListBoxRowHeight = RowHeight;
	m_ListBoxNumItems = NumItems;
	m_ListBoxItemsPerRow = ItemsPerRow;
	m_ListBoxDoneEvents = false;
	m_ListBoxItemActivated = false;

	// handle input
	if(!pActive || *pActive)
	{
		if(UI()->ConsumeHotkey(CUI::HOTKEY_DOWN))
			m_ListBoxNewSelOffset += 1;
		if(UI()->ConsumeHotkey(CUI::HOTKEY_UP))
			m_ListBoxNewSelOffset -= 1;
	}

	// setup the scrollbar
	m_ScrollOffset = vec2(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = m_ListBoxRowHeight * RowsPerScroll;
	m_ScrollRegion.Begin(&m_ListBoxView, &m_ScrollOffset, &ScrollParams);
	m_ListBoxView.y += m_ScrollOffset.y;
}

CListboxItem CListBox::DoNextRow()
{
	static CUIRect s_RowView;
	CListboxItem Item = {0};

	if(m_ListBoxItemIndex%m_ListBoxItemsPerRow == 0)
		m_ListBoxView.HSplitTop(m_ListBoxRowHeight /*-2.0f*/, &s_RowView, &m_ListBoxView);
	m_ScrollRegion.AddRect(s_RowView);
	if(m_ListBoxUpdateScroll && m_ListBoxSelectedIndex == m_ListBoxItemIndex)
	{
		m_ScrollRegion.ScrollHere(CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
		m_ListBoxUpdateScroll = false;
	}

	s_RowView.VSplitLeft(s_RowView.w/(m_ListBoxItemsPerRow-m_ListBoxItemIndex%m_ListBoxItemsPerRow), &Item.m_Rect, &s_RowView);

	Item.m_Selected = m_ListBoxSelectedIndex == m_ListBoxItemIndex;
	Item.m_Visible = !m_ScrollRegion.IsRectClipped(Item.m_Rect);

	m_ListBoxItemIndex++;
	return Item;
}

CListboxItem CListBox::DoNextItem(const void *pId, bool Selected, bool *pActive)
{
	int ThisItemIndex = m_ListBoxItemIndex;
	if(Selected)
	{
		if(m_ListBoxSelectedIndex == m_ListBoxNewSelected)
			m_ListBoxNewSelected = ThisItemIndex;
		m_ListBoxSelectedIndex = ThisItemIndex;
	}

	CListboxItem Item = DoNextRow();
	static bool s_ItemClicked = false;

	if(Item.m_Visible && UI()->DoButtonLogic(pId, &Item.m_Rect))
	{
		s_ItemClicked = true;
		m_ListBoxNewSelected = ThisItemIndex;
		if(pActive)
			*pActive = true;
	}
	else
		s_ItemClicked = false;

	const bool ProcessInput = !pActive || *pActive;

	// process input, regard selected index
	if(m_ListBoxSelectedIndex == ThisItemIndex)
	{
		if(ProcessInput && !m_ListBoxDoneEvents)
		{
			m_ListBoxDoneEvents = true;

			if(UI()->ConsumeHotkey(CUI::HOTKEY_ENTER) || (s_ItemClicked && Input()->MouseDoubleClick()))
			{
				m_ListBoxItemActivated = true;
				UI()->SetActiveItem(0);
			}
		}

		Item.m_Rect.Draw(vec4(1, 1, 1, ProcessInput ? 0.5f : 0.33f));
	}
	/*else*/ if(UI()->HotItem() == pId && !m_ScrollRegion.IsAnimating())
	{
		Item.m_Rect.Draw(vec4(1, 1, 1, 0.33f));
	}

	return Item;
}

CListboxItem CListBox::DoSubheader()
{
	CListboxItem Item = DoNextRow();
	Item.m_Rect.Draw(vec4(1, 1, 1, 0.2f), 0.0f, CUIRect::CORNER_NONE);
	return Item;
}

int CListBox::DoEnd()
{
	m_ScrollRegion.End();
	if(m_ListBoxNewSelOffset != 0 && m_ListBoxSelectedIndex != -1 && m_ListBoxSelectedIndex == m_ListBoxNewSelected)
	{
		m_ListBoxNewSelected = clamp(m_ListBoxNewSelected + m_ListBoxNewSelOffset, 0, m_ListBoxNumItems - 1);
		m_ListBoxUpdateScroll = true;
	}
	return m_ListBoxNewSelected;
}

bool CListBox::FilterMatches(const char *pNeedle) const
{
	return !m_FilterInput.GetLength() || str_find_nocase(pNeedle, m_FilterInput.GetString());
}
