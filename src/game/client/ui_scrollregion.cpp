/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/keys.h>

#include "ui_scrollregion.h"

CScrollRegion::CScrollRegion()
{
	m_ScrollY = 0;
	m_ContentH = 0;
	m_AnimTime = 0;
	m_AnimInitScrollY = 0;
	m_AnimTargetScrollY = 0;
	m_RequestScrollY = -1;
	m_ContentScrollOff = vec2(0,0);
	m_Params = CScrollRegionParams();
}

void CScrollRegion::Begin(CUIRect *pClipRect, vec2 *pOutOffset, CScrollRegionParams *pParams)
{
	if(pParams)
		m_Params = *pParams;

	const bool ContentOverflows = m_ContentH > pClipRect->h;
	const bool ForceShowScrollbar = m_Params.m_Flags&CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;

	CUIRect ScrollBarBg;
	bool HasScrollBar = ContentOverflows || ForceShowScrollbar;
	CUIRect *pModifyRect = HasScrollBar ? pClipRect : 0;
	pClipRect->VSplitRight(m_Params.m_ScrollbarWidth, pModifyRect, &ScrollBarBg);
	ScrollBarBg.Margin(m_Params.m_ScrollbarMargin, &m_RailRect);

	// only show scrollbar if required
	if(HasScrollBar)
	{
		if(m_Params.m_ScrollbarBgColor.a > 0)
			ScrollBarBg.Draw(m_Params.m_ScrollbarBgColor, 4.0f, CUIRect::CORNER_R);
		if(m_Params.m_RailBgColor.a > 0)
			m_RailRect.Draw(m_Params.m_RailBgColor, m_RailRect.w/2.0f);
	}
	if(!ContentOverflows)
		m_ContentScrollOff.y = 0;

	if(m_Params.m_ClipBgColor.a > 0)
		pClipRect->Draw(m_Params.m_ClipBgColor, 4.0f, HasScrollBar ? CUIRect::CORNER_L : CUIRect::CORNER_ALL);

	UI()->ClipEnable(pClipRect);

	m_ClipRect = *pClipRect;
	m_ContentH = 0;
	*pOutOffset = m_ContentScrollOff;
}

void CScrollRegion::End()
{
	UI()->ClipDisable();

	// only show scrollbar if content overflows
	if(m_ContentH <= m_ClipRect.h)
		return;

	// scroll wheel
	CUIRect RegionRect = m_ClipRect;
	RegionRect.w += m_Params.m_ScrollbarWidth;

	float AnimationDuration = 0.5f;

	const bool IsPageScroll = Input()->KeyIsPressed(KEY_LALT) || Input()->KeyIsPressed(KEY_RALT);
	if(UI()->MouseHovered(&RegionRect))
	{
		const float ScrollUnit = IsPageScroll ? m_ClipRect.h : m_Params.m_ScrollUnit;
		if(UI()->KeyPress(KEY_MOUSE_WHEEL_UP))
		{
			m_AnimTime = AnimationDuration;
			m_AnimInitScrollY = m_ScrollY;
			m_AnimTargetScrollY -= ScrollUnit;
		}
		else if(UI()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
		{
			m_AnimTime = AnimationDuration;
			m_AnimInitScrollY = m_ScrollY;
			m_AnimTargetScrollY += ScrollUnit;
		}
	}

	const float SliderHeight = maximum(m_Params.m_SliderMinHeight,
		m_ClipRect.h/m_ContentH * m_RailRect.h);

	CUIRect Slider = m_RailRect;
	Slider.h = SliderHeight;
	
	const float MaxSlider = m_RailRect.h - SliderHeight;
	const float MaxScroll = m_ContentH - m_ClipRect.h;

	if(m_RequestScrollY >= 0)
	{
		m_AnimTargetScrollY = m_RequestScrollY;
		m_AnimTime = 0;
		m_RequestScrollY = -1;
	}

	m_AnimTargetScrollY = clamp(m_AnimTargetScrollY, 0.0f, MaxScroll);

	if(absolute(m_AnimInitScrollY - m_AnimTargetScrollY) < 0.5f)
		m_AnimTime = 0;

	if(m_AnimTime > 0)
	{
		m_AnimTime -= Client()->RenderFrameTime();
		float AnimProgress = (1 - pow(m_AnimTime / AnimationDuration, 3)); // cubic ease out
		m_ScrollY = m_AnimInitScrollY + (m_AnimTargetScrollY - m_AnimInitScrollY) * AnimProgress;
	}
	else
	{
		m_ScrollY = m_AnimTargetScrollY;
	}

	Slider.y += m_ScrollY/MaxScroll * MaxSlider;

	bool Hovered = false;
	bool Grabbed = false;
	const void *pID = &m_ScrollY;
	const bool InsideSlider = UI()->MouseHovered(&Slider);
	const bool InsideRail = UI()->MouseHovered(&m_RailRect);

	if(UI()->CheckActiveItem(pID) && UI()->MouseButton(0))
	{
		float MouseY = UI()->MouseY();
		m_ScrollY += (MouseY - (Slider.y + m_SliderGrabPos.y)) / MaxSlider * MaxScroll;
		m_SliderGrabPos.y = clamp(m_SliderGrabPos.y, 0.0f, SliderHeight);
		m_AnimTargetScrollY = m_ScrollY;
		m_AnimTime = 0;
		Grabbed = true;
	}
	else if(InsideSlider)
	{
		UI()->SetHotItem(pID);

		if(!UI()->CheckActiveItem(pID) && UI()->MouseButtonClicked(0))
		{
			UI()->SetActiveItem(pID);
			m_SliderGrabPos.y = UI()->MouseY() - Slider.y;
			m_AnimTargetScrollY = m_ScrollY;
			m_AnimTime = 0;
		}
		Hovered = true;
	}
	else if(InsideRail && UI()->MouseButtonClicked(0))
	{
		m_ScrollY += (UI()->MouseY() - (Slider.y+Slider.h/2)) / MaxSlider * MaxScroll;
		UI()->SetActiveItem(pID);
		m_SliderGrabPos.y = Slider.h/2;
		m_AnimTargetScrollY = m_ScrollY;
		m_AnimTime = 0;
		Hovered = true;
	}
	else if(UI()->CheckActiveItem(pID) && !UI()->MouseButton(0))
	{
		UI()->SetActiveItem(0);
	}

	m_ScrollY = clamp(m_ScrollY, 0.0f, MaxScroll);
	m_ContentScrollOff.y = -m_ScrollY;

	vec4 SliderColor;
	if(Grabbed)
		SliderColor = m_Params.m_SliderColorGrabbed;
	else if(Hovered)
		SliderColor = m_Params.m_SliderColorHover;
	else
		SliderColor = m_Params.m_SliderColor;
	Slider.Draw(SliderColor, Slider.w/2.0f);
}

void CScrollRegion::AddRect(const CUIRect &Rect)
{
	vec2 ContentPos = vec2(m_ClipRect.x, m_ClipRect.y);
	ContentPos.x += m_ContentScrollOff.x;
	ContentPos.y += m_ContentScrollOff.y;
	m_LastAddedRect = Rect;
	m_ContentH = maximum(Rect.y + Rect.h - ContentPos.y, m_ContentH);
}

void CScrollRegion::ScrollHere(int Option)
{
	const float MinHeight = minimum(m_ClipRect.h, m_LastAddedRect.h);
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

bool CScrollRegion::IsRectClipped(const CUIRect &Rect) const
{
	return (m_ClipRect.x > (Rect.x + Rect.w)
		|| (m_ClipRect.x + m_ClipRect.w) < Rect.x
		|| m_ClipRect.y > (Rect.y + Rect.h)
		|| (m_ClipRect.y + m_ClipRect.h) < Rect.y);
}

bool CScrollRegion::IsScrollbarShown() const
{
	return m_ContentH > m_ClipRect.h;
}

bool CScrollRegion::IsAnimating() const
{
	return m_AnimTime > 0;
}
