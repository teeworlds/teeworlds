#include <engine/system.h>
#include <engine/interface.h>
#include "ui.h"

/********************************************************
 UI                                                      
*********************************************************/
//static unsigned mouse_buttons_last = 0;

struct pretty_font
{
    float m_CharStartTable[256];
	float m_CharEndTable[256];
	int font_texture;
};

extern struct pretty_font *current_font;

static void *hot_item = 0;
static void *active_item = 0;
static void *last_active_item = 0;
static void *becomming_hot_item = 0;
static float mouse_x, mouse_y; // in gui space
static float mouse_wx, mouse_wy; // in world space
static unsigned mouse_buttons = 0;

float ui_mouse_x() { return mouse_x; }
float ui_mouse_y() { return mouse_y; }
float ui_mouse_world_x() { return mouse_wx; }
float ui_mouse_world_y() { return mouse_wy; }
int ui_mouse_button(int index) { return (mouse_buttons>>index)&1; }

void ui_set_hot_item(void *id) { becomming_hot_item = id; }
void ui_set_active_item(void *id) { active_item = id; if (id) last_active_item = id; }
void ui_clear_last_active_item() { last_active_item = 0; }
void *ui_hot_item() { return hot_item; }
void *ui_active_item() { return active_item; }
void *ui_last_active_item() { return last_active_item; }

int ui_update(float mx, float my, float mwx, float mwy, int buttons)
{
    //mouse_buttons_last = mouse_buttons;
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

/*
static int ui_mouse_button_released(int index)
{
    return ((mouse_buttons_last>>index)&1) && !();
}*/

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
        0.0f, // startx
        0.0f, // starty
        1.0f, // endx
        1.0f); // endy                                
    gfx_quads_drawTL(x,y,w,h);
    gfx_quads_end();
}

void ui_do_label(float x, float y, const char *text, float size)
{
    gfx_blend_normal();
    gfx_texture_set(current_font->font_texture);
    gfx_pretty_text(x, y, size, text, -1);
}

int ui_do_button(void *id, const char *text, int checked, float x, float y, float w, float h, draw_button_callback draw_func, void *extra)
{
    // logic
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

