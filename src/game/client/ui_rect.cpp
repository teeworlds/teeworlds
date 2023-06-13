/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include "ui_rect.h"

/********************************************************
 UI Rect
*********************************************************/

const float CUIRect::s_CornerAnglePerSegment = 2 * NUM_ROUND_CORNER_SEGMENTS / pi;
IGraphics *CUIRect::s_pGraphics = 0;

vec2 CUIRect::Center() const
{
	return vec2(x + w / 2.0f, y + h / 2.0f);
}

void CUIRect::HSplitMid(CUIRect *pTop, CUIRect *pBottom, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.h/2;
	const float HalfSpacing = Spacing/2;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut - HalfSpacing;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut + HalfSpacing;
		pBottom->w = r.w;
		pBottom->h = r.h - Cut - HalfSpacing;
	}
}

void CUIRect::HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut;
		pBottom->w = r.w;
		pBottom->h = r.h - Cut;
	}
}

void CUIRect::HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = r.h - Cut;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + r.h - Cut;
		pBottom->w = r.w;
		pBottom->h = Cut;
	}
}


void CUIRect::VSplitMid(CUIRect *pLeft, CUIRect *pRight, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.w/2;
	const float HalfSpacing = Spacing/2;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut - HalfSpacing;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + Cut + HalfSpacing;
		pRight->y = r.y;
		pRight->w = r.w - Cut - HalfSpacing;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + Cut;
		pRight->y = r.y;
		pRight->w = r.w - Cut;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = r.w - Cut;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + r.w - Cut;
		pRight->y = r.y;
		pRight->w = Cut;
		pRight->h = r.h;
	}
}

void CUIRect::Margin(float Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x + Cut;
	pOtherRect->y = r.y + Cut;
	pOtherRect->w = r.w - 2*Cut;
	pOtherRect->h = r.h - 2*Cut;
}

void CUIRect::VMargin(float Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x + Cut;
	pOtherRect->y = r.y;
	pOtherRect->w = r.w - 2*Cut;
	pOtherRect->h = r.h;
}

void CUIRect::HMargin(float Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x;
	pOtherRect->y = r.y + Cut;
	pOtherRect->w = r.w;
	pOtherRect->h = r.h - 2*Cut;
}

bool CUIRect::Inside(float x, float y) const
{
	return x >= this->x
		&& x < this->x + this->w
		&& y >= this->y
		&& y < this->y + this->h;
}

void CUIRect::Draw(const vec4 &Color, float Rounding, int Corners) const
{
	s_pGraphics->TextureClear();

	// TODO: FIX US
	s_pGraphics->QuadsBegin();
	s_pGraphics->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);

	const float r = Rounding;

	IGraphics::CFreeformItem a_FreeformItems[NUM_ROUND_CORNER_SEGMENTS/2*8];
	int NumFreeformItems = 0;
	for(int i = 0; i < NUM_ROUND_CORNER_SEGMENTS; i += 2)
	{
		float a1 = i/s_CornerAnglePerSegment;
		float a2 = (i+1)/s_CornerAnglePerSegment;
		float a3 = (i+2)/s_CornerAnglePerSegment;
		float Ca1 = cosf(a1);
		float Ca2 = cosf(a2);
		float Ca3 = cosf(a3);
		float Sa1 = sinf(a1);
		float Sa2 = sinf(a2);
		float Sa3 = sinf(a3);

		if(Corners&CORNER_TL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+r, y+r,
				x+(1-Ca1)*r, y+(1-Sa1)*r,
				x+(1-Ca3)*r, y+(1-Sa3)*r,
				x+(1-Ca2)*r, y+(1-Sa2)*r);

		if(Corners&CORNER_TR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w-r, y+r,
				x+w-r+Ca1*r, y+(1-Sa1)*r,
				x+w-r+Ca3*r, y+(1-Sa3)*r,
				x+w-r+Ca2*r, y+(1-Sa2)*r);

		if(Corners&CORNER_BL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+r, y+h-r,
				x+(1-Ca1)*r, y+h-r+Sa1*r,
				x+(1-Ca3)*r, y+h-r+Sa3*r,
				x+(1-Ca2)*r, y+h-r+Sa2*r);

		if(Corners&CORNER_BR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w-r, y+h-r,
				x+w-r+Ca1*r, y+h-r+Sa1*r,
				x+w-r+Ca3*r, y+h-r+Sa3*r,
				x+w-r+Ca2*r, y+h-r+Sa2*r);

		if(Corners&CORNER_ITL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x, y,
				x+(1-Ca1)*r, y-r+Sa1*r,
				x+(1-Ca3)*r, y-r+Sa3*r,
				x+(1-Ca2)*r, y-r+Sa2*r);

		if(Corners&CORNER_ITR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w, y,
				x+w-r+Ca1*r, y-r+Sa1*r,
				x+w-r+Ca3*r, y-r+Sa3*r,
				x+w-r+Ca2*r, y-r+Sa2*r);

		if(Corners&CORNER_IBL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x, y+h,
				x+(1-Ca1)*r, y+h+(1-Sa1)*r,
				x+(1-Ca3)*r, y+h+(1-Sa3)*r,
				x+(1-Ca2)*r, y+h+(1-Sa2)*r);

		if(Corners&CORNER_IBR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w, y+h,
				x+w-r+Ca1*r, y+h+(1-Sa1)*r,
				x+w-r+Ca3*r, y+h+(1-Sa3)*r,
				x+w-r+Ca2*r, y+h+(1-Sa2)*r);
	}
	s_pGraphics->QuadsDrawFreeform(a_FreeformItems, NumFreeformItems);

	IGraphics::CQuadItem a_QuadItems[9];
	int NumQuadItems = 0;
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+r, y+r, w-r*2, h-r*2); // center
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+r, y, w-r*2, r); // top
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+r, y+h-r, w-r*2, r); // bottom
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x, y+r, r, h-r*2); // left
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+w-r, y+r, r, h-r*2); // right

	if(!(Corners&CORNER_TL)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x, y, r, r);
	if(!(Corners&CORNER_TR)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+w, y, -r, r);
	if(!(Corners&CORNER_BL)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x, y+h, r, -r);
	if(!(Corners&CORNER_BR)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+w, y+h, -r, -r);

	s_pGraphics->QuadsDrawTL(a_QuadItems, NumQuadItems);
	s_pGraphics->QuadsEnd();
}

void CUIRect::Draw4(const vec4 &ColorTopLeft, const vec4 &ColorTopRight, const vec4 &ColorBottomLeft, const vec4 &ColorBottomRight, float Rounding, int Corners) const
{
	s_pGraphics->TextureClear();
	s_pGraphics->QuadsBegin();

	const float r = Rounding;

	for(int i = 0; i < NUM_ROUND_CORNER_SEGMENTS; i+=2)
	{
		float a1 = i/s_CornerAnglePerSegment;
		float a2 = (i+1)/s_CornerAnglePerSegment;
		float a3 = (i+2)/s_CornerAnglePerSegment;
		float Ca1 = cosf(a1);
		float Ca2 = cosf(a2);
		float Ca3 = cosf(a3);
		float Sa1 = sinf(a1);
		float Sa2 = sinf(a2);
		float Sa3 = sinf(a3);

		if(Corners&CORNER_TL)
		{
			s_pGraphics->SetColor(ColorTopLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+r, y+r,
				x+(1-Ca1)*r, y+(1-Sa1)*r,
				x+(1-Ca3)*r, y+(1-Sa3)*r,
				x+(1-Ca2)*r, y+(1-Sa2)*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_TR)
		{
			s_pGraphics->SetColor(ColorTopRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w-r, y+r,
				x+w-r+Ca1*r, y+(1-Sa1)*r,
				x+w-r+Ca3*r, y+(1-Sa3)*r,
				x+w-r+Ca2*r, y+(1-Sa2)*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_BL)
		{
			s_pGraphics->SetColor(ColorBottomLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+r, y+h-r,
				x+(1-Ca1)*r, y+h-r+Sa1*r,
				x+(1-Ca3)*r, y+h-r+Sa3*r,
				x+(1-Ca2)*r, y+h-r+Sa2*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_BR)
		{
			s_pGraphics->SetColor(ColorBottomRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w-r, y+h-r,
				x+w-r+Ca1*r, y+h-r+Sa1*r,
				x+w-r+Ca3*r, y+h-r+Sa3*r,
				x+w-r+Ca2*r, y+h-r+Sa2*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_ITL)
		{
			s_pGraphics->SetColor(ColorTopLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x, y,
				x+(1-Ca1)*r, y-r+Sa1*r,
				x+(1-Ca3)*r, y-r+Sa3*r,
				x+(1-Ca2)*r, y-r+Sa2*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_ITR)
		{
			s_pGraphics->SetColor(ColorTopRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w, y,
				x+w-r+Ca1*r, y-r+Sa1*r,
				x+w-r+Ca3*r, y-r+Sa3*r,
				x+w-r+Ca2*r, y-r+Sa2*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_IBL)
		{
			s_pGraphics->SetColor(ColorBottomLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x, y+h,
				x+(1-Ca1)*r, y+h+(1-Sa1)*r,
				x+(1-Ca3)*r, y+h+(1-Sa3)*r,
				x+(1-Ca2)*r, y+h+(1-Sa2)*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_IBR)
		{
			s_pGraphics->SetColor(ColorBottomRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w, y+h,
				x+w-r+Ca1*r, y+h+(1-Sa1)*r,
				x+w-r+Ca3*r, y+h+(1-Sa3)*r,
				x+w-r+Ca2*r, y+h+(1-Sa2)*r);
			s_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}
	}

	// center
	{
		s_pGraphics->SetColor4(ColorTopLeft, ColorTopRight, ColorBottomLeft, ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y+r, w-r*2, h-r*2);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// top
	{
		s_pGraphics->SetColor4(ColorTopLeft, ColorTopRight, ColorTopLeft, ColorTopRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y, w-r*2, r);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// bottom
	{
		s_pGraphics->SetColor4(ColorBottomLeft, ColorBottomRight, ColorBottomLeft, ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y+h-r, w-r*2, r);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// left
	{
		s_pGraphics->SetColor4(ColorTopLeft, ColorTopLeft, ColorBottomLeft, ColorBottomLeft);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y+r, r, h-r*2);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// right
	{
		s_pGraphics->SetColor4(ColorTopRight, ColorTopRight, ColorBottomRight, ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w-r, y+r, r, h-r*2);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_TL))
	{
		s_pGraphics->SetColor(ColorTopLeft);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y, r, r);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_TR))
	{
		s_pGraphics->SetColor(ColorTopRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w, y, -r, r);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_BL))
	{
		s_pGraphics->SetColor(ColorBottomLeft);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y+h, r, -r);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_BR))
	{
		s_pGraphics->SetColor(ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w, y+h, -r, -r);
		s_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	s_pGraphics->QuadsEnd();
}
