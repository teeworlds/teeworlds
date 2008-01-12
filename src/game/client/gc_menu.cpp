/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <game/g_math.h>
#include <game/g_vmath.h>

extern "C" {
	#include <engine/e_system.h>
	#include <engine/e_interface.h>
	#include <engine/e_config.h>
	#include <engine/client/ec_ui.h>
	#include <engine/client/ec_font.h>
}

#include "../g_mapres.h"
#include "../g_version.h"
#include "../g_protocol.h"

#include "gc_mapres_image.h"
#include "gc_mapres_tilemap.h"

#include "../generated/gc_data.h"
#include "gc_render.h"
#include "gc_skin.h"
#include <mastersrv/mastersrv.h>

extern data_container *data;

// abit uglyness
extern const obj_player_info *local_info;
extern const obj_game *gameobj;

extern bool menu_active;
extern bool menu_game_active;

static bool need_restart = false;

enum
{
	POPUP_NONE=0,
	POPUP_FIRST_LAUNCH,
	POPUP_CONNECTING,
	POPUP_DISCONNECTED,
	POPUP_PASSWORD,
	POPUP_QUIT, 
};

static int popup = POPUP_NONE;

//static vec4 gui_color(0.9f,0.78f,0.65f, 0.5f);
//static vec4 gui_color(0.78f,0.9f,0.65f, 0.5f);

static vec4 gui_color(0.65f,0.78f,0.9f, 0.5f);

static vec4 color_tabbar_inactive_outgame(0,0,0,0.25f);
static vec4 color_tabbar_active_outgame(0,0,0,0.5f);

static float color_ingame_scale_i = 0.5f;
static float color_ingame_scale_a = 0.2f;
static vec4 color_tabbar_inactive_ingame(gui_color.r*color_ingame_scale_i, gui_color.g*color_ingame_scale_i, gui_color.b*color_ingame_scale_i,0.75f);
static vec4 color_tabbar_active_ingame(gui_color.r*color_ingame_scale_a, gui_color.g*color_ingame_scale_a, gui_color.b*color_ingame_scale_a,0.85f);
//static vec4 color_tabbar_inactive_ingame(0.2f,0.2f,0.2f,0.5f);
//static vec4 color_tabbar_active_ingame(0.2f,0.2f,0.2f,0.75f);

static vec4 color_tabbar_inactive = color_tabbar_inactive_outgame;
static vec4 color_tabbar_active = color_tabbar_active_outgame;

enum
{
	/*
	CORNER_TL=1,
	CORNER_TR=2,
	CORNER_BL=4,
	CORNER_BR=8,
	
	CORNER_T=CORNER_TL|CORNER_TR,
	CORNER_B=CORNER_BL|CORNER_BR,
	CORNER_R=CORNER_TR|CORNER_BR,
	CORNER_L=CORNER_TL|CORNER_BL,
	
	CORNER_ALL=CORNER_T|CORNER_B,
	*/
	
	PAGE_NEWS=0,
	PAGE_INTERNET,
	PAGE_LAN,
	PAGE_FAVORITES,
	PAGE_SETTINGS,
	//PAGE_GAME, // not a real page
	PAGE_SYSTEM,
};
/*
typedef struct 
{
    float x, y, w, h;
} RECT;*/

static RECT screen = { 0.0f, 0.0f, 848.0f, 480.0f };

extern void select_sprite(int id, int flags=0, int sx=0, int sy=0);

RECT *ui2_screen()
{
    float aspect = gfx_screenaspect();
    float w, h;

    h = 600;
    w = aspect*h;

    screen.w = w;
    screen.h = h;

    return &screen;
}

void ui2_set_scale(float s)
{
    config.ui_scale = (int)(s*100.0f);
}

float ui2_scale()
{
    return config.ui_scale/100.0f;
}

void ui2_clip_enable(const RECT *r)
{
	float xscale = gfx_screenwidth()/ui2_screen()->w;
	float yscale = gfx_screenheight()/ui2_screen()->h;
	gfx_clip_enable((int)(r->x*xscale), (int)(r->y*yscale), (int)(r->w*xscale), (int)(r->h*yscale));
}

void ui2_clip_disable()
{
	gfx_clip_disable();
}

void ui2_hsplit_t(const RECT *original, float cut, RECT *top, RECT *bottom)
{
    RECT r = *original;
    cut *= ui2_scale();

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

void ui2_hsplit_b(const RECT *original, float cut, RECT *top, RECT *bottom)
{
    RECT r = *original;
    cut *= ui2_scale();

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


void ui2_vsplit_mid(const RECT *original, RECT *left, RECT *right)
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

void ui2_vsplit_l(const RECT *original, float cut, RECT *left, RECT *right)
{
    RECT r = *original;
    cut *= ui2_scale();

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

void ui2_vsplit_r(const RECT *original, float cut, RECT *left, RECT *right)
{
    RECT r = *original;
    cut *= ui2_scale();

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

void ui2_margin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= ui2_scale();

    other_rect->x = r.x + cut;
    other_rect->y = r.y + cut;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h - 2*cut;
}

void ui2_vmargin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= ui2_scale();

    other_rect->x = r.x + cut;
    other_rect->y = r.y;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h;
}

void ui2_hmargin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= ui2_scale();

    other_rect->x = r.x;
    other_rect->y = r.y + cut;
    other_rect->w = r.w;
    other_rect->h = r.h - 2*cut;
}

typedef void (*ui2_draw_button_func)(const void *id, const char *text, int checked, const RECT *r, void *extra);

int ui2_do_button(const void *id, const char *text, int checked, const RECT *r, ui2_draw_button_func draw_func, void *extra)
{
    /* logic */
    int ret = 0;
    int inside = ui_mouse_inside(r);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
		{
			if(inside)
				ret = 1;
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

	if(draw_func)
    	draw_func(id, text, checked, r, extra);
    return ret;
}


void ui2_do_label(const RECT *r, const char *text, float size, int align, int max_width = -1)
{
    gfx_blend_normal();
    size *= ui2_scale();
    if(align == 0)
    {
    	float tw = gfx_pretty_text_width(size, text, max_width);
    	gfx_pretty_text(r->x + r->w/2-tw/2, r->y, size, text, max_width);
	}
	else if(align < 0)
    	gfx_pretty_text(r->x, r->y, size, text, max_width);
	else if(align > 0)
	{
    	float tw = gfx_pretty_text_width(size, text, max_width);
    	gfx_pretty_text(r->x + r->w-tw, r->y, size, text, max_width);
	}
}


extern void draw_round_rect_ext(float x, float y, float w, float h, float r, int corners);
extern void draw_round_rect(float x, float y, float w, float h, float r);

static void ui2_draw_rect(const RECT *r, vec4 color, int corners, float rounding)
{
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(color.r, color.g, color.b, color.a);
	draw_round_rect_ext(r->x,r->y,r->w,r->h,rounding*ui2_scale(), corners);
	gfx_quads_end();
}

static void ui2_draw_browse_icon(int what, const RECT *r)
{
	gfx_texture_set(data->images[IMAGE_BROWSEICONS].id);
	gfx_quads_begin();
	select_sprite(SPRITE_BROWSE_PROGRESS1); // default
	if(what == -1)
	{
	}
	else if(what <= 100)
	{
		if(what < 66)
			select_sprite(SPRITE_BROWSE_PROGRESS2);
		else
			select_sprite(SPRITE_BROWSE_PROGRESS3);
	}
	else if(what&0x100)
	{
		select_sprite(SPRITE_BROWSE_LOCK);
	}
	gfx_quads_drawTL(r->x,r->y,r->w,r->h);
	gfx_quads_end();
}

static void ui2_draw_menu_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_draw_rect(r, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
	ui2_do_label(r, text, 18.0f, 0);
}


static void ui2_draw_keyselect_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_draw_rect(r, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
	ui2_do_label(r, text, 14.0f, 0);
}

static void ui2_draw_menu_tab_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
		ui2_draw_rect(r, color_tabbar_active, CORNER_T, 10.0f);
	else
		ui2_draw_rect(r, color_tabbar_inactive, CORNER_T, 10.0f);
	ui2_do_label(r, text, 22.0f, 0);
}


static void ui2_draw_settings_tab_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
		ui2_draw_rect(r, color_tabbar_active, CORNER_R, 10.0f);
	else
		ui2_draw_rect(r, color_tabbar_inactive, CORNER_R, 10.0f);
	ui2_do_label(r, text, 20.0f, 0);
}

static void ui2_draw_grid_header(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
		ui2_draw_rect(r, vec4(1,1,1,0.5f), CORNER_T, 5.0f);
	//else
	//	ui2_draw_rect(r, vec4(1,1,1,0.1f), CORNER_T, 5.0f);
	RECT t;
	ui2_vsplit_l(r, 5.0f, 0, &t);
	ui2_do_label(&t, text, 14.0f, -1);
}
/*
static void ui2_draw_grid_cell_l(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_do_label(r, text, 18, -1);
}

static void ui2_draw_grid_cell_r(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_do_label(r, text, 18, 1);
}*/

static void ui2_draw_list_row(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
	{
		RECT sr = *r;
		ui2_margin(&sr, 1.5f, &sr);
		ui2_draw_rect(&sr, vec4(1,1,1,0.5f), CORNER_ALL, 4.0f);
	}
	ui2_do_label(r, text, 14.0f, -1);
}

static void ui2_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const RECT *r)
{
	RECT c = *r;
	RECT t = *r;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	ui2_vsplit_l(&t, 5.0f, 0, &t);
	
	ui2_margin(&c, 2.0f, &c);
	ui2_draw_rect(&c, vec4(1,1,1,0.25f), CORNER_ALL, 3.0f);
	c.y += 2;
	ui2_do_label(&c, boxtext, 12.0f, 0);
	ui2_do_label(&t, text, 14.0f, -1);	
}

static void ui2_draw_checkbox(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_draw_checkbox_common(id, text, checked?"X":"", r);
}


static void ui2_draw_checkbox_number(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	char buf[16];
	sprintf(buf, "%d", checked);
	ui2_draw_checkbox_common(id, text, buf, r);
}

int ui2_do_edit_box(void *id, const RECT *rect, char *str, int str_size, bool hidden=false)
{
    int inside = ui_mouse_inside(rect);
	int r = 0;
	static int at_index = 0;

	if(ui_last_active_item() == id)
	{
		int len = strlen(str);

		if (inside && ui_mouse_button(0))
		{
			int mx_rel = (int)(ui_mouse_x() - rect->x);

			for (int i = 1; i <= len; i++)
			{
				if (gfx_pretty_text_width(14.0f, str, i) + 10 > mx_rel)
				{
					at_index = i - 1;
					break;
				}

				if (i == len)
					at_index = len;
			}
		}

		if (at_index > len)
			at_index = len;
			
		for(int i = 0; i < inp_num_events(); i++)
		{
			INPUTEVENT e = inp_get_event(i);
			char c = e.ch;
			int k = e.key;

			if (!(c >= 0 && c < 32))
			{
				if (len < str_size - 1 && at_index < str_size - 1)
				{
					memmove(str + at_index + 1, str + at_index, len - at_index + 1);
					str[at_index] = c;
					at_index++;
				}
			}

			if (k == KEY_BACKSPACE && at_index > 0)
			{
				memmove(str + at_index - 1, str + at_index, len - at_index + 1);
				at_index--;
			}
			else if (k == KEY_DEL && at_index < len)
				memmove(str + at_index, str + at_index + 1, len - at_index);
			else if (k == KEY_ENTER)
				ui_clear_last_active_item();
			else if (k == KEY_LEFT && at_index > 0)
				at_index--;
			else if (k == KEY_RIGHT && at_index < len)
				at_index++;
			else if (k == KEY_HOME)
				at_index = 0;
			else if (k == KEY_END)
				at_index = len;
		}
		
		r = 1;
	}

	int box_type;
	if (ui_active_item() == id || ui_hot_item() == id || ui_last_active_item() == id)
		box_type = GUI_BOX_SCREEN_INFO;
	else
		box_type = GUI_BOX_SCREEN_TEXTBOX;

	bool just_got_active = false;
	
	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
			ui_set_active_item(0);
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			if (ui_last_active_item() != id)
				just_got_active = true;
			ui_set_active_item(id);
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	RECT textbox = *rect;
	ui2_draw_rect(&textbox, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
	ui2_vmargin(&textbox, 5.0f, &textbox);
	
	const char *display_str = str;
	char stars[128];
	
	if(hidden)
	{
		unsigned s = strlen(str);
		if(s >= sizeof(stars))
			s = sizeof(stars)-1;
		memset(stars, '*', s);
		stars[s] = 0;
		display_str = stars;
	}
	
	ui2_do_label(&textbox, display_str, 14, -1);

	if (ui_last_active_item() == id && !just_got_active)
	{
		float w = gfx_pretty_text_width(14.0f, display_str, at_index);
		textbox.x += w*ui2_scale();
		ui2_do_label(&textbox, "_", 14, -1);
	}

	return r;
}

float ui2_do_scrollbar_v(const void *id, const RECT *rect, float current)
{
	RECT handle;
	static float offset_y;
	ui2_hsplit_t(rect, 33, &handle, 0);

	handle.y += (rect->h-handle.h)*current;

	/* logic */
    float ret = current;
    int inside = ui_mouse_inside(&handle);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
			ui_set_active_item(0);
		
		float min = rect->y;
		float max = rect->h-handle.h;
		float cur = ui_mouse_y()-offset_y;
		ret = (cur-min)/max;
		if(ret < 0.0f) ret = 0.0f;
		if(ret > 1.0f) ret = 1.0f;
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			ui_set_active_item(id);
			offset_y = ui_mouse_y()-handle.y;
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	// render
	RECT rail;
	ui2_vmargin(rect, 5.0f, &rail);
	ui2_draw_rect(&rail, vec4(1,1,1,0.25f), 0, 0.0f);

	RECT slider = handle;
	slider.w = rail.x-slider.x;
	ui2_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_L, 2.5f);
	slider.x = rail.x+rail.w;
	ui2_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_R, 2.5f);

	slider = handle;
	ui2_margin(&slider, 5.0f, &slider);
	ui2_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_ALL, 2.5f);
	
    return ret;
}



float ui2_do_scrollbar_h(const void *id, const RECT *rect, float current)
{
	RECT handle;
	static float offset_x;
	ui2_vsplit_l(rect, 33, &handle, 0);

	handle.x += (rect->w-handle.w)*current;

	/* logic */
    float ret = current;
    int inside = ui_mouse_inside(&handle);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
			ui_set_active_item(0);
		
		float min = rect->x;
		float max = rect->w-handle.w;
		float cur = ui_mouse_x()-offset_x;
		ret = (cur-min)/max;
		if(ret < 0.0f) ret = 0.0f;
		if(ret > 1.0f) ret = 1.0f;
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			ui_set_active_item(id);
			offset_x = ui_mouse_x()-handle.x;
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	// render
	RECT rail;
	ui2_hmargin(rect, 5.0f, &rail);
	ui2_draw_rect(&rail, vec4(1,1,1,0.25f), 0, 0.0f);

	RECT slider = handle;
	slider.h = rail.y-slider.y;
	ui2_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_T, 2.5f);
	slider.y = rail.y+rail.h;
	ui2_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_B, 2.5f);

	slider = handle;
	ui2_margin(&slider, 5.0f, &slider);
	ui2_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_ALL, 2.5f);
	
    return ret;
}

int ui2_do_key_reader(void *id, const RECT *rect, int key)
{
	// process
	static bool mouse_released = true;
	int inside = ui_mouse_inside(rect);
	int new_key = key;
	
	if(!ui_mouse_button(0))
		mouse_released = true;

	if(ui_active_item() == id)
	{
		for(int i = 0; i < inp_num_events(); i++)
		{
			INPUTEVENT e = inp_get_event(i);
			if(e.key && e.key != KEY_ESC)
			{
				new_key = e.key;
				ui_set_active_item(0);
				mouse_released = false;
				inp_clear_events();
				break;
			}
		}
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0) && mouse_released)
			ui_set_active_item(id);
	}
	
	if(inside)
		ui_set_hot_item(id);

	// draw
	if (ui_active_item() == id)
		ui2_draw_keyselect_button(id, "???", 0, rect, 0);
	else
		ui2_draw_keyselect_button(id, inp_key_name(key), 0, rect, 0);
	return new_key;
}


static int menu2_render_menubar(RECT r)
{
	RECT box = r;
	RECT button;
	
	int active_page = config.ui_page;
	int new_page = -1;
	if(menu_game_active)
		active_page = -1;
	
	if(client_state() == CLIENTSTATE_OFFLINE)
	{
		if(0) // this is not done yet
		{
			ui2_vsplit_l(&box, 90.0f, &button, &box);
			static int news_button=0;
			if (ui2_do_button(&news_button, "News", active_page==PAGE_NEWS, &button, ui2_draw_menu_tab_button, 0))
				new_page = PAGE_NEWS;
			ui2_vsplit_l(&box, 30.0f, 0, &box); 
		}
	}
	else
	{
		ui2_vsplit_l(&box, 90.0f, &button, &box);
		static int game_button=0;
		if (ui2_do_button(&game_button, "Game", menu_game_active, &button, ui2_draw_menu_tab_button, 0))
			menu_game_active = true;
			
		ui2_vsplit_l(&box, 30.0f, 0, &box);
	}
		
	ui2_vsplit_l(&box, 110.0f, &button, &box);
	static int internet_button=0;
	if (ui2_do_button(&internet_button, "Internet", active_page==PAGE_INTERNET, &button, ui2_draw_menu_tab_button, 0))
	{
		client_serverbrowse_refresh(0);
		new_page = PAGE_INTERNET;
	}

	ui2_vsplit_l(&box, 4.0f, 0, &box);
	ui2_vsplit_l(&box, 90.0f, &button, &box);
	static int lan_button=0;
	if (ui2_do_button(&lan_button, "LAN", active_page==PAGE_LAN, &button, ui2_draw_menu_tab_button, 0))
	{
		client_serverbrowse_refresh(1);
		new_page = PAGE_LAN;
	}

	if(0) // this one is not done yet
	{
		ui2_vsplit_l(&box, 4.0f, 0, &box);
		ui2_vsplit_l(&box, 120.0f, &button, &box);
		static int favorites_button=0;
		if (ui2_do_button(&favorites_button, "Favorites", active_page==PAGE_FAVORITES, &button, ui2_draw_menu_tab_button, 0))
			new_page  = PAGE_FAVORITES;
	}

	/*
	ui2_vsplit_r(&box, 110.0f, &box, &button);
	static int system_button=0;
	if (ui2_do_button(&system_button, "System", config.ui_page==PAGE_SYSTEM, &button, ui2_draw_menu_tab_button, 0))
		config.ui_page = PAGE_SYSTEM;
		
	ui2_vsplit_r(&box, 30.0f, &box, 0);
	*/
	
	ui2_vsplit_r(&box, 110.0f, &box, &button);
	static int quit_button=0;
	if (ui2_do_button(&quit_button, "Quit", 0, &button, ui2_draw_menu_tab_button, 0))
		popup = POPUP_QUIT;

	ui2_vsplit_r(&box, 10.0f, &box, &button);
	ui2_vsplit_r(&box, 110.0f, &box, &button);
	static int settings_button=0;
	if (ui2_do_button(&settings_button, "Settings", active_page==PAGE_SETTINGS, &button, ui2_draw_menu_tab_button, 0))
		new_page = PAGE_SETTINGS;
	
	if(new_page != -1)
	{
		config.ui_page = new_page;
		menu_game_active = false;
	}
		
	return 0;
}

static void menu2_render_background()
{
	RECT s = *ui2_screen();

	gfx_texture_set(-1);
	gfx_quads_begin();
		vec4 bottom(gui_color.r*0.6f, gui_color.g*0.6f, gui_color.b*0.6f, 1.0f);
		vec4 top(gui_color.r, gui_color.g, gui_color.b, 1.0f);
		gfx_setcolorvertex(0, top.r, top.g, top.b, top.a);
		gfx_setcolorvertex(1, top.r, top.g, top.b, top.a);
		gfx_setcolorvertex(2, bottom.r, bottom.g, bottom.b, bottom.a);
		gfx_setcolorvertex(3, bottom.r, bottom.g, bottom.b, bottom.a);
		gfx_quads_drawTL(0, 0, s.w, s.h);
	gfx_quads_end();
	
	if(data->images[IMAGE_BANNER].id != 0)
	{
		gfx_texture_set(data->images[IMAGE_BANNER].id);
		gfx_quads_begin();
		gfx_setcolor(0,0,0,0.05f);
		gfx_quads_setrotation(-pi/4+0.15f);
		gfx_quads_draw(400, 300, 1000, 250);
		gfx_quads_end();
	}
}

extern "C" void *gfx_font_set;

void render_loading(float percent)
{
    if (1)
    {
        static FONT_SET font_set;
        static bool first = true;

        gfx_font_set = &font_set;

        if (first)
        {
			int before = gfx_memory_usage();
            font_set_load(&font_set, "fonts/default_font%d.tfnt", "fonts/default_font%d.png", "fonts/default_font%d_b.png", 14, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 36);
            first = false;
			int after = gfx_memory_usage();
			dbg_msg("font", "gfx memory usage: %d", after-before);
        }
    }

	// need up date this here to get correct
	vec3 rgb = hsl_to_rgb(vec3(config.ui_color_hue/255.0f, config.ui_color_sat/255.0f, config.ui_color_lht/255.0f));
	gui_color = vec4(rgb.r, rgb.g, rgb.b, config.ui_color_alpha/255.0f);
	
    RECT screen = *ui2_screen();
	gfx_mapscreen(screen.x, screen.y, screen.w, screen.h);
	
	menu2_render_background();

	float tw;

	float w = 700;
	float h = 200;
	float x = screen.w/2-w/2;
	float y = screen.h/2-h/2;

	gfx_blend_normal();

	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.50f);
	draw_round_rect(x, y, w, h, 40.0f);
	gfx_quads_end();

	const char *caption = "Loading";

	tw = gfx_pretty_text_width(48.0f, caption, -1);
	RECT r;
	r.x = x+w/2;
	r.y = y+20;
	ui_do_label(&r, caption, 48.0f, 0, -1);

	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(1,1,1,0.75f);
	draw_round_rect(x+40, y+h-75, (w-80)*percent, 25, 5.0f);
	gfx_quads_end();

	gfx_swap();
}

static void menu2_render_serverbrowser(RECT main_view)
{
	ui2_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);
	
	RECT view;
	ui2_margin(&main_view, 10.0f, &view);
	
	RECT headers;
	RECT filters;
	RECT status;
	RECT toolbox;
	RECT server_details;
	RECT server_scoreboard;

	//ui2_hsplit_t(&view, 20.0f, &status, &view);
	ui2_hsplit_b(&view, 110.0f, &view, &filters);

	// split off a piece for details and scoreboard
	ui2_vsplit_r(&view, 200.0f, &view, &server_details);

	// server list
	ui2_hsplit_t(&view, 20.0f, &headers, &view);
	//ui2_hsplit_b(&view, 110.0f, &view, &filters);
	ui2_hsplit_b(&view, 5.0f, &view, 0);
	ui2_hsplit_b(&view, 20.0f, &view, &status);

	//ui2_vsplit_r(&filters, 300.0f, &filters, &toolbox);
	//ui2_vsplit_r(&filters, 150.0f, &filters, 0);

	ui2_vsplit_mid(&filters, &filters, &toolbox);
	ui2_vsplit_r(&filters, 50.0f, &filters, 0);
	
	// split of the scrollbar
	ui2_draw_rect(&headers, vec4(1,1,1,0.25f), CORNER_T, 5.0f);
	ui2_vsplit_r(&headers, 20.0f, &headers, 0);
	
	struct column
	{
		int id;
		int sort;
		const char *caption;
		int direction;
		float width;
		int flags;
		RECT rect;
		RECT spacer;
	};
	
	enum
	{
		FIXED=1,
		SPACER=2,
		
		COL_FLAGS=0,
		COL_NAME,
		COL_GAMETYPE,
		COL_MAP,
		COL_PLAYERS,
		COL_PING,
		COL_PROGRESS,
		COL_VERSION,
	};
	
	static column cols[] = {
		{-1,			-1,						" ",		-1, 10.0f, 0, {0}, {0}},
		{COL_FLAGS,		-1,						" ",		-1, 20.0f, 0, {0}, {0}},
		{COL_NAME,		BROWSESORT_NAME,		"Name",		0, 300.0f, 0, {0}, {0}},
		{COL_GAMETYPE,	BROWSESORT_GAMETYPE,	"Type",		1, 50.0f, 0, {0}, {0}},
		{COL_MAP,		BROWSESORT_MAP,			"Map", 		1, 100.0f, 0, {0}, {0}},
		{COL_PLAYERS,	BROWSESORT_NUMPLAYERS,	"Players",	1, 60.0f, 0, {0}, {0}},
		{-1,			-1,						" ",		1, 10.0f, 0, {0}, {0}},
		//{COL_VERSION,	-1,						"Ver",		1, 45.0f, FIXED, {0}, {0}},
		{-1,			-1,						" ",		1, 5.0f, 0, {0}, {0}},
		//{COL_PROGRESS,	BROWSESORT_PROGRESSION,	"%",		1, 20.0f, FIXED, {0}, {0}},
		{COL_PING,		BROWSESORT_PING,		"Ping",		1, 40.0f, FIXED, {0}, {0}},
	};
	
	int num_cols = sizeof(cols)/sizeof(column);
	
	// do layout
	for(int i = 0; i < num_cols; i++)
	{
		if(cols[i].direction == -1)
		{
			ui2_vsplit_l(&headers, cols[i].width, &cols[i].rect, &headers);
			
			if(i+1 < num_cols)
			{
				//cols[i].flags |= SPACER;
				ui2_vsplit_l(&headers, 2, &cols[i].spacer, &headers);
			}
		}
	}
	
	for(int i = num_cols-1; i >= 0; i--)
	{
		if(cols[i].direction == 1)
		{
			ui2_vsplit_r(&headers, cols[i].width, &headers, &cols[i].rect);
			ui2_vsplit_r(&headers, 2, &headers, &cols[i].spacer);
		}
	}
	
	for(int i = 0; i < num_cols; i++)
	{
		if(cols[i].direction == 0)
			cols[i].rect = headers;
	}
	
	// do headers
	for(int i = 0; i < num_cols; i++)
	{
		if(ui2_do_button(cols[i].caption, cols[i].caption, config.b_sort == cols[i].sort, &cols[i].rect, ui2_draw_grid_header, 0))
		{
			if(cols[i].sort != -1)
			{
				if(config.b_sort == cols[i].sort)
					config.b_sort_order ^= 1;
				else
					config.b_sort_order = 0;
				config.b_sort = cols[i].sort;
			}
		}
	}
	
	ui2_draw_rect(&view, vec4(0,0,0,0.15f), 0, 0);
	
	RECT scroll;
	ui2_vsplit_r(&view, 15, &view, &scroll);
	
	int num_servers = client_serverbrowse_sorted_num();
	
	int num = (int)(view.h/cols[0].rect.h);
	static int scrollbar = 0;
	static float scrollvalue = 0;
	//static int selected_index = -1;
	ui2_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui2_do_scrollbar_v(&scrollbar, &scroll, scrollvalue);
	
	int scrollnum = num_servers-num+10;
	if(scrollnum > 0)
	{
		if(inp_key_presses(KEY_MOUSE_WHEEL_UP))
			scrollvalue -= 1.0f/scrollnum;
		if(inp_key_presses(KEY_MOUSE_WHEEL_DOWN))
			scrollvalue += 1.0f/scrollnum;
			
		if(scrollvalue < 0) scrollvalue = 0;
		if(scrollvalue > 1) scrollvalue = 1;
	}
	else
		scrollnum = 0;

	// set clipping
	ui2_clip_enable(&view);
	
	int start = (int)(scrollnum*scrollvalue);
	if(start < 0)
		start = 0;
	
	RECT original_view = view;
	view.y -= scrollvalue*scrollnum*cols[0].rect.h;
	
	int new_selected = -1;
	int selected_index = -1;
	
	for (int i = 0; i < num_servers; i++)
	{
		int item_index = i;
		SERVER_INFO *item = client_serverbrowse_sorted_get(item_index);
		RECT row;
        RECT select_hit_box;
			
		int selected = strcmp(item->address, config.ui_server_address) == 0; //selected_index==item_index;
		
		if(selected)
		{
			selected_index = i;
			
			// selected server, draw the players on it
			RECT whole;
			int h = (item->num_players+2)/3;
			
			ui2_hsplit_t(&view, 25.0f+h*15.0f, &whole, &view);

            select_hit_box = whole;
			
			RECT r = whole;
			ui2_margin(&r, 1.5f, &r);
			ui2_draw_rect(&r, vec4(1,1,1,0.5f), CORNER_ALL, 4.0f);
			
			ui2_hsplit_t(&whole, 20.0f, &row, &whole);
			ui2_vsplit_l(&whole, 50.0f, 0, &whole);

			
			for(int p = 0; p < item->num_players; p+=3)
			{
				RECT player_row;
				RECT player_rect;
				RECT player_score;
				RECT player_name;
				ui2_hsplit_t(&whole, 15.0f, &player_row, &whole);
				
				for(int a = 0; a < 3; a++)
				{
					if(p+a >= item->num_players)
						break;
						
					ui2_vsplit_l(&player_row, 170.0f, &player_rect, &player_row);
					ui2_vsplit_l(&player_rect, 30.0f, &player_score, &player_name);
					ui2_vsplit_l(&player_name, 10.0f, 0, &player_name);
					char buf[32];
					sprintf(buf, "%d", item->player_scores[p+a]);
					ui2_do_label(&player_score, buf, 12.0f, 1);
					ui2_do_label(&player_name, item->player_names[p+a], 12.0f, -1);
				}
			}
			
			//k += h*3/4;
		}
		else
        {
			ui2_hsplit_t(&view, 20.0f, &row, &view);
            select_hit_box = row;
        }

		// make sure that only those in view can be selected
		if(row.y+row.h > original_view.y)
		{
			if(select_hit_box.y < original_view.y) // clip the selection
			{
				select_hit_box.h -= original_view.y-select_hit_box.y;
				select_hit_box.y = original_view.y;
			}
			
			if(ui2_do_button(item, "", selected, &select_hit_box, 0, 0))
			{
				new_selected = item_index;
			}
		}
		
		// check if we need to do more
		if(row.y > original_view.y+original_view.h)
			break;

		for(int c = 0; c < num_cols; c++)
		{
			RECT button;
			char temp[64];
			button.x = cols[c].rect.x;
			button.y = row.y;
			button.h = row.h;
			button.w = cols[c].rect.w;
			
			//int s = 0;
			int id = cols[c].id;

			//s = ui2_do_button(item, "L", l, &button, ui2_draw_browse_icon, 0);
			
			if(id == COL_FLAGS)
			{
				if(item->flags&1)
					ui2_draw_browse_icon(0x100, &button);
			}
			else if(id == COL_NAME)
				ui2_do_label(&button, item->name, 15.0f, -1);
			else if(id == COL_MAP)
				ui2_do_label(&button, item->map, 15.0f, -1);
			else if(id == COL_PLAYERS)
			{
				sprintf(temp, "%i/%i", item->num_players, item->max_players);
				ui2_do_label(&button, temp, 15.0f, 1);
			}
			else if(id == COL_PING)
			{
				sprintf(temp, "%i", item->latency);
				ui2_do_label(&button, temp, 15.0f, 1);
			}
			else if(id == COL_PROGRESS)
			{
				if(item->progression > 100)
					item->progression = 100;
				ui2_draw_browse_icon(item->progression, &button);
			}
			else if(id == COL_VERSION)
			{
				const char *version = item->version;
				if(strcmp(version, "0.3 e2d7973c6647a13c") == 0) // TODO: remove me later on
					version = "0.3.0";
				ui2_do_label(&button, version, 15.0f, 1);
			}			
			else if(id == COL_GAMETYPE)
			{
				const char *type = "???";
				if(item->game_type == GAMETYPE_DM) type = "DM";
				else if(item->game_type == GAMETYPE_TDM) type = "TDM";
				else if(item->game_type == GAMETYPE_CTF) type = "CTF";
				ui2_do_label(&button, type, 15.0f, 0);
			}
		}
	}

	ui2_clip_disable();
	
	if(new_selected != -1)
	{
		// select the new server
		SERVER_INFO *item = client_serverbrowse_sorted_get(new_selected);
		strncpy(config.ui_server_address, item->address, sizeof(config.ui_server_address));
		if(inp_mouse_doubleclick())
			client_connect(config.ui_server_address);
	}
	
	SERVER_INFO *selected_server = client_serverbrowse_sorted_get(selected_index);
	RECT server_header;

	ui2_vsplit_l(&server_details, 10.0f, 0x0, &server_details);

	// split off a piece to use for scoreboard
	ui2_hsplit_t(&server_details, 140.0f, &server_details, &server_scoreboard);
	ui2_hsplit_b(&server_details, 10.0f, &server_details, 0x0);

	// server details
	ui2_hsplit_t(&server_details, 20.0f, &server_header, &server_details);
	ui2_draw_rect(&server_header, vec4(1,1,1,0.25f), CORNER_T, 4.0f);
	ui2_draw_rect(&server_details, vec4(0,0,0,0.15f), CORNER_B, 4.0f);
	ui2_vsplit_l(&server_header, 8.0f, 0x0, &server_header);
	ui2_do_label(&server_header, "Server Details: ", 14.0f, -1);

	ui2_vsplit_l(&server_details, 5.0f, 0x0, &server_details);

	ui2_margin(&server_details, 3.0f, &server_details);

	if (selected_server)
	{
		const float row_height = 18.0f;
		RECT row;
		static char *labels[] = { "Version:", "Game Type:", "Progression:", "Ping:" };

		ui2_hsplit_t(&server_details, row_height, &row, &server_details);
		ui2_do_label(&row, selected_server->name, 15.0f, -1);

		ui2_hsplit_t(&server_details, row_height, &row, &server_details);
		ui2_vsplit_l(&row, 1.0f, 0x0, &row);
		ui2_do_label(&row, selected_server->address, 14.0f, -1);

		RECT left_column;
		RECT right_column;

		ui2_vsplit_l(&server_details, 5.0f, 0x0, &server_details);
		ui2_vsplit_l(&server_details, 80.0f, &left_column, &right_column);

		for (int i = 0; i < 4; i++)
		{
			ui2_hsplit_t(&left_column, 15.0f, &row, &left_column);
			ui2_do_label(&row, labels[i], 13.0f, -1);
		}

		ui2_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui2_do_label(&row, selected_server->version, 13.0f, -1);

		ui2_hsplit_t(&right_column, 15.0f, &row, &right_column);
		static char *game_types[] = { "DM", "TDM", "CTF" };
		if (selected_server->game_type >= 0 && selected_server->game_type < (int)(sizeof(game_types)/sizeof(*game_types)))
			ui2_do_label(&row, game_types[selected_server->game_type], 13.0f, -1);

		char temp[16];

		if(selected_server->progression < 0)
			sprintf(temp, "N/A");
		else
			sprintf(temp, "%d%%", selected_server->progression);
		ui2_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui2_do_label(&row, temp, 13.0f, -1);

		sprintf(temp, "%d", selected_server->latency);
		ui2_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui2_do_label(&row, temp, 13.0f, -1);
	}
	
	// server scoreboard
	ui2_hsplit_b(&server_scoreboard, 10.0f, &server_scoreboard, 0x0);
	ui2_hsplit_t(&server_scoreboard, 20.0f, &server_header, &server_scoreboard);
	ui2_draw_rect(&server_header, vec4(1,1,1,0.25f), CORNER_T, 4.0f);
	ui2_draw_rect(&server_scoreboard, vec4(0,0,0,0.15f), CORNER_B, 4.0f);
	ui2_vsplit_l(&server_header, 8.0f, 0x0, &server_header);
	ui2_do_label(&server_header, "Scoreboard: ", 14.0f, -1);

	ui2_vsplit_l(&server_scoreboard, 5.0f, 0x0, &server_scoreboard);

	ui2_margin(&server_scoreboard, 3.0f, &server_scoreboard);

	if (selected_server)
	{
		for (int i = 0; i < selected_server->num_players; i++)
		{
			RECT row;
			char temp[16];
			ui2_hsplit_t(&server_scoreboard, 16.0f, &row, &server_scoreboard);

			sprintf(temp, "%d", selected_server->player_scores[i]);
			ui2_do_label(&row, temp, 14.0f, -1);

			ui2_vsplit_l(&row, 25.0f, 0x0, &row);
			ui2_do_label(&row, selected_server->player_names[i], 14.0f, -1);
		}
	}
	
	RECT button;
	RECT types;
	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	ui2_do_label(&button, "Quick search: ", 14.0f, -1);
	ui2_vsplit_l(&button, 95.0f, 0, &button);
	ui2_do_edit_box(&config.b_filter_string, &button, config.b_filter_string, sizeof(config.b_filter_string));

	ui2_vsplit_l(&filters, 180.0f, &filters, &types);

	// render filters
	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui2_do_button(&config.b_filter_empty, "Has people playing", config.b_filter_empty, &button, ui2_draw_checkbox, 0))
		config.b_filter_empty ^= 1;

	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui2_do_button(&config.b_filter_full, "Server not full", config.b_filter_full, &button, ui2_draw_checkbox, 0))
		config.b_filter_full ^= 1;

	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui2_do_button(&config.b_filter_pw, "No password", config.b_filter_pw, &button, ui2_draw_checkbox, 0))
		config.b_filter_pw ^= 1;

	ui2_hsplit_t(&filters, 2.0f, &button, &filters); // ping
	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	{
		RECT editbox;
		ui2_vsplit_l(&button, 40.0f, &editbox, &button);
		ui2_vsplit_l(&button, 5.0f, &button, &button);
		
		char buf[8];
		sprintf(buf, "%d", config.b_filter_ping);
		ui2_do_edit_box(&config.b_filter_ping, &editbox, buf, sizeof(buf));
		config.b_filter_ping = atoi(buf);
		
		ui2_do_label(&button, "Maximum ping", 14.0f, -1);
	}

	ui2_hsplit_t(&types, 20.0f, &button, &types);
	if (ui2_do_button(&config.b_filter_gametype, "DM", config.b_filter_gametype&(1<<GAMETYPE_DM), &button, ui2_draw_checkbox, 0))
		config.b_filter_gametype ^= (1<<GAMETYPE_DM);

	ui2_hsplit_t(&types, 20.0f, &button, &types);
	if (ui2_do_button((char *)&config.b_filter_gametype + 1, "TDM", config.b_filter_gametype&(1<<GAMETYPE_TDM), &button, ui2_draw_checkbox, 0))
		config.b_filter_gametype ^= (1<<GAMETYPE_TDM);

	ui2_hsplit_t(&types, 20.0f, &button, &types);
	if (ui2_do_button((char *)&config.b_filter_gametype + 2, "CTF", config.b_filter_gametype&(1<<GAMETYPE_CTF), &button, ui2_draw_checkbox, 0))
		config.b_filter_gametype ^= (1<<GAMETYPE_CTF);

	// render status
	ui2_draw_rect(&status, vec4(1,1,1,0.25f), CORNER_B, 5.0f);
	ui2_vmargin(&status, 50.0f, &status);
	char buf[128];
	sprintf(buf, "%d of %d servers", client_serverbrowse_sorted_num(), client_serverbrowse_num());
	ui2_do_label(&status, buf, 14.0f, -1);

	// render toolbox
	{
		RECT buttons, button;
		ui2_hsplit_b(&toolbox, 25.0f, &toolbox, &buttons);

		ui2_vsplit_r(&buttons, 100.0f, &buttons, &button);
		ui2_vmargin(&button, 2.0f, &button);
		static int join_button = 0;
		if(ui2_do_button(&join_button, "Connect", 0, &button, ui2_draw_menu_button, 0))
			client_connect(config.ui_server_address);

		ui2_vsplit_r(&buttons, 20.0f, &buttons, &button);
		ui2_vsplit_r(&buttons, 100.0f, &buttons, &button);
		ui2_vmargin(&button, 2.0f, &button);
		static int refresh_button = 0;
		if(ui2_do_button(&refresh_button, "Refresh", 0, &button, ui2_draw_menu_button, 0))
		{
			if(config.ui_page == PAGE_INTERNET)
				client_serverbrowse_refresh(0);
			else if(config.ui_page == PAGE_LAN)
				client_serverbrowse_refresh(1);
		}
		
		ui2_hsplit_t(&toolbox, 20.0f, &button, &toolbox);
		ui2_do_label(&button, "Host address:", 14.0f, -1);
		ui2_vsplit_l(&button, 100.0f, 0, &button);
		ui2_do_edit_box(&config.ui_server_address, &button, config.ui_server_address, sizeof(config.ui_server_address));
	}
}

static void menu2_render_settings_player(RECT main_view)
{
	RECT button;
	RECT skinselection;
	ui2_vsplit_l(&main_view, 300.0f, &main_view, &skinselection);


	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);

	// render settings
	{	
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		ui2_do_label(&button, "Name:", 14.0, -1);
		ui2_vsplit_l(&button, 80.0f, 0, &button);
		ui2_vsplit_l(&button, 180.0f, &button, 0);
		ui2_do_edit_box(config.player_name, &button, config.player_name, sizeof(config.player_name));

		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		if (ui2_do_button(&config.cl_dynamic_camera, "Dynamic camera", config.cl_dynamic_camera, &button, ui2_draw_checkbox, 0))
			config.cl_dynamic_camera ^= 1;
			
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		if (ui2_do_button(&config.cl_autoswitch_weapons, "Switch weapon on pickup", config.cl_autoswitch_weapons, &button, ui2_draw_checkbox, 0))
			config.cl_autoswitch_weapons ^= 1;
			
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		if (ui2_do_button(&config.cl_nameplates, "Show name plates", config.cl_nameplates, &button, ui2_draw_checkbox, 0))
			config.cl_nameplates ^= 1;

		if(config.cl_nameplates)
		{
			ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
			ui2_vsplit_l(&button, 15.0f, 0, &button);
			if (ui2_do_button(&config.cl_nameplates_always, "Always show name plates", config.cl_nameplates_always, &button, ui2_draw_checkbox, 0))
				config.cl_nameplates_always ^= 1;
		}
			
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		if (ui2_do_button(&config.player_color_body, "Custom colors", config.player_use_custom_color, &button, ui2_draw_checkbox, 0))
			config.player_use_custom_color = config.player_use_custom_color?0:1;
		
		if(config.player_use_custom_color)
		{
			int *colors[2];
			colors[0] = &config.player_color_body;
			colors[1] = &config.player_color_feet;
			
			const char *parts[] = {"Body", "Feet"};
			const char *labels[] = {"Hue", "Sat.", "Lht."};
			static int color_slider[2][3] = {{0}};
			//static float v[2][3] = {{0, 0.5f, 0.25f}, {0, 0.5f, 0.25f}};
				
			for(int i = 0; i < 2; i++)
			{
				RECT text;
				ui2_hsplit_t(&main_view, 20.0f, &text, &main_view);
				ui2_vsplit_l(&text, 15.0f, 0, &text);
				ui2_do_label(&text, parts[i], 14.0f, -1);
				
				int prevcolor = *colors[i];
				int color = 0;
				for(int s = 0; s < 3; s++)
				{
					RECT text;
					ui2_hsplit_t(&main_view, 19.0f, &button, &main_view);
					ui2_vsplit_l(&button, 30.0f, 0, &button);
					ui2_vsplit_l(&button, 30.0f, &text, &button);
					ui2_vsplit_r(&button, 5.0f, &button, 0);
					ui2_hsplit_t(&button, 4.0f, 0, &button);
					
					float k = ((prevcolor>>((2-s)*8))&0xff)  / 255.0f;
					k = ui2_do_scrollbar_h(&color_slider[i][s], &button, k);
					color <<= 8;
					color += clamp((int)(k*255), 0, 255);
					ui2_do_label(&text, labels[s], 15.0f, -1);
					 
				}
				
				*colors[i] = color;
				ui2_hsplit_t(&main_view, 5.0f, 0, &main_view);
			}
		}
	}
		
	// draw header
	RECT header, footer;
	ui2_hsplit_t(&skinselection, 20, &header, &skinselection);
	ui2_draw_rect(&header, vec4(1,1,1,0.25f), CORNER_T, 5.0f); 
	ui2_do_label(&header, "Skins", 18.0f, 0);

	// draw footers	
	ui2_hsplit_b(&skinselection, 20, &skinselection, &footer);
	ui2_draw_rect(&footer, vec4(1,1,1,0.25f), CORNER_B, 5.0f); 
	ui2_vsplit_l(&footer, 10.0f, 0, &footer);

	// modes
	ui2_draw_rect(&skinselection, vec4(0,0,0,0.15f), 0, 0);

	RECT scroll;
	ui2_vsplit_r(&skinselection, 15, &skinselection, &scroll);

	RECT list = skinselection;
	ui2_hsplit_t(&list, 50, &button, &list);
	
	int num = (int)(skinselection.h/button.h);
	static float scrollvalue = 0;
	static int scrollbar = 0;
	ui2_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui2_do_scrollbar_v(&scrollbar, &scroll, scrollvalue);

	int start = (int)((skin_num()-num)*scrollvalue);
	if(start < 0)
		start = 0;
		
	animstate state;
	anim_eval(&data->animations[ANIM_BASE], 0, &state);
	anim_eval_add(&state, &data->animations[ANIM_IDLE], 0, 1.0f);
	//anim_eval_add(&state, &data->animations[ANIM_WALK], fmod(client_localtime(), 1.0f), 1.0f);
		
	for(int i = start; i < start+num && i < skin_num(); i++)
	{
		const skin *s = skin_get(i);
		char buf[128];
		sprintf(buf, "%s", s->name);
		int selected = 0;
		if(strcmp(s->name, config.player_skin) == 0)
			selected = 1;
		
		tee_render_info info;
		info.texture = s->org_texture;
		info.color_body = vec4(1,1,1,1);
		info.color_feet = vec4(1,1,1,1);
		if(config.player_use_custom_color)
		{
			info.color_body = skin_get_color(config.player_color_body);
			info.color_feet = skin_get_color(config.player_color_feet);
			info.texture = s->color_texture;
		}
			
		info.size = ui2_scale()*50.0f;
		
		RECT icon;
		RECT text;
		ui2_vsplit_l(&button, 50.0f, &icon, &text);
		
		if(ui2_do_button(s, "", selected, &button, ui2_draw_list_row, 0))
			config_set_player_skin(&config, s->name);
		
		ui2_hsplit_t(&text, 12.0f, 0, &text); // some margin from the top
		ui2_do_label(&text, buf, 18.0f, 0);
		
		ui2_hsplit_t(&icon, 5.0f, 0, &icon); // some margin from the top
		render_tee(&state, &info, 0, vec2(1, 0), vec2(icon.x+icon.w/2, icon.y+icon.h/2));
		
		ui2_hsplit_t(&list, 50, &button, &list);
	}
}

typedef void (*assign_func_callback)(CONFIGURATION *config, int value);

static void menu2_render_settings_controls(RECT main_view)
{
	ui2_vsplit_l(&main_view, 300.0f, &main_view, 0);
	
	{
		RECT button, label;
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		ui2_vsplit_l(&button, 110.0f, &label, &button);
		ui2_do_label(&label, "Mouse sens.", 14.0f, -1);
		ui2_hmargin(&button, 2.0f, &button);
		config.inp_mousesens = (int)(ui2_do_scrollbar_h(&config.inp_mousesens, &button, config.inp_mousesens/500.0f)*500.0f);
		//*key.key = ui2_do_key_reader(key.key, &button, *key.key);
		ui2_hsplit_t(&main_view, 20.0f, 0, &main_view);
	}
	
	typedef struct 
	{
		char name[32];
		int *key;
	} KEYINFO;

	const KEYINFO keys[] = 
	{
		{ "Move Left:", &config.key_move_left },
		{ "Move Right:", &config.key_move_right },
		{ "Jump:", &config.key_jump },
		{ "Fire:", &config.key_fire },
		{ "Hook:", &config.key_hook },
		{ "Hammer:", &config.key_weapon1 },
		{ "Pistol:", &config.key_weapon2 },
		{ "Shotgun:", &config.key_weapon3 },
		{ "Grenade:", &config.key_weapon4 },
		{ "Next Weapon:", &config.key_next_weapon },
		{ "Prev. Weapon:", &config.key_prev_weapon },
		{ "Emoticon:", &config.key_emoticon },
		{ "Chat:", &config.key_chat },
		{ "Team Chat:", &config.key_teamchat },
		{ "Console:", &config.key_console },
		{ "Remote Console:", &config.key_remoteconsole },
		{ "Screenshot:", &config.key_screenshot },
	};

	const int key_count = sizeof(keys) / sizeof(KEYINFO);
	
	for (int i = 0; i < key_count; i++)
    {
		KEYINFO key = keys[i];
    	RECT button, label;
    	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
    	ui2_vsplit_l(&button, 110.0f, &label, &button);
    	
		ui2_do_label(&label, key.name, 14.0f, -1);
		*key.key = ui2_do_key_reader(key.key, &button, *key.key);
    	ui2_hsplit_t(&main_view, 5.0f, 0, &main_view);
    }	
}

static void menu2_render_settings_graphics(RECT main_view)
{
	RECT button;
	char buf[128];
	
	static const int MAX_RESOLUTIONS = 256;
	static VIDEO_MODE modes[MAX_RESOLUTIONS];
	static int num_modes = -1;
	
	if(num_modes == -1)
		num_modes = gfx_get_video_modes(modes, MAX_RESOLUTIONS);
	
	RECT modelist;
	ui2_vsplit_l(&main_view, 300.0f, &main_view, &modelist);
	
	// draw allmodes switch
	RECT header, footer;
	ui2_hsplit_t(&modelist, 20, &button, &modelist);
	if (ui2_do_button(&config.gfx_display_all_modes, "Show only supported", config.gfx_display_all_modes^1, &button, ui2_draw_checkbox, 0))
	{
		config.gfx_display_all_modes ^= 1;
		num_modes = gfx_get_video_modes(modes, MAX_RESOLUTIONS);
	}
	
	// draw header
	ui2_hsplit_t(&modelist, 20, &header, &modelist);
	ui2_draw_rect(&header, vec4(1,1,1,0.25f), CORNER_T, 5.0f); 
	ui2_do_label(&header, "Display Modes", 14.0f, 0);

	// draw footers	
	ui2_hsplit_b(&modelist, 20, &modelist, &footer);
	sprintf(buf, "Current: %dx%d %d bit", config.gfx_screen_width, config.gfx_screen_height, config.gfx_color_depth);
	ui2_draw_rect(&footer, vec4(1,1,1,0.25f), CORNER_B, 5.0f); 
	ui2_vsplit_l(&footer, 10.0f, 0, &footer);
	ui2_do_label(&footer, buf, 14.0f, -1);

	// modes
	ui2_draw_rect(&modelist, vec4(0,0,0,0.15f), 0, 0);

	RECT scroll;
	ui2_vsplit_r(&modelist, 15, &modelist, &scroll);

	RECT list = modelist;
	ui2_hsplit_t(&list, 20, &button, &list);
	
	int num = (int)(modelist.h/button.h);
	static float scrollvalue = 0;
	static int scrollbar = 0;
	ui2_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui2_do_scrollbar_v(&scrollbar, &scroll, scrollvalue);

	int start = (int)((num_modes-num)*scrollvalue);
	if(start < 0)
		start = 0;
		
	for(int i = start; i < start+num && i < num_modes; i++)
	{
		int depth = modes[i].red+modes[i].green+modes[i].blue;
		if(depth < 16)
			depth = 16;
		else if(depth > 16)
			depth = 24;
			
		int selected = 0;
		if(config.gfx_color_depth == depth &&
			config.gfx_screen_width == modes[i].width &&
			config.gfx_screen_height == modes[i].height)
		{
			selected = 1;
		}
		
		sprintf(buf, "  %dx%d %d bit", modes[i].width, modes[i].height, depth);
		if(ui2_do_button(&modes[i], buf, selected, &button, ui2_draw_list_row, 0))
		{
			config.gfx_color_depth = depth;
			config.gfx_screen_width = modes[i].width;
			config.gfx_screen_height = modes[i].height;
			if(!selected)
				need_restart = true;
		}
		
		ui2_hsplit_t(&list, 20, &button, &list);
	}
	
	
	// switches
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_fullscreen, "Fullscreen", config.gfx_fullscreen, &button, ui2_draw_checkbox, 0))
	{
		config.gfx_fullscreen ^= 1;
		need_restart = true;
	}

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_vsync, "V-Sync", config.gfx_vsync, &button, ui2_draw_checkbox, 0))
		config.gfx_vsync ^= 1;

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_fsaa_samples, "FSAA samples", config.gfx_fsaa_samples, &button, ui2_draw_checkbox_number, 0))
	{
		if(config.gfx_fsaa_samples < 2) config.gfx_fsaa_samples = 2;
		else if(config.gfx_fsaa_samples < 4) config.gfx_fsaa_samples = 4;
		else if(config.gfx_fsaa_samples < 6) config.gfx_fsaa_samples = 6;
		else if(config.gfx_fsaa_samples < 8) config.gfx_fsaa_samples = 8;
		else if(config.gfx_fsaa_samples < 16) config.gfx_fsaa_samples = 16;
		else if(config.gfx_fsaa_samples >= 16) config.gfx_fsaa_samples = 0;
		need_restart = true;
	}
		
	ui2_hsplit_t(&main_view, 40.0f, &button, &main_view);
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_texture_quality, "Quality Textures", config.gfx_texture_quality, &button, ui2_draw_checkbox, 0))
	{
		config.gfx_texture_quality ^= 1;
		need_restart = true;
	}

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_texture_compression, "Texture Compression", config.gfx_texture_compression, &button, ui2_draw_checkbox, 0))
	{
		config.gfx_texture_compression ^= 1;
		need_restart = true;
	}

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_high_detail, "High Detail", config.gfx_high_detail, &button, ui2_draw_checkbox, 0))
		config.gfx_high_detail ^= 1;

	//
	
	RECT text;
	ui2_hsplit_t(&main_view, 20.0f, 0, &main_view);
	ui2_hsplit_t(&main_view, 20.0f, &text, &main_view);
	//ui2_vsplit_l(&text, 15.0f, 0, &text);
	ui2_do_label(&text, "UI Color", 14.0f, -1);
	
	const char *labels[] = {"Hue", "Sat.", "Lht.", "Alpha"};
	int *color_slider[4] = {&config.ui_color_hue, &config.ui_color_sat, &config.ui_color_lht, &config.ui_color_alpha};
	for(int s = 0; s < 4; s++)
	{
		RECT text;
		ui2_hsplit_t(&main_view, 19.0f, &button, &main_view);
		ui2_vmargin(&button, 15.0f, &button);
		ui2_vsplit_l(&button, 30.0f, &text, &button);
		ui2_vsplit_r(&button, 5.0f, &button, 0);
		ui2_hsplit_t(&button, 4.0f, 0, &button);
		
		float k = (*color_slider[s]) / 255.0f;
		k = ui2_do_scrollbar_h(color_slider[s], &button, k);
		*color_slider[s] = (int)(k*255.0f);
		ui2_do_label(&text, labels[s], 15.0f, -1);
	}		
}

static void menu2_render_settings_sound(RECT main_view)
{
	RECT button;
	ui2_vsplit_l(&main_view, 300.0f, &main_view, 0);
	
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.snd_enable, "Use Sounds", config.snd_enable, &button, ui2_draw_checkbox, 0))
	{
		config.snd_enable ^= 1;
		need_restart = true;
	}
	
	if(!config.snd_enable)
		return;
	
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.snd_nonactive_mute, "Mute when not active", config.snd_nonactive_mute, &button, ui2_draw_checkbox, 0))
		config.snd_nonactive_mute ^= 1;
		
	// sample rate box
	{
		char buf[64];
		sprintf(buf, "%d", config.snd_rate);
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		ui2_do_label(&button, "Sample Rate", 14.0f, -1);
		ui2_vsplit_l(&button, 110.0f, 0, &button);
		ui2_vsplit_l(&button, 180.0f, &button, 0);
		ui2_do_edit_box(&config.snd_rate, &button, buf, sizeof(buf));
		int before = config.snd_rate;
		config.snd_rate = atoi(buf);
		
		if(config.snd_rate != before)
			need_restart = true;

		if(config.snd_rate < 1)
			config.snd_rate = 1;
	}
	
	// volume slider
	{
		RECT button, label;
		ui2_hsplit_t(&main_view, 5.0f, &button, &main_view);
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		ui2_vsplit_l(&button, 110.0f, &label, &button);
		ui2_hmargin(&button, 2.0f, &button);
		ui2_do_label(&label, "Sound Volume", 14.0f, -1);
		config.snd_volume = (int)(ui2_do_scrollbar_h(&config.snd_volume, &button, config.snd_volume/100.0f)*100.0f);
		ui2_hsplit_t(&main_view, 20.0f, 0, &main_view);
	}
}


static void menu2_render_settings_network(RECT main_view)
{
	RECT button;
	ui2_vsplit_l(&main_view, 300.0f, &main_view, 0);
	
	{
		ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
		ui2_do_label(&button, "Rcon Password", 14.0, -1);
		ui2_vsplit_l(&button, 110.0f, 0, &button);
		ui2_vsplit_l(&button, 180.0f, &button, 0);
		ui2_do_edit_box(&config.rcon_password, &button, config.rcon_password, sizeof(config.rcon_password), true);
	}
}

static void menu2_render_settings(RECT main_view)
{
	static int settings_page = 0;
	
	// render background
	RECT temp, tabbar;
	ui2_vsplit_r(&main_view, 120.0f, &main_view, &tabbar);
	ui2_draw_rect(&main_view, color_tabbar_active, CORNER_B|CORNER_TL, 10.0f);
	ui2_hsplit_t(&tabbar, 50.0f, &temp, &tabbar);
	ui2_draw_rect(&temp, color_tabbar_active, CORNER_R, 10.0f);
	
	ui2_hsplit_t(&main_view, 10.0f, 0, &main_view);
	
	RECT button;
	
	const char *tabs[] = {"Player", "Controls", "Network", "Graphics", "Sound"};
	int num_tabs = (int)(sizeof(tabs)/sizeof(*tabs));

	for(int i = 0; i < num_tabs; i++)
	{
		ui2_hsplit_t(&tabbar, 10, &button, &tabbar);
		ui2_hsplit_t(&tabbar, 26, &button, &tabbar);
		if(ui2_do_button(tabs[i], tabs[i], settings_page == i, &button, ui2_draw_settings_tab_button, 0))
			settings_page = i;
	}
	
	ui2_margin(&main_view, 10.0f, &main_view);
	
	if(settings_page == 0)
		menu2_render_settings_player(main_view);
	else if(settings_page == 1)
		menu2_render_settings_controls(main_view);
	else if(settings_page == 2)
		menu2_render_settings_network(main_view);
	else if(settings_page == 3)
		menu2_render_settings_graphics(main_view);
	else if(settings_page == 4)
		menu2_render_settings_sound(main_view);

	if(need_restart)
	{
		RECT restart_warning;
		ui2_hsplit_b(&main_view, 40, &main_view, &restart_warning);
		ui2_do_label(&restart_warning, "You must restart Teewars for all settings to take effect.", 15.0f, -1, 220);
	}
}

static void menu2_render_news(RECT main_view)
{
	ui2_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);
}


static void menu2_render_game(RECT main_view)
{
	RECT button;
	ui2_hsplit_t(&main_view, 45.0f, &main_view, 0);
	ui2_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);

	ui2_hsplit_t(&main_view, 10.0f, 0, &main_view);
	ui2_hsplit_t(&main_view, 25.0f, &main_view, 0);
	ui2_vmargin(&main_view, 10.0f, &main_view);
	
	ui2_vsplit_r(&main_view, 120.0f, &main_view, &button);
	static int disconnect_button = 0;
	if(ui2_do_button(&disconnect_button, "Disconnect", 0, &button, ui2_draw_menu_button, 0))
		client_disconnect();

	if(local_info && gameobj)
	{
		if(local_info->team != -1)
		{
			ui2_vsplit_l(&main_view, 10.0f, &button, &main_view);
			ui2_vsplit_l(&main_view, 120.0f, &button, &main_view);
			static int spectate_button = 0;
			if(ui2_do_button(&spectate_button, "Spectate", 0, &button, ui2_draw_menu_button, 0))
			{
				config.cl_team = -1;
				menu_active = false;
			}
		}
		
		if(gameobj->gametype == GAMETYPE_DM)
		{
			if(local_info->team != 0)
			{
				ui2_vsplit_l(&main_view, 10.0f, &button, &main_view);
				ui2_vsplit_l(&main_view, 120.0f, &button, &main_view);
				static int spectate_button = 0;
				if(ui2_do_button(&spectate_button, "Join Game", 0, &button, ui2_draw_menu_button, 0))
				{
					config.cl_team = 0;
					menu_active = false;
				}
			}						
		}
		else
		{
			if(local_info->team != 0)
			{
				ui2_vsplit_l(&main_view, 10.0f, &button, &main_view);
				ui2_vsplit_l(&main_view, 120.0f, &button, &main_view);
				static int spectate_button = 0;
				if(ui2_do_button(&spectate_button, "Join Red", 0, &button, ui2_draw_menu_button, 0))
				{
					config.cl_team = 0;
					menu_active = false;
				}
			}

			if(local_info->team != 1)
			{
				ui2_vsplit_l(&main_view, 10.0f, &button, &main_view);
				ui2_vsplit_l(&main_view, 120.0f, &button, &main_view);
				static int spectate_button = 0;
				if(ui2_do_button(&spectate_button, "Join Blue", 0, &button, ui2_draw_menu_button, 0))
				{
					config.cl_team = 1;
					menu_active = false;
				}
			}
		}
	}
}

void menu_do_disconnected()
{
	popup = POPUP_NONE;
	if(client_error_string() && client_error_string()[0] != 0)
	{
		if(strstr(client_error_string(), "password"))
		{
			popup = POPUP_PASSWORD;
			ui_set_hot_item(&config.password);
			ui_set_active_item(&config.password);
		}
		else
			popup = POPUP_DISCONNECTED;
	}
}

void menu_do_connecting()
{
	popup = POPUP_CONNECTING;
}

void menu_do_connected()
{
	popup = POPUP_NONE;
}

void menu_init()
{
	if(config.cl_show_welcome)
		popup = POPUP_FIRST_LAUNCH;
	config.cl_show_welcome = 0;
}

int menu2_render()
{
	if(0)
	{
		gfx_mapscreen(0,0,10*4/3.0f,10);
		gfx_clear(gui_color.r, gui_color.g, gui_color.b);
		
		animstate state;
		anim_eval(&data->animations[ANIM_BASE], 0, &state);
		anim_eval_add(&state, &data->animations[ANIM_IDLE], 0, 1.0f);
		//anim_eval_add(&state, &data->animations[ANIM_WALK], fmod(client_localtime(), 1.0f), 1.0f);
			
		for(int i = 0; i < skin_num(); i++)
		{
			float x = (i/8)*3;
			float y = (i%8);
			for(int c = 0; c < 2; c++)
			{
				//int colors[2] = {54090, 10998628};
				//int colors[2] = {65432, 9895832}; // NEW
				int colors[2] = {65387, 10223467}; // NEW
				
				tee_render_info info;
				info.texture = skin_get(i)->color_texture;
				info.color_feet = info.color_body = skin_get_color(colors[c]);
				//info.color_feet = info.color_body = vec4(1,1,1,1);
				info.size = 1.0f; //ui2_scale()*16.0f;
				//render_tee(&state, &info, 0, vec2(sinf(client_localtime()*3), cosf(client_localtime()*3)), vec2(1+x+c,1+y));
				render_tee(&state, &info, 0, vec2(1,0), vec2(1+x+c,1+y));
			}
		}
			
		return 0;
	}
	
    RECT screen = *ui2_screen();
	gfx_mapscreen(screen.x, screen.y, screen.w, screen.h);

    if (0)
    {
        gfx_clear(0.65f,0.78f,0.9f);
        //gfx_pretty_text_color(0.0f, 0.0f, 0.0f, 1.0f);
        //gfx_pretty_text_color(0.0f, 0.0f, 0.0f, 1.0f);

        for (int i = 0; i < 24; i++)
        {
            float size = i * 0.5 + 8;
            char temp[64];
            sprintf(temp, "%f: Ingen tomte i jul", size);
            gfx_pretty_text(50, 10 + i*int(size), size, temp, -1);
            gfx_text(gfx_font_set, 400, 10 + i*int(size), size, temp, -1);
        }

        return 0;
    }

	
	static bool first = true;
	if(first)
	{
		if(config.ui_page == PAGE_INTERNET)
			client_serverbrowse_refresh(0);
		else if(config.ui_page == PAGE_LAN)
			client_serverbrowse_refresh(1);
		first = false;
	}
	
	if(client_state() == CLIENTSTATE_ONLINE)
	{
		color_tabbar_inactive = color_tabbar_inactive_ingame;
		color_tabbar_active = color_tabbar_active_ingame;
	}
	else
	{
		menu2_render_background();
		color_tabbar_inactive = color_tabbar_inactive_outgame;
		color_tabbar_active = color_tabbar_active_outgame;
	}
	
	RECT tab_bar;
	RECT main_view;

	// some margin around the screen
	ui2_margin(&screen, 10.0f, &screen);
	
	if(popup == POPUP_NONE)
	{
		// do tab bar
		ui2_hsplit_t(&screen, 26.0f, &tab_bar, &main_view);
		ui2_vmargin(&tab_bar, 20.0f, &tab_bar);
		menu2_render_menubar(tab_bar);
			
		// render current page
		if(menu_game_active)
			menu2_render_game(main_view);
		else if(config.ui_page == PAGE_NEWS)
			menu2_render_news(main_view);
		else if(config.ui_page == PAGE_INTERNET)
			menu2_render_serverbrowser(main_view);
		else if(config.ui_page == PAGE_LAN)
			menu2_render_serverbrowser(main_view);
		else if(config.ui_page == PAGE_FAVORITES)
			menu2_render_serverbrowser(main_view);
		else if(config.ui_page == PAGE_SETTINGS)
			menu2_render_settings(main_view);
	}
	else
	{
		// make sure that other windows doesn't do anything funnay!
		//ui_set_hot_item(0);
		//ui_set_active_item(0);
		const char *title = "";
		const char *extra_text = "";
		const char *button_text = "";
		
		if(popup == POPUP_CONNECTING)
		{
			title = "Connecting to";
			extra_text = config.ui_server_address;  // TODO: query the client about the address
			button_text = "Abort";
		}
		else if(popup == POPUP_DISCONNECTED)
		{
			title = "Disconnected";
			extra_text = client_error_string();
			button_text = "Ok";
		}
		else if(popup == POPUP_PASSWORD)
		{
			title = "Password Error";
			extra_text = client_error_string();
			button_text = "Try Again";
		}
		else if(popup == POPUP_QUIT)
		{
			title = "Quit";
			extra_text = "Are you sure that you want to quit?";
		}
		else if(popup == POPUP_FIRST_LAUNCH)
		{
			title = "Welcome to Teewars";
			extra_text =
			"As this is the first time you launch Teewars, please enter your nick name below. "
			"It's recommended that you check the settings to adjust them to your liking "
			"before joining a server.";
			button_text = "Ok";
		}
		
		RECT box, part;
		box = screen;
		ui2_vmargin(&box, 150.0f, &box);
		ui2_hmargin(&box, 150.0f, &box);
		
		// render the box
		ui2_draw_rect(&box, vec4(0,0,0,0.5f), CORNER_ALL, 15.0f);
		 
		ui2_hsplit_t(&box, 20.f, &part, &box);
		ui2_hsplit_t(&box, 24.f, &part, &box);
		ui2_do_label(&part, title, 24.f, 0);
		ui2_hsplit_t(&box, 20.f, &part, &box);
		ui2_hsplit_t(&box, 24.f, &part, &box);
		ui2_vmargin(&part, 20.f, &part);
		ui2_do_label(&part, extra_text, 20.f, -1, (int)part.w);

		if(popup == POPUP_QUIT)
		{
			RECT yes, no;
			ui2_hsplit_b(&box, 20.f, &box, &part);
			ui2_hsplit_b(&box, 24.f, &box, &part);
			ui2_vmargin(&part, 80.0f, &part);
			
			ui2_vsplit_mid(&part, &no, &yes);
			
			ui2_vmargin(&yes, 20.0f, &yes);
			ui2_vmargin(&no, 20.0f, &no);

			static int button_abort = 0;
			if(ui2_do_button(&button_abort, "No", 0, &no, ui2_draw_menu_button, 0) || inp_key_down(KEY_ESC))
				popup = POPUP_NONE;

			static int button_tryagain = 0;
			if(ui2_do_button(&button_tryagain, "Yes", 0, &yes, ui2_draw_menu_button, 0) || inp_key_down(KEY_ENTER))
				client_quit();
		}
		else if(popup == POPUP_PASSWORD)
		{
			RECT label, textbox, tryagain, abort;
			
			ui2_hsplit_b(&box, 20.f, &box, &part);
			ui2_hsplit_b(&box, 24.f, &box, &part);
			ui2_vmargin(&part, 80.0f, &part);
			
			ui2_vsplit_mid(&part, &abort, &tryagain);
			
			ui2_vmargin(&tryagain, 20.0f, &tryagain);
			ui2_vmargin(&abort, 20.0f, &abort);
			
			static int button_abort = 0;
			if(ui2_do_button(&button_abort, "Abort", 0, &abort, ui2_draw_menu_button, 0) || inp_key_down(KEY_ESC))
				popup = POPUP_NONE;

			static int button_tryagain = 0;
			if(ui2_do_button(&button_tryagain, "Try again", 0, &tryagain, ui2_draw_menu_button, 0) || inp_key_down(KEY_ENTER))
			{
				client_connect(config.ui_server_address);
			}
			
			ui2_hsplit_b(&box, 60.f, &box, &part);
			ui2_hsplit_b(&box, 24.f, &box, &part);
			
			ui2_vsplit_l(&part, 60.0f, 0, &label);
			ui2_vsplit_l(&label, 100.0f, 0, &textbox);
			ui2_vsplit_l(&textbox, 20.0f, 0, &textbox);
			ui2_vsplit_r(&textbox, 60.0f, &textbox, 0);
			ui2_do_label(&label, "Password:", 20, -1);
			ui2_do_edit_box(&config.password, &textbox, config.password, sizeof(config.password), true);
		}
		else if(popup == POPUP_FIRST_LAUNCH)
		{
			RECT label, textbox;
			
			ui2_hsplit_b(&box, 20.f, &box, &part);
			ui2_hsplit_b(&box, 24.f, &box, &part);
			ui2_vmargin(&part, 80.0f, &part);
			
			static int enter_button = 0;
			if(ui2_do_button(&enter_button, "Enter", 0, &part, ui2_draw_menu_button, 0) || inp_key_down(KEY_ENTER))
				popup = POPUP_NONE;
			
			ui2_hsplit_b(&box, 60.f, &box, &part);
			ui2_hsplit_b(&box, 24.f, &box, &part);
			
			ui2_vsplit_l(&part, 60.0f, 0, &label);
			ui2_vsplit_l(&label, 100.0f, 0, &textbox);
			ui2_vsplit_l(&textbox, 20.0f, 0, &textbox);
			ui2_vsplit_r(&textbox, 60.0f, &textbox, 0);
			ui2_do_label(&label, "Nickname:", 20, -1);
			ui2_do_edit_box(&config.player_name, &textbox, config.player_name, sizeof(config.player_name));			
		}
		else
		{
			ui2_hsplit_b(&box, 20.f, &box, &part);
			ui2_hsplit_b(&box, 24.f, &box, &part);
			ui2_vmargin(&part, 120.0f, &part);

			static int button = 0;
			if(ui2_do_button(&button, button_text, 0, &part, ui2_draw_menu_button, 0) || inp_key_down(KEY_ESC) || inp_key_down(KEY_ENTER))
			{
				if(popup == POPUP_CONNECTING)
					client_disconnect();
				popup = POPUP_NONE;
			}
		}
	}
	
	return 0;
}

void modmenu_render()
{
	static int mouse_x = 0;
	static int mouse_y = 0;

	// update colors

	vec3 rgb = hsl_to_rgb(vec3(config.ui_color_hue/255.0f, config.ui_color_sat/255.0f, config.ui_color_lht/255.0f));
	gui_color = vec4(rgb.r, rgb.g, rgb.b, config.ui_color_alpha/255.0f);

	color_tabbar_inactive_outgame = vec4(0,0,0,0.25f);
	color_tabbar_active_outgame = vec4(0,0,0,0.5f);

	color_ingame_scale_i = 0.5f;
	color_ingame_scale_a = 0.2f;
	color_tabbar_inactive_ingame = vec4(
		gui_color.r*color_ingame_scale_i,
		gui_color.g*color_ingame_scale_i,
		gui_color.b*color_ingame_scale_i,
		gui_color.a*0.8f);
	
	color_tabbar_active_ingame = vec4(
		gui_color.r*color_ingame_scale_a,
		gui_color.g*color_ingame_scale_a,
		gui_color.b*color_ingame_scale_a,
		gui_color.a);


    // handle mouse movement
    float mx, my;
    {
        int rx, ry;
        inp_mouse_relative(&rx, &ry);
        mouse_x += rx;
        mouse_y += ry;
        if(mouse_x < 0) mouse_x = 0;
        if(mouse_y < 0) mouse_y = 0;
        if(mouse_x > gfx_screenwidth()) mouse_x = gfx_screenwidth();
        if(mouse_y > gfx_screenheight()) mouse_y = gfx_screenheight();
            
        // update the ui
        RECT *screen = ui2_screen();
        mx = (mouse_x/(float)gfx_screenwidth())*screen->w;
        my = (mouse_y/(float)gfx_screenheight())*screen->h;
            
        int buttons = 0;
        if(inp_key_pressed(KEY_MOUSE_1)) buttons |= 1;
        if(inp_key_pressed(KEY_MOUSE_2)) buttons |= 2;
        if(inp_key_pressed(KEY_MOUSE_3)) buttons |= 4;
            
        ui_update(mx,my,mx*3.0f,my*3.0f,buttons);
    }
    
	menu2_render();
	
    gfx_texture_set(data->images[IMAGE_CURSOR].id);
    gfx_quads_begin();
    gfx_setcolor(1,1,1,1);
    gfx_quads_drawTL(mx,my,24,24);
    gfx_quads_end();

	inp_clear_events();
}
