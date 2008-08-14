/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>

#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include "gc_ui.hpp"

/********************************************************
 UI                                                      
*********************************************************/
static const void *hot_item = 0;
static const void *active_item = 0;
static const void *last_active_item = 0;
static const void *becomming_hot_item = 0;
static float mouse_x, mouse_y; /* in gui space */
static float mouse_wx, mouse_wy; /* in world space */
static unsigned mouse_buttons = 0;
static unsigned last_mouse_buttons = 0;

float ui_mouse_x() { return mouse_x; }
float ui_mouse_y() { return mouse_y; }
float ui_mouse_world_x() { return mouse_wx; }
float ui_mouse_world_y() { return mouse_wy; }
int ui_mouse_button(int index) { return (mouse_buttons>>index)&1; }
int ui_mouse_button_clicked(int index) { return ui_mouse_button(index) && !((last_mouse_buttons>>index)&1) ; }

void ui_set_hot_item(const void *id) { becomming_hot_item = id; }
void ui_set_active_item(const void *id) { active_item = id; if (id) last_active_item = id; }
void ui_clear_last_active_item() { last_active_item = 0; }
const void *ui_hot_item() { return hot_item; }
const void *ui_next_hot_item() { return becomming_hot_item; }
const void *ui_active_item() { return active_item; }
const void *ui_last_active_item() { return last_active_item; }

int ui_update(float mx, float my, float mwx, float mwy, int buttons)
{
    mouse_x = mx;
    mouse_y = my;
    mouse_wx = mwx;
    mouse_wy = mwy;
    last_mouse_buttons = mouse_buttons;
    mouse_buttons = buttons;
    hot_item = becomming_hot_item;
    if(active_item)
    	hot_item = active_item;
    becomming_hot_item = 0;
    return 0;
}
/*
bool ui_
*/
int ui_mouse_inside(const RECT *r)
{
    if(mouse_x >= r->x && mouse_x <= r->x+r->w && mouse_y >= r->y && mouse_y <= r->y+r->h)
        return 1;
    return 0;
}

static RECT screen = { 0.0f, 0.0f, 848.0f, 480.0f };

RECT *ui_screen()
{
    float aspect = gfx_screenaspect();
    float w, h;

    h = 600;
    w = aspect*h;

    screen.w = w;
    screen.h = h;

    return &screen;
}

void ui_set_scale(float s)
{
    config.ui_scale = (int)(s*100.0f);
}

float ui_scale()
{
    return config.ui_scale/100.0f;
}

void ui_clip_enable(const RECT *r)
{
	float xscale = gfx_screenwidth()/ui_screen()->w;
	float yscale = gfx_screenheight()/ui_screen()->h;
	gfx_clip_enable((int)(r->x*xscale), (int)(r->y*yscale), (int)(r->w*xscale), (int)(r->h*yscale));
}

void ui_clip_disable()
{
	gfx_clip_disable();
}

void ui_hsplit_t(const RECT *original, float cut, RECT *top, RECT *bottom)
{
    RECT r = *original;
    cut *= ui_scale();

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

void ui_hsplit_b(const RECT *original, float cut, RECT *top, RECT *bottom)
{
    RECT r = *original;
    cut *= ui_scale();

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


void ui_vsplit_mid(const RECT *original, RECT *left, RECT *right)
{
    RECT r = *original;
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

void ui_vsplit_l(const RECT *original, float cut, RECT *left, RECT *right)
{
    RECT r = *original;
    cut *= ui_scale();

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

void ui_vsplit_r(const RECT *original, float cut, RECT *left, RECT *right)
{
    RECT r = *original;
    cut *= ui_scale();

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

void ui_margin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= ui_scale();

    other_rect->x = r.x + cut;
    other_rect->y = r.y + cut;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h - 2*cut;
}

void ui_vmargin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= ui_scale();

    other_rect->x = r.x + cut;
    other_rect->y = r.y;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h;
}

void ui_hmargin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= ui_scale();

    other_rect->x = r.x;
    other_rect->y = r.y + cut;
    other_rect->w = r.w;
    other_rect->h = r.h - 2*cut;
}


int ui_do_button(const void *id, const char *text, int checked, const RECT *r, ui_draw_button_func draw_func, const void *extra)
{
    /* logic */
    int ret = 0;
    int inside = ui_mouse_inside(r);
	static int button_used = 0;

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(button_used))
		{
			if(inside && checked >= 0)
				ret = 1+button_used;
			ui_set_active_item(0);
		}
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			ui_set_active_item(id);
			button_used = 0;
		}
		
		if(ui_mouse_button(1))
		{
			ui_set_active_item(id);
			button_used = 1;
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	if(draw_func)
    	draw_func(id, text, checked, r, extra);
    return ret;
}

void ui_do_label(const RECT *r, const char *text, float size, int align, int max_width)
{
    gfx_blend_normal();
    size *= ui_scale();
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
