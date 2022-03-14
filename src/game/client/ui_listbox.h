/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_LISTBOX_H
#define GAME_CLIENT_UI_LISTBOX_H

#include "ui_scrollregion.h"

struct CListboxItem
{
	bool m_Visible;
	bool m_Selected;
	bool m_Disabled;
	CUIRect m_Rect;
};

// Instances of CListBox must be static, as member addresses are used as UI item IDs
class CListBox : private CUIElementBase
{
private:
	CUIRect m_ListBoxView;
	float m_ListBoxRowHeight;
	int m_ListBoxItemIndex;
	int m_ListBoxSelectedIndex;
	int m_ListBoxNewSelected;
	int m_ListBoxNewSelOffset;
	bool m_ListBoxUpdateScroll;
	bool m_ListBoxDoneEvents;
	int m_ListBoxNumItems;
	int m_ListBoxItemsPerRow;
	bool m_ListBoxItemActivated;
	const char *m_pBottomText;
	float m_FooterHeight;
	CScrollRegion m_ScrollRegion;
	vec2 m_ScrollOffset;
	CLineInputBuffered<128> m_FilterInput;
	int m_BackgroundCorners;

protected:
	CListboxItem DoNextRow();

public:
	CListBox();

	void DoBegin(const CUIRect *pRect);
	void DoHeader(const CUIRect *pRect, const char *pTitle, float HeaderHeight = 20.0f, float Spacing = 2.0f);
	void DoSpacing(float Spacing = 20.0f);
	bool DoFilter(float FilterHeight = 20.0f, float Spacing = 2.0f);
	void DoFooter(const char *pBottomText, float FooterHeight = 20.0f); // call before DoStart to create a footer
	void DoStart(float RowHeight, int NumItems, int ItemsPerRow, int RowsPerScroll, int SelectedIndex,
				const CUIRect *pRect = 0, bool Background = true, bool *pActive = 0, int BackgroundCorners = CUIRect::CORNER_ALL);
	void ScrollToSelected() { m_ListBoxUpdateScroll = true; }
	CListboxItem DoNextItem(const void *pID, bool Selected = false, bool *pActive = 0);
	CListboxItem DoSubheader();
	int DoEnd();
	bool FilterMatches(const char *pNeedle) const;
	bool WasItemActivated() const { return m_ListBoxItemActivated; }
	float GetScrollBarWidth() const { return m_ScrollRegion.IsScrollbarShown() ? 20 : 0; }
};

#endif
