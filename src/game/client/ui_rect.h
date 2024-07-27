/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_RECT_H
#define GAME_CLIENT_UI_RECT_H

#include <base/vmath.h>

class CUIRect
{
	enum
	{
		NUM_ROUND_CORNER_SEGMENTS = 8
	};
	static const float s_CornerAnglePerSegment;
	static class IGraphics *s_pGraphics;

public:
	static void Init(class IGraphics *pGraphics) { s_pGraphics = pGraphics; }

	float x, y, w, h;

	vec2 Center() const;

	void HSplitMid(CUIRect *pTop, CUIRect *pBottom, float Spacing = 0.0f) const;
	void HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const;
	void HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const;
	void VSplitMid(CUIRect *pLeft, CUIRect *pRight, float Spacing = 0.0f) const;
	void VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const;
	void VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const;

	void Margin(float Cut, CUIRect *pOtherRect) const;
	void VMargin(float Cut, CUIRect *pOtherRect) const;
	void HMargin(float Cut, CUIRect *pOtherRect) const;

	bool Inside(float x, float y) const;
	bool Inside(vec2 Pos) const { return Inside(Pos.x, Pos.y); }

	enum
	{
		CORNER_NONE = 0,
		CORNER_TL = 1,
		CORNER_TR = 2,
		CORNER_BL = 4,
		CORNER_BR = 8,
		CORNER_ITL = 16,
		CORNER_ITR = 32,
		CORNER_IBL = 64,
		CORNER_IBR = 128,

		CORNER_T = CORNER_TL|CORNER_TR,
		CORNER_B = CORNER_BL|CORNER_BR,
		CORNER_R = CORNER_TR|CORNER_BR,
		CORNER_L = CORNER_TL|CORNER_BL,

		CORNER_IT = CORNER_ITL|CORNER_ITR,
		CORNER_IB = CORNER_IBL|CORNER_IBR,
		CORNER_IR = CORNER_ITR|CORNER_IBR,
		CORNER_IL = CORNER_ITL|CORNER_IBL,

		CORNER_ALL = CORNER_T|CORNER_B,
		CORNER_INV_ALL = CORNER_IT|CORNER_IB
	};

	void Draw(const vec4 &Color, float Rounding = 5.0f, int Corners = CORNER_ALL) const;
	void Draw4(const vec4 &ColorTopLeft, const vec4 &ColorTopRight, const vec4 &ColorBottomLeft, const vec4 &ColorBottomRight, float Rounding = 5.0f, int Corners = CORNER_ALL) const;
};

#endif
