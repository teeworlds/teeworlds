/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/shared/config.h>

#include "menus.h"

CMenus::CListBox::CListBox()
{
	m_ScrollOffset = vec2(0,0);
	m_ListBoxUpdateScroll = false;
	m_aFilterString[0] = '\0';
	m_OffsetFilter = 0.0f;
}

void CMenus::CListBox::DoHeader(const CUIRect *pRect, const char *pTitle,
							   float HeaderHeight, float Spacing)
{
	CUIRect Header;
	CUIRect View = *pRect;

	// background
	View.HSplitTop(HeaderHeight+Spacing, &Header, 0);
	m_pRenderTools->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_T, 5.0f);

	// draw header
	View.HSplitTop(HeaderHeight, &Header, &View);
	Header.y += 2.0f;
	m_pUI->DoLabel(&Header, pTitle, Header.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	View.HSplitTop(Spacing, &Header, &View);

	// setup the variables
	m_ListBoxView = View;
}

bool CMenus::CListBox::DoFilter(float FilterHeight, float Spacing)
{
	CUIRect Filter;
	CUIRect View = m_ListBoxView;

	// background
	View.HSplitTop(FilterHeight+Spacing, &Filter, 0);
	m_pRenderTools->DrawUIRect(&Filter, vec4(0.0f, 0.0f, 0.0f, 0.25f), 0, 5.0f);

	// draw filter
	View.HSplitTop(FilterHeight, &Filter, &View);
	Filter.Margin(Spacing, &Filter);

	float FontSize = Filter.h*ms_FontmodHeight*0.8f;

	CUIRect Label, EditBox;
	Filter.VSplitLeft(Filter.w/5.0f, &Label, &EditBox);
	Label.y += Spacing;
	m_pUI->DoLabel(&Label, Localize("Search:"), FontSize, CUI::ALIGN_CENTER);
	bool Changed = m_pMenus->DoEditBox(m_aFilterString, &EditBox, m_aFilterString, sizeof(m_aFilterString), FontSize, &m_OffsetFilter);

	View.HSplitTop(Spacing, &Filter, &View);

	m_ListBoxView = View;

	return Changed;
}

void CMenus::CListBox::DoFooter(const char *pBottomText, float FooterHeight)
{
	m_pBottomText = pBottomText;
	m_FooterHeight = FooterHeight;
}

void CMenus::CListBox::DoStart(float RowHeight, int NumItems, int ItemsPerRow, int SelectedIndex,
							  const CUIRect *pRect, bool Background, bool *pActive)
{
	CUIRect View;
	if(pRect)
		View = *pRect;
	else
		View = m_ListBoxView;

	// background
	if(Background)
		m_pRenderTools->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	// draw footers
	if(m_pBottomText)
	{
		CUIRect Footer;
		View.HSplitBottom(m_FooterHeight, &View, &Footer);
		Footer.VSplitLeft(10.0f, 0, &Footer);
		Footer.y += 2.0f;
		m_pUI->DoLabel(&Footer, m_pBottomText, Footer.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
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
	m_ListBoxDoneEvents = 0;
	m_ListBoxItemActivated = false;

	// handle input
	if(!pActive || *pActive)
	{
		if(m_pMenus->m_DownArrowPressed)
			m_ListBoxNewSelOffset += 1;
		if(m_pMenus->m_UpArrowPressed)
			m_ListBoxNewSelOffset -= 1;
	}

	// setup the scrollbar
	m_ScrollOffset = vec2(0, 0);
	m_ScrollRegion.Begin(&m_ListBoxView, &m_ScrollOffset);
	m_ListBoxView.y += m_ScrollOffset.y;
}

CMenus::CListboxItem CMenus::CListBox::DoNextRow()
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

	if(m_ListBoxSelectedIndex == m_ListBoxItemIndex)
		Item.m_Selected = 1;

	Item.m_Visible = !m_ScrollRegion.IsRectClipped(Item.m_Rect);

	m_ListBoxItemIndex++;
	return Item;
}

CMenus::CListboxItem CMenus::CListBox::DoNextItem(const void *pId, bool Selected, bool *pActive)
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

	if(Item.m_Visible && m_pUI->DoButtonLogic(pId, "", m_ListBoxSelectedIndex == m_ListBoxItemIndex, &Item.m_Rect))
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
			m_ListBoxDoneEvents = 1;

			if(m_pMenus->m_EnterPressed || (s_ItemClicked && m_pInput->MouseDoubleClick()))
			{
				m_ListBoxItemActivated = true;
				m_pUI->SetActiveItem(0);
			}
		}

		CUIRect r = Item.m_Rect;
		m_pRenderTools->DrawUIRect(&r, vec4(1, 1, 1, ProcessInput ? 0.5f : 0.33f), CUI::CORNER_ALL, 5.0f);
	}
	/*else*/ if(m_pUI->HotItem() == pId)
	{
		CUIRect r = Item.m_Rect;
		m_pRenderTools->DrawUIRect(&r, vec4(1, 1, 1, 0.33f), CUI::CORNER_ALL, 5.0f);
	}

	return Item;
}

int CMenus::CListBox::DoEnd(bool *pItemActivated)
{
	m_ScrollRegion.End();
	if(m_ListBoxNewSelOffset != 0 && m_ListBoxSelectedIndex != -1 && m_ListBoxSelectedIndex == m_ListBoxNewSelected)
	{
		m_ListBoxNewSelected = clamp(m_ListBoxNewSelected + m_ListBoxNewSelOffset, 0, m_ListBoxNumItems - 1);
		m_ListBoxUpdateScroll = true;
	}
	if(pItemActivated)
		*pItemActivated = m_ListBoxItemActivated;
	return m_ListBoxNewSelected;
}

bool CMenus::CListBox::FilterMatches(const char *pNeedle)
{
	return !m_aFilterString[0] || str_find_nocase(pNeedle, m_aFilterString);
}
