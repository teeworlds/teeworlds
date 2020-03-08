/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include "menus.h"

CMenus::CScrollRegion::CScrollRegion()
{
	m_ScrollY = 0;
	m_ContentH = 0;
	m_RequestScrollY = -1;
	m_ContentScrollOff = vec2(0,0);
	m_Params = CScrollRegionParams();
}

void CMenus::CScrollRegion::Begin(CUIRect* pClipRect, vec2* pOutOffset, const CScrollRegionParams* pParams)
{
	if(pParams)
		m_Params = *pParams;

	const bool ContentOverflows = m_ContentH > pClipRect->h;
	const bool ForceShowScrollbar = m_Params.m_Flags&CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;

	CUIRect ScrollBarBg;
	CUIRect* pModifyRect = (ContentOverflows || ForceShowScrollbar) ? pClipRect : 0;
	pClipRect->VSplitRight(m_Params.m_ScrollbarWidth, pModifyRect, &ScrollBarBg);
	ScrollBarBg.Margin(m_Params.m_ScrollbarMargin, &m_RailRect);

	// only show scrollbar if required
	if(ContentOverflows || ForceShowScrollbar)
	{
		if(m_Params.m_ScrollbarBgColor.a > 0)
			m_pRenderTools->DrawRoundRect(&ScrollBarBg, m_Params.m_ScrollbarBgColor, 4.0f);
		if(m_Params.m_RailBgColor.a > 0)
			m_pRenderTools->DrawRoundRect(&m_RailRect, m_Params.m_RailBgColor, m_RailRect.w/2.0f);
	}
	if(!ContentOverflows)
		m_ContentScrollOff.y = 0;

	if(m_Params.m_ClipBgColor.a > 0)
		m_pRenderTools->DrawRoundRect(pClipRect, m_Params.m_ClipBgColor, 4.0f);

	m_pUI->ClipEnable(pClipRect);

	m_ClipRect = *pClipRect;
	m_ContentH = 0;
	*pOutOffset = m_ContentScrollOff;
}

void CMenus::CScrollRegion::End()
{
	m_pUI->ClipDisable();

	// only show scrollbar if content overflows
	if(m_ContentH <= m_ClipRect.h)
		return;

	// scroll wheel
	CUIRect RegionRect = m_ClipRect;
	RegionRect.w += m_Params.m_ScrollbarWidth;
	if(m_pUI->MouseInside(&RegionRect))
	{
		if(m_pInput->KeyPress(KEY_MOUSE_WHEEL_UP))
			m_ScrollY -= m_Params.m_ScrollSpeed;
		else if(m_pInput->KeyPress(KEY_MOUSE_WHEEL_DOWN))
			m_ScrollY += m_Params.m_ScrollSpeed;
	}

	const float SliderHeight = max(m_Params.m_SliderMinHeight,
		m_ClipRect.h/m_ContentH * m_RailRect.h);

	CUIRect Slider = m_RailRect;
	Slider.h = SliderHeight;
	const float MaxScroll = m_RailRect.h - SliderHeight;

	if(m_RequestScrollY >= 0)
	{
		m_ScrollY = m_RequestScrollY/(m_ContentH - m_ClipRect.h) * MaxScroll;
		m_RequestScrollY = -1;
	}

	m_ScrollY = clamp(m_ScrollY, 0.0f, MaxScroll);
	Slider.y += m_ScrollY;

	bool Hovered = false;
	bool Grabbed = false;
	const void* pID = &m_ScrollY;
	const bool InsideSlider = m_pUI->MouseInside(&Slider);
	const bool InsideRail = m_pUI->MouseInside(&m_RailRect);

	if(InsideSlider)
	{
		m_pUI->SetHotItem(pID);

		if(!m_pUI->CheckActiveItem(pID) && m_pUI->MouseButtonClicked(0))
		{
			m_pUI->SetActiveItem(pID);
			m_MouseGrabStart.y = m_pUI->MouseY();
		}

		Hovered = true;
	}
	else if(InsideRail && m_pUI->MouseButton(0))
	{
		const float SliderDistance = m_pUI->MouseY() - (Slider.y+Slider.h/2);
		m_ScrollY += sign(SliderDistance) * log(abs(SliderDistance)); // slow down to reasonable scroll speed; keep sign with logarithm
		Hovered = true;
	}

	if(m_pUI->CheckActiveItem(pID) && !m_pUI->MouseButton(0))
		m_pUI->SetActiveItem(0);

	// move slider
	if(m_pUI->CheckActiveItem(pID) && m_pUI->MouseButton(0))
	{
		float my = m_pUI->MouseY();
		m_ScrollY += my - m_MouseGrabStart.y;
		m_MouseGrabStart.y = my;

		Grabbed = true;
	}

	m_ScrollY = clamp(m_ScrollY, 0.0f, MaxScroll);
	m_ContentScrollOff.y = -m_ScrollY/MaxScroll * (m_ContentH - m_ClipRect.h);

	vec4 SliderColor = m_Params.m_SliderColor;
	if(Grabbed)
		SliderColor = m_Params.m_SliderColorGrabbed;
	else if(Hovered)
		SliderColor = m_Params.m_SliderColorHover;

	m_pRenderTools->DrawRoundRect(&Slider, SliderColor, Slider.w/2.0f);
}

void CMenus::CScrollRegion::AddRect(CUIRect Rect)
{
	vec2 ContentPos = vec2(m_ClipRect.x, m_ClipRect.y);
	ContentPos.x += m_ContentScrollOff.x;
	ContentPos.y += m_ContentScrollOff.y;
	m_LastAddedRect = Rect;
	m_ContentH = max(Rect.y + Rect.h - ContentPos.y, m_ContentH);
}

void CMenus::CScrollRegion::ScrollHere(int Option)
{
	const float MinHeight = min(m_ClipRect.h, m_LastAddedRect.h);
	const float TopScroll = m_LastAddedRect.y - (m_ClipRect.y + m_ContentScrollOff.y);

	switch(Option)
	{
		case CScrollRegion::SCROLLHERE_TOP:
			m_RequestScrollY = TopScroll;
			break;

		case CScrollRegion::SCROLLHERE_BOTTOM:
			m_RequestScrollY = TopScroll - (m_ClipRect.h - MinHeight);
			break;

		case CScrollRegion::SCROLLHERE_KEEP_IN_VIEW:
		default: {
			const float dy = m_LastAddedRect.y - m_ClipRect.y;

			if(dy < 0)
				m_RequestScrollY = TopScroll;
			else if(dy > (m_ClipRect.h-MinHeight))
				m_RequestScrollY = TopScroll - (m_ClipRect.h - MinHeight);
		} break;
	}
}

bool CMenus::CScrollRegion::IsRectClipped(const CUIRect& Rect) const
{
	return (m_ClipRect.x > (Rect.x + Rect.w)
		|| (m_ClipRect.x + m_ClipRect.w) < Rect.x
		|| m_ClipRect.y > (Rect.y + Rect.h)
		|| (m_ClipRect.y + m_ClipRect.h) < Rect.y);
}

bool CMenus::CScrollRegion::IsScrollbarShown() const
{
	return m_ContentH > m_ClipRect.h;
}