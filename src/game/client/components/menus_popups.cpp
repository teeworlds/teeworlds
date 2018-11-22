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

