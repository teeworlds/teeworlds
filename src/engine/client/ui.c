/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/system.h>
#include <engine/interface.h>
#include <engine/config.h>
#include "ui.h"

/********************************************************
 UI                                                      
*********************************************************/

struct pretty_font
{
    float m_CharStartTable[256];
	float m_CharEndTable[256];
	int font_texture;
};

extern struct pretty_font *current_font;

static const void *hot_item = 0;
static const void *active_item = 0;
static const void *last_active_item = 0;
static const void *becomming_hot_item = 0;
static float mouse_x, mouse_y; /* in gui space */
static float mouse_wx, mouse_wy; /* in world space */
static unsigned mouse_buttons = 0;

float ui_mouse_x() { return mouse_x; }
float ui_mouse_y() { return mouse_y; }
float ui_mouse_world_x() { return mouse_wx; }
float ui_mouse_world_y() { return mouse_wy; }
int ui_mouse_button(int index) { return (mouse_buttons>>index)&1; }

void ui_set_hot_item(const void *id) { becomming_hot_item = id; }
void ui_set_active_item(const void *id) { active_item = id; if (id) last_active_item = id; }
void ui_clear_last_active_item() { last_active_item = 0; }
const void *ui_hot_item() { return hot_item; }
const void *ui_active_item() { return active_item; }
const void *ui_last_active_item() { return last_active_item; }

int ui_update(float mx, float my, float mwx, float mwy, int buttons)
{
    mouse_x = mx;
    mouse_y = my;
    mouse_wx = mwx;
    mouse_wy = mwy;
    mouse_buttons = buttons;
    hot_item = becomming_hot_item;
    if(active_item)
    	hot_item = active_item;
    becomming_hot_item = 0;
    return 0;
}

int ui_mouse_inside(float x, float y, float w, float h)
{
    if(mouse_x >= x && mouse_x <= x+w && mouse_y >= y && mouse_y <= y+h)
        return 1;
    return 0;
}

void ui_do_image(int texture, float x, float y, float w, float h)
{
    gfx_blend_normal();
    gfx_texture_set(texture);
    gfx_quads_begin();
    gfx_setcolor(1,1,1,1);
    gfx_quads_setsubset(
        0.0f, /* startx */
        0.0f, /* starty */
        1.0f, /* endx */
        1.0f); /* endy */                              
    gfx_quads_drawTL(x,y,w,h);
    gfx_quads_end();
}

void ui_do_label(float x, float y, const char *text, float size)
{
    gfx_blend_normal();
    gfx_texture_set(current_font->font_texture);
    gfx_pretty_text(x, y, size, text, -1);
}

int ui_do_button(const void *id, const char *text, int checked, float x, float y, float w, float h, draw_button_callback draw_func, void *extra)
{
    /* logic */
    int r = 0;
    int inside = ui_mouse_inside(x,y,w,h);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
		{
			if(inside)
				r = 1;
			ui_set_active_item(0);
		}
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
			ui_set_active_item(id);
	}
	
	if(inside)
		ui_set_hot_item(id);

    draw_func(id, text, checked, x, y, w, h, extra);
    return r;
}

static float scale = 1.0f;
#define MEMORY_SIZE 10*1024
static struct rect memory[MEMORY_SIZE];
static int memory_used = 0;
static struct rect screen = { 0.0f, 0.0f, 800.0f, 600.0f };

void ui_foreach_rect(rect_fun fun)
{
    int hrm;
    for (hrm = 0; hrm < memory_used; hrm++)
        fun(&memory[hrm]);
}

static void add_rect(struct rect *r)
{
    if (memory_used < MEMORY_SIZE)
        memory[memory_used++] = *r;
    else
        dbg_msg("ui", "rect memory full.");
}

struct rect *ui_screen()
{
    if (config.debug)
    {
        memory_used = 0;
    }

    return &screen;
}

void ui_scale(float s)
{
    scale = s;
}

void ui_hsplit_t(const struct rect *original, int pixels, struct rect *top, struct rect *bottom)
{
    struct rect r = *original;
    pixels *= scale;

    if (top)
    {
        top->x = r.x;
        top->y = r.y;
        top->w = r.w;
        top->h = pixels;
    }

    if (bottom)
    {
        bottom->x = r.x;
        bottom->y = r.y + pixels;
        bottom->w = r.w;
        bottom->h = r.h - pixels;
    }

    if (config.debug)
    {
        if (top)
            add_rect(top);
        if (bottom)
            add_rect(bottom);
    }
}

void ui_hsplit_b(const struct rect *original, int pixels, struct rect *top, struct rect *bottom)
{
    struct rect r = *original;
    pixels *= scale;

    if (top)
    {
        top->x = r.x;
        top->y = r.y;
        top->w = r.w;
        top->h = r.h - pixels;
    }

    if (bottom)
    {
        bottom->x = r.x;
        bottom->y = r.y + r.h - pixels;
        bottom->w = r.w;
        bottom->h = pixels;
    }

    if (config.debug)
    {
        if (top)
            add_rect(top);
        if (bottom)
            add_rect(bottom);
    }
}

void ui_vsplit_l(const struct rect *original, int pixels, struct rect *left, struct rect *right)
{
    struct rect r = *original;
    pixels *= scale;

    if (left)
    {
        left->x = r.x;
        left->y = r.y;
        left->w = pixels;
        left->h = r.h;
    }

    if (right)
    {
        right->x = r.x + pixels;
        right->y = r.y;
        right->w = r.w - pixels;
        right->h = r.h;
    }

    if (config.debug)
    {
        if (left)
            add_rect(left);
        if (right)
            add_rect(right);
    }
}

void ui_vsplit_r(const struct rect *original, int pixels, struct rect *left, struct rect *right)
{
    struct rect r = *original;
    pixels *= scale;

    if (left)
    {
        left->x = r.x;
        left->y = r.y;
        left->w = r.w - pixels;
        left->h = r.h;
    }

    if (right)
    {
        right->x = r.x + r.w - pixels;
        right->y = r.y;
        right->w = pixels;
        right->h = r.h;
    }

    if (config.debug)
    {
        if (left)
            add_rect(left);
        if (right)
            add_rect(right);
    }
}

void ui_margin(const struct rect *original, int pixels, struct rect *other_rect)
{
    struct rect r = *original;
    pixels *= scale;

    other_rect->x = r.x + pixels;
    other_rect->y = r.y + pixels;
    other_rect->w = r.w - 2*pixels;
    other_rect->h = r.h - 2*pixels;
}
