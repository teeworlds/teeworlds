/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_SCROLLREGION_H
#define GAME_CLIENT_UI_SCROLLREGION_H

#include "ui.h"

struct CScrollRegionParams
{
	float m_ScrollbarWidth;
	float m_ScrollbarMargin;
	float m_SliderMinHeight;
	float m_ScrollUnit;
	vec4 m_ClipBgColor;
	vec4 m_ScrollbarBgColor;
	vec4 m_RailBgColor;
	vec4 m_SliderColor;
	vec4 m_SliderColorHover;
	vec4 m_SliderColorGrabbed;
	int m_Flags;

	enum
	{
		FLAG_CONTENT_STATIC_WIDTH = 0x1
	};

	CScrollRegionParams()
	{
		m_ScrollbarWidth = 20;
		m_ScrollbarMargin = 5;
		m_SliderMinHeight = 25;
		m_ScrollUnit = 10;
		m_ClipBgColor = vec4(0.0f, 0.0f, 0.0f, 0.25f);
		m_ScrollbarBgColor = vec4(0.0f, 0.0f, 0.0f, 0.25f);
		m_RailBgColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
		m_SliderColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
		m_SliderColorHover = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_SliderColorGrabbed = vec4(0.9f, 0.9f, 0.9f, 1.0f);
		m_Flags = 0;
	}
};

/*
Usage:
	-- Initialization --
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	s_ScrollRegion.Begin(&ScrollRegionRect, &ScrollOffset);
	Content = ScrollRegionRect;
	Content.y += ScrollOffset.y;

	-- "Register" your content rects --
	CUIRect Rect;
	Content.HSplitTop(SomeValue, &Rect, &Content);
	s_ScrollRegion.AddRect(Rect);

	-- [Optional] Knowing if a rect is clipped --
	s_ScrollRegion.IsRectClipped(Rect);

	-- [Optional] Scroll to a rect (to the last added rect)--
	...
	s_ScrollRegion.AddRect(Rect);
	s_ScrollRegion.ScrollHere(Option);

	-- End --
	s_ScrollRegion.End();
*/

// Instances of CScrollRegion must be static, as member addresses are used as UI item IDs
class CScrollRegion : private CUIElementBase
{
private:
	float m_ScrollY;
	float m_ContentH;
	float m_RequestScrollY; // [0, ContentHeight]

	float m_AnimTime;
	float m_AnimInitScrollY;
	float m_AnimTargetScrollY;

	CUIRect m_ClipRect;
	CUIRect m_RailRect;
	CUIRect m_LastAddedRect; // saved for ScrollHere()
	vec2 m_SliderGrabPos; // where did user grab the slider
	vec2 m_ContentScrollOff;
	CScrollRegionParams m_Params;

public:
	enum
	{
		SCROLLHERE_KEEP_IN_VIEW = 0,
		SCROLLHERE_TOP,
		SCROLLHERE_BOTTOM,
	};

	CScrollRegion();
	void Begin(CUIRect *pClipRect, vec2 *pOutOffset, CScrollRegionParams *pParams = 0);
	void End();
	void AddRect(const CUIRect &Rect);
	void ScrollHere(int Option = CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
	bool IsRectClipped(const CUIRect &Rect) const;
	bool IsScrollbarShown() const;
	bool IsAnimating() const;
};

#endif
