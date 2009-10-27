/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>

#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include <engine/client/graphics.h>
#include "ui.hpp"

/********************************************************
 UI                                                      
*********************************************************/

CUI::CUI()
{
	m_pHotItem = 0;
	m_pActiveItem = 0;
	m_pLastActiveItem = 0;
	m_pBecommingHotItem = 0;
	
	m_MouseX = 0;
	m_MouseY = 0;
	m_MouseWorldX = 0;
	m_MouseWorldY = 0;
	m_MouseButtons = 0;
	m_LastMouseButtons = 0;	
	
	m_Screen.x = 0;
	m_Screen.y = 0;
	m_Screen.w = 848.0f;
	m_Screen.h = 480.0f;
}

int CUI::Update(float mx, float my, float mwx, float mwy, int Buttons)
{
    m_MouseX = mx;
    m_MouseY = my;
    m_MouseWorldX = mwx;
    m_MouseWorldY = mwy;
    m_LastMouseButtons = m_MouseButtons;
    m_MouseButtons = Buttons;
    m_pHotItem = m_pBecommingHotItem;
    if(m_pActiveItem)
    	m_pHotItem = m_pActiveItem;
    m_pBecommingHotItem = 0;
    return 0;
}

int CUI::MouseInside(const CUIRect *r)
{
    if(m_MouseX >= r->x && m_MouseX <= r->x+r->w && m_MouseY >= r->y && m_MouseY <= r->y+r->h)
        return 1;
    return 0;
}

CUIRect *CUI::Screen()
{
    float aspect = Graphics()->ScreenAspect();
    float w, h;

    h = 600;
    w = aspect*h;

    m_Screen.w = w;
    m_Screen.h = h;

    return &m_Screen;
}

void CUI::SetScale(float s)
{
    //config.UI()->Scale = (int)(s*100.0f);
}

/*float CUI::Scale()
{
    return config.UI()->Scale/100.0f;
}*/

void CUI::ClipEnable(const CUIRect *r)
{
	float xscale = Graphics()->ScreenWidth()/Screen()->w;
	float yscale = Graphics()->ScreenHeight()/Screen()->h;
	Graphics()->ClipEnable((int)(r->x*xscale), (int)(r->y*yscale), (int)(r->w*xscale), (int)(r->h*yscale));
}

void CUI::ClipDisable()
{
	Graphics()->ClipDisable();
}

void CUIRect::HSplitTop(float cut, CUIRect *top, CUIRect *bottom) const
{
    CUIRect r = *this;
    cut *= Scale();

    if (top)
    {
        top->x = r.x;
        top->y = r.y;
        top->w = r.w;
        top->h = cut;
    }

    if (bottom)
    {
        bottom->x = r.x;
        bottom->y = r.y + cut;
        bottom->w = r.w;
        bottom->h = r.h - cut;
    }
}

void CUIRect::HSplitBottom(float cut, CUIRect *top, CUIRect *bottom) const
{
    CUIRect r = *this;
    cut *= Scale();

    if (top)
    {
        top->x = r.x;
        top->y = r.y;
        top->w = r.w;
        top->h = r.h - cut;
    }

    if (bottom)
    {
        bottom->x = r.x;
        bottom->y = r.y + r.h - cut;
        bottom->w = r.w;
        bottom->h = cut;
    }
}


void CUIRect::VSplitMid(CUIRect *left, CUIRect *right) const
{
    CUIRect r = *this;
    float cut = r.w/2;

    if (left)
    {
        left->x = r.x;
        left->y = r.y;
        left->w = cut;
        left->h = r.h;
    }

    if (right)
    {
        right->x = r.x + cut;
        right->y = r.y;
        right->w = r.w - cut;
        right->h = r.h;
    }
}

void CUIRect::VSplitLeft(float cut, CUIRect *left, CUIRect *right) const
{
    CUIRect r = *this;
    cut *= Scale();

    if (left)
    {
        left->x = r.x;
        left->y = r.y;
        left->w = cut;
        left->h = r.h;
    }

    if (right)
    {
        right->x = r.x + cut;
        right->y = r.y;
        right->w = r.w - cut;
        right->h = r.h;
    }
}

void CUIRect::VSplitRight(float cut, CUIRect *left, CUIRect *right) const
{
    CUIRect r = *this;
    cut *= Scale();

    if (left)
    {
        left->x = r.x;
        left->y = r.y;
        left->w = r.w - cut;
        left->h = r.h;
    }

    if (right)
    {
        right->x = r.x + r.w - cut;
        right->y = r.y;
        right->w = cut;
        right->h = r.h;
    }
}

void CUIRect::Margin(float cut, CUIRect *other_rect) const
{
    CUIRect r = *this;
	cut *= Scale();

    other_rect->x = r.x + cut;
    other_rect->y = r.y + cut;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h - 2*cut;
}

void CUIRect::VMargin(float cut, CUIRect *other_rect) const
{
    CUIRect r = *this;
	cut *= Scale();

    other_rect->x = r.x + cut;
    other_rect->y = r.y;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h;
}

void CUIRect::HMargin(float cut, CUIRect *other_rect) const
{
    CUIRect r = *this;
	cut *= Scale();

    other_rect->x = r.x;
    other_rect->y = r.y + cut;
    other_rect->w = r.w;
    other_rect->h = r.h - 2*cut;
}

int CUI::DoButtonLogic(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
    /* logic */
    int ReturnValue = 0;
    int Inside = MouseInside(pRect);
	static int ButtonUsed = 0;

	if(ActiveItem() == pID)
	{
		if(!MouseButton(ButtonUsed))
		{
			if(Inside && Checked >= 0)
				ReturnValue = 1+ButtonUsed;
			SetActiveItem(0);
		}
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
		{
			SetActiveItem(pID);
			ButtonUsed = 0;
		}
		
		if(MouseButton(1))
		{
			SetActiveItem(pID);
			ButtonUsed = 1;
		}
	}
	
	if(Inside)
		SetHotItem(pID);

    return ReturnValue;
}
/*
int CUI::DoButton(const void *id, const char *text, int checked, const CUIRect *r, ui_draw_button_func draw_func, const void *extra)
{
    // logic
    int ret = 0;
    int inside = ui_MouseInside(r);
	static int button_used = 0;

	if(ui_ActiveItem() == id)
	{
		if(!ui_MouseButton(button_used))
		{
			if(inside && checked >= 0)
				ret = 1+button_used;
			ui_SetActiveItem(0);
		}
	}
	else if(ui_HotItem() == id)
	{
		if(ui_MouseButton(0))
		{
			ui_SetActiveItem(id);
			button_used = 0;
		}
		
		if(ui_MouseButton(1))
		{
			ui_SetActiveItem(id);
			button_used = 1;
		}
	}
	
	if(inside)
		ui_SetHotItem(id);

	if(draw_func)
    	draw_func(id, text, checked, r, extra);
    return ret;
}*/

void CUI::DoLabel(const CUIRect *r, const char *text, float size, int align, int max_width)
{
	// TODO: FIX ME!!!!
    //Graphics()->BlendNormal();
    size *= Scale();
    if(align == 0)
    {
    	float tw = gfx_text_width(0, size, text, max_width);
    	gfx_text(0, r->x + r->w/2-tw/2, r->y, size, text, max_width);
	}
	else if(align < 0)
    	gfx_text(0, r->x, r->y, size, text, max_width);
	else if(align > 0)
	{
    	float tw = gfx_text_width(0, size, text, max_width);
    	gfx_text(0, r->x + r->w-tw, r->y, size, text, max_width);
	}
}
