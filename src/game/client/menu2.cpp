#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <game/math.h>
#include <game/vmath.h>

extern "C" {
	#include <engine/system.h>
	#include <engine/interface.h>
	#include <engine/config.h>
	#include <engine/client/ui.h>
}

#include "../mapres.h"
#include "../version.h"
#include "../game_protocol.h"

#include "mapres_image.h"
#include "mapres_tilemap.h"

#include "data.h"
#include <mastersrv/mastersrv.h>

extern data_container *data;

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
	CORNER_TL=1,
	CORNER_TR=2,
	CORNER_BL=4,
	CORNER_BR=8,
	
	CORNER_T=CORNER_TL|CORNER_TR,
	CORNER_B=CORNER_BL|CORNER_BR,
	CORNER_R=CORNER_TR|CORNER_BR,
	CORNER_L=CORNER_TL|CORNER_BL,
	
	CORNER_ALL=CORNER_T|CORNER_B,
	
	PAGE_NEWS=0,
	PAGE_INTERNET,
	PAGE_LAN,
	PAGE_FAVORITES,
	PAGE_SETTINGS,
	PAGE_GAME,
};

typedef struct 
{
    float x, y, w, h;
} RECT;

static float scale = 1.0f;
static RECT screen = { 0.0f, 0.0f, 800.0f, 600.0f };

RECT *ui2_screen()
{
    return &screen;
}

void ui2_set_scale(float s)
{
    scale = s;
}

float ui2_scale()
{
    return scale;
}

void ui2_hsplit_t(const RECT *original, float cut, RECT *top, RECT *bottom)
{
    RECT r = *original;
    cut *= scale;

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
    cut *= scale;

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

void ui2_vsplit_l(const RECT *original, float cut, RECT *left, RECT *right)
{
    RECT r = *original;
    cut *= scale;

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
    cut *= scale;

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
	cut *= scale;

    other_rect->x = r.x + cut;
    other_rect->y = r.y + cut;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h - 2*cut;
}

void ui2_vmargin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= scale;

    other_rect->x = r.x + cut;
    other_rect->y = r.y;
    other_rect->w = r.w - 2*cut;
    other_rect->h = r.h;
}

void ui2_hmargin(const RECT *original, float cut, RECT *other_rect)
{
    RECT r = *original;
	cut *= scale;

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
    int inside = ui_mouse_inside(r->x,r->y,r->w,r->h);

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

    draw_func(id, text, checked, r, extra);
    return ret;
}


void ui2_do_label(const RECT *r, const char *text, float size, int align)
{
    gfx_blend_normal();
    size *= ui2_scale();
    if(align == 0)
    {
    	float tw = gfx_pretty_text_width(size, text, -1);
    	gfx_pretty_text(r->x + r->w/2-tw/2, r->y, size, text, -1);
	}
	else if(align < 0)
    	gfx_pretty_text(r->x, r->y, size, text, -1);
	else if(align > 0)
	{
    	float tw = gfx_pretty_text_width(size, text, -1);
    	gfx_pretty_text(r->x + r->w-tw, r->y, size, text, -1);
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

static void ui2_draw_menu_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_draw_rect(r, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
	ui2_do_label(r, text, 24, 0);
}


static void ui2_draw_keyselect_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	ui2_draw_rect(r, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
	ui2_do_label(r, text, 18, 0);
}

static void ui2_draw_menu_tab_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
		ui2_draw_rect(r, color_tabbar_active, CORNER_T, 10.0f);
	else
		ui2_draw_rect(r, color_tabbar_inactive, CORNER_T, 10.0f);
	ui2_do_label(r, text, 26, 0);
}


static void ui2_draw_settings_tab_button(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
		ui2_draw_rect(r, color_tabbar_active, CORNER_R, 10.0f);
	else
		ui2_draw_rect(r, color_tabbar_inactive, CORNER_R, 10.0f);
	ui2_do_label(r, text, 24, 0);
}

static void ui2_draw_grid_header(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
		ui2_draw_rect(r, vec4(1,1,1,0.5f), CORNER_T, 5.0f);
	//else
	//	ui2_draw_rect(r, vec4(1,1,1,0.1f), CORNER_T, 5.0f);
	RECT t;
	ui2_vsplit_l(r, 5.0f, 0, &t);
	ui2_do_label(&t, text, 18, -1);
}

static void ui2_draw_grid_cell(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	//ui2_draw_round_rect(r, 2.0f, vec4(1,1,1,1));
	ui2_do_label(r, text, 18, -1);
}

static void ui2_draw_list_row(const void *id, const char *text, int checked, const RECT *r, void *extra)
{
	if(checked)
	{
		RECT sr = *r;
		ui2_margin(&sr, 1.5f, &sr);
		ui2_draw_rect(&sr, vec4(1,1,1,0.5f), CORNER_ALL, 4.0f);
	}
	ui2_do_label(r, text, 18, -1);
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
	ui2_do_label(&c, boxtext, 16, 0);
	ui2_do_label(&t, text, 18, -1);	
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

int ui2_do_edit_box(void *id, const RECT *rect, char *str, int str_size)
{
    int inside = ui_mouse_inside(rect->x,rect->y,rect->w,rect->h);
	int r = 0;
	static int at_index = 0;

	if(ui_last_active_item() == id)
	{
		int c = inp_last_char();
		int k = inp_last_key();
		int len = strlen(str);

		if (inside && ui_mouse_button(0))
		{
			int mx_rel = (int)(ui_mouse_x() - rect->x);

			for (int i = 1; i <= len; i++)
			{
				if (gfx_pretty_text_width(18.0f, str, i) + 10 > mx_rel)
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
	ui2_do_label(&textbox, str, 18, -1);

	if (ui_last_active_item() == id && !just_got_active)
	{
		float w = gfx_pretty_text_width(18.0f, str, at_index);
		textbox.x += w*ui2_scale();
		ui2_do_label(&textbox, "_", 18, -1);
	}

	return r;
}

float ui2_do_scrollbar_v(const void *id, const RECT *rect, float current)
{
	RECT handle;
	static float offset_y;
	ui2_hsplit_t(rect, 33, &handle, 0);

	handle.y += (rect->h-handle.h)*current;

	//dbg_msg("scroll", "%f %f %f %f", handle.x,handle.y,handle.w,handle.h);
	
	/* logic */
    float ret = current;
    int inside = ui_mouse_inside(handle.x,handle.y,handle.w,handle.h);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
		{
			//if(inside)
			//	ret = 1;
			ui_set_active_item(0);
		}
		
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

int ui2_do_key_reader(void *id, const RECT *rect, int key)
{
	// process
	static bool mouse_released = true;
	int inside = ui_mouse_inside(rect->x, rect->y, rect->w, rect->h);
	int new_key = key;
	
	if(!ui_mouse_button(0))
		mouse_released = true;

	if(ui_active_item() == id)
	{
		int k = inp_last_key();
		if (k)
		{
			new_key = k;
			ui_set_active_item(0);
			mouse_released = false;
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
	
	if(client_state() == CLIENTSTATE_OFFLINE)
	{
		if(0) // this is not done yet
		{
			ui2_vsplit_l(&box, 90.0f, &button, &box);
			static int news_button=0;
			if (ui2_do_button(&news_button, "News", config.ui_page==PAGE_NEWS, &button, ui2_draw_menu_tab_button, 0))
				config.ui_page = PAGE_NEWS;
			ui2_vsplit_l(&box, 30.0f, 0, &box); 
		}
	}
	else
	{
		ui2_vsplit_l(&box, 90.0f, &button, &box);
		static int game_button=0;
		if (ui2_do_button(&game_button, "Game", config.ui_page==PAGE_GAME, &button, ui2_draw_menu_tab_button, 0))
			config.ui_page = PAGE_GAME;
		ui2_vsplit_l(&box, 30.0f, 0, &box); 
	}
		
	ui2_vsplit_l(&box, 110.0f, &button, &box);
	static int internet_button=0;
	if (ui2_do_button(&internet_button, "Internet", config.ui_page==PAGE_INTERNET, &button, ui2_draw_menu_tab_button, 0))
		config.ui_page = PAGE_INTERNET;

	ui2_vsplit_l(&box, 4.0f, 0, &box);
	ui2_vsplit_l(&box, 90.0f, &button, &box);
	static int lan_button=0;
	if (ui2_do_button(&lan_button, "LAN", config.ui_page==PAGE_LAN, &button, ui2_draw_menu_tab_button, 0))
		config.ui_page = PAGE_LAN;

	if(0) // this one is not done yet
	{
		ui2_vsplit_l(&box, 4.0f, 0, &box);
		ui2_vsplit_l(&box, 120.0f, &button, &box);
		static int favorites_button=0;
		if (ui2_do_button(&favorites_button, "Favorites", config.ui_page==PAGE_FAVORITES, &button, ui2_draw_menu_tab_button, 0))
			config.ui_page = PAGE_FAVORITES;
	}

	ui2_vsplit_r(&box, 110.0f, &box, &button);
	static int settings_button=0;
	if (ui2_do_button(&settings_button, "Settings", config.ui_page==PAGE_SETTINGS, &button, ui2_draw_menu_tab_button, 0))
		config.ui_page = PAGE_SETTINGS;
		
	return 0;
}

static void menu2_render_background()
{
	//gfx_clear(0.65f,0.78f,0.9f);
	gfx_clear(gui_color.r, gui_color.g, gui_color.b);
	//gfx_clear(0.78f,0.9f,0.65f);
	
	gfx_texture_set(data->images[IMAGE_BANNER].id);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.05f);
	gfx_quads_setrotation(-pi/4+0.15f);
	gfx_quads_draw(400, 300, 1000, 250);
	gfx_quads_end();
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
	
	//ui2_hsplit_t(&view, 20.0f, &status, &view);
	ui2_hsplit_t(&view, 20.0f, &headers, &view);
	ui2_hsplit_b(&view, 90.0f, &view, &filters);
	ui2_hsplit_b(&view, 5.0f, &view, 0);
	ui2_hsplit_b(&view, 20.0f, &view, &status);

	ui2_vsplit_r(&filters, 300.0f, &filters, &toolbox);
	ui2_vsplit_r(&filters, 150.0f, &filters, 0);
	
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
	};
	
	static column cols[] = {
		{-1,			-1,						" ",		-1, 10.0f, 0, {0}, {0}},
		{COL_FLAGS,		-1,						" ",		-1, 15.0f, 0, {0}, {0}},
		{COL_NAME,		BROWSESORT_NAME,		"Name",		0, 300.0f, 0, {0}, {0}},
		{COL_GAMETYPE,	BROWSESORT_GAMETYPE,	"Type",		1, 50.0f, 0, {0}, {0}},
		{COL_MAP,		BROWSESORT_MAP,			"Map", 		1, 100.0f, 0, {0}, {0}},
		{COL_PLAYERS,	BROWSESORT_NUMPLAYERS,	"Players",	1, 60.0f, 0, {0}, {0}},
		{COL_PROGRESS,	BROWSESORT_PROGRESSION,	"%",		1, 40.0f, FIXED, {0}, {0}},
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
				config.b_sort = cols[i].sort;
		}
	}
	
	
	ui2_draw_rect(&view, vec4(0,0,0,0.15f), 0, 0);
	
	RECT scroll;
	ui2_vsplit_r(&view, 15, &view, &scroll);
	
	int num_servers = client_serverbrowse_sorted_num();
	
	int num = (int)(view.h/cols[0].rect.h);
	static int scrollbar = 0;
	static float scrollvalue = 0;
	static int selected_index = -1;
	ui2_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui2_do_scrollbar_v(&scrollbar, &scroll, scrollvalue);

	int start = (int)((num_servers-num)*scrollvalue);
	if(start < 0)
		start = 0;
	
	//int r = -1;
	int new_selected = selected_index;
	
	for (int i = start, k = 0; i < num_servers && k < num; i++, k++)
	{
		int item_index = i;
		SERVER_INFO *item = client_serverbrowse_sorted_get(item_index);
		RECT row;
			
		int l = selected_index==item_index;
		
		if(l)
		{
			// selected server, draw the players on it
			RECT whole;
			int h = (item->num_players+2)/3;
			
			ui2_hsplit_t(&view, 25.0f+h*15.0f, &whole, &view);
			
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
					ui2_do_label(&player_score, buf, 16.0f, 1);
					ui2_do_label(&player_name, item->player_names[p+a], 16.0f, -1);
				}
			}
			
			k += h*3/4;
			i += h*3/4;
		}
		else
			ui2_hsplit_t(&view, 20.0f, &row, &view);

		for(int c = 0; c < num_cols; c++)
		{
			RECT button;
			char temp[64];
			button.x = cols[c].rect.x;
			button.y = row.y;
			button.h = row.h;
			button.w = cols[c].rect.w;
			
			int s = 0;
			int id = cols[c].id;

			if(id == COL_FLAGS)
			{
				if(item->flags&1)
					s = ui2_do_button(item, "P", l, &button, ui2_draw_grid_cell, 0);
			}
			else if(id == COL_NAME)
				s = ui2_do_button(item, item->name, l, &button, ui2_draw_grid_cell, 0);
			else if(id == COL_MAP)
				s = ui2_do_button(item, item->map, l, &button, ui2_draw_grid_cell, 0);
			else if(id == COL_PLAYERS)
			{
				sprintf(temp, "%i/%i", item->num_players, item->max_players);
				s = ui2_do_button(item, temp, l, &button, ui2_draw_grid_cell, 0);
			}
			else if(id == COL_PING)
			{
				sprintf(temp, "%i", item->latency);
				s = ui2_do_button(item, temp, l, &button, ui2_draw_grid_cell, 0);
			}
			else if(id == COL_PROGRESS)
			{
				sprintf(temp, "%i", item->progression);
				s = ui2_do_button(item, temp, l, &button, ui2_draw_grid_cell, 0);
			}
			else if(id == COL_GAMETYPE)
			{
				const char *type = "???";
				if(item->game_type == GAMETYPE_DM) type = "DM";
				else if(item->game_type == GAMETYPE_TDM) type = "TDM";
				else if(item->game_type == GAMETYPE_CTF) type = "CTF";
				s = ui2_do_button(item, type, l, &button, ui2_draw_grid_cell, 0);
			}
			
			if(s)
			{
				new_selected = item_index;
				strncpy(config.ui_server_address, item->address, sizeof(config.ui_server_address));
			}
		}
	}
	
	selected_index = new_selected;

	
	// render quick search
	RECT button;
	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	ui2_do_label(&button, "Quick search: ", 18, -1);
	ui2_vsplit_l(&button, 95.0f, 0, &button);
	ui2_do_edit_box(&config.b_filter_string, &button, config.b_filter_string, sizeof(config.b_filter_string));

	// render filters
	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui2_do_button(&config.b_filter_empty, "Has people playing", config.b_filter_empty, &button, ui2_draw_checkbox, 0))
		config.b_filter_empty ^= 1;

	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui2_do_button(&config.b_filter_full, "Server not full", config.b_filter_full, &button, ui2_draw_checkbox, 0))
		config.b_filter_full ^= 1;

	ui2_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui2_do_button(&config.b_filter_pw, "Is not password protected", config.b_filter_pw, &button, ui2_draw_checkbox, 0))
		config.b_filter_pw ^= 1;


	// render status
	ui2_draw_rect(&status, vec4(1,1,1,0.25f), CORNER_B, 5.0f);
	ui2_vmargin(&status, 50.0f, &status);
	char buf[128];
	sprintf(buf, "%d of %d servers", client_serverbrowse_sorted_num(), client_serverbrowse_num());
	ui2_do_label(&status, buf, 18.0f, -1);

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
			client_serverbrowse_refresh(0);
		
		ui2_hsplit_t(&toolbox, 20.0f, &button, &toolbox);
		ui2_do_label(&button, "Host address:", 18, -1);
		ui2_vsplit_l(&button, 100.0f, 0, &button);
		ui2_do_edit_box(&config.ui_server_address, &button, config.ui_server_address, sizeof(config.ui_server_address));
	}
}

static void menu2_render_settings_player(RECT main_view)
{
	RECT button;
	
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	ui2_do_label(&button, "Name:", 18.0, -1);
	ui2_vsplit_l(&button, 80.0f, 0, &button);
	ui2_vsplit_l(&button, 180.0f, &button, 0);
	ui2_do_edit_box(config.player_name, &button, config.player_name, sizeof(config.player_name));

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.dynamic_camera, "Dynamic camera", config.dynamic_camera, &button, ui2_draw_checkbox, 0))
		config.dynamic_camera ^= 1;

}

typedef void (*assign_func_callback)(CONFIGURATION *config, int value);

static void menu2_render_settings_controls(RECT main_view)
{
	ui2_vsplit_l(&main_view, 300.0f, &main_view, 0);
	
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
		{ "Screenshot:", &config.key_screenshot },
	};

	const int key_count = sizeof(keys) / sizeof(KEYINFO);
	
	for (int i = 0; i < key_count; i++)
    {
		KEYINFO key = keys[i];
    	RECT button, label;
    	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
    	ui2_vsplit_l(&button, 110.0f, &label, &button);
    	
		ui2_do_label(&label, key.name, 18.0f, -1);
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
	ui2_do_label(&header, "Display Modes", 18.0f, 0);

	// draw footers	
	ui2_hsplit_b(&modelist, 20, &modelist, &footer);
	sprintf(buf, "Current: %dx%d %d bit", config.gfx_screen_width, config.gfx_screen_height, config.gfx_color_depth);
	ui2_draw_rect(&footer, vec4(1,1,1,0.25f), CORNER_B, 5.0f); 
	ui2_vsplit_l(&footer, 10.0f, 0, &footer);
	ui2_do_label(&footer, buf, 18.0f, -1);

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
		}
		
		ui2_hsplit_t(&list, 20, &button, &list);
	}
	
	
	// switches
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_fullscreen, "Fullscreen", config.gfx_fullscreen, &button, ui2_draw_checkbox, 0))
		config.gfx_fullscreen ^= 1;

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_vsync, "V-Sync", config.gfx_vsync, &button, ui2_draw_checkbox, 0))
		config.gfx_vsync ^= 1;

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_fsaa_samples, "FSAA samples", config.gfx_fsaa_samples, &button, ui2_draw_checkbox_number, 0))
	{
		dbg_msg("d", "clicked!");
		if(config.gfx_fsaa_samples < 2) config.gfx_fsaa_samples = 2;
		else if(config.gfx_fsaa_samples < 4) config.gfx_fsaa_samples = 4;
		else if(config.gfx_fsaa_samples < 6) config.gfx_fsaa_samples = 6;
		else if(config.gfx_fsaa_samples < 8) config.gfx_fsaa_samples = 8;
		else if(config.gfx_fsaa_samples < 16) config.gfx_fsaa_samples = 16;
		else if(config.gfx_fsaa_samples >= 16) config.gfx_fsaa_samples = 0;
	}
		
	ui2_hsplit_t(&main_view, 40.0f, &button, &main_view);
	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_texture_quality, "Quality Textures", config.gfx_texture_quality, &button, ui2_draw_checkbox, 0))
		config.gfx_texture_quality ^= 1;

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_texture_compression, "Texture Compression", config.gfx_texture_compression, &button, ui2_draw_checkbox, 0))
		config.gfx_texture_compression ^= 1;

	ui2_hsplit_t(&main_view, 20.0f, &button, &main_view);
	if (ui2_do_button(&config.gfx_high_detail, "High Detail", config.gfx_high_detail, &button, ui2_draw_checkbox, 0))
		config.gfx_high_detail ^= 1;
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
		{}
	else if(settings_page == 3)
		menu2_render_settings_graphics(main_view);
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

	ui2_vsplit_l(&main_view, 120.0f, &button, &main_view);
	static int spectate_button = 0;
	if(ui2_do_button(&spectate_button, "Spectate", 0, &button, ui2_draw_menu_button, 0))
		;
		
	ui2_vsplit_l(&main_view, 10.0f, &button, &main_view);
	ui2_vsplit_l(&main_view, 120.0f, &button, &main_view);
	static int change_team_button = 0;
	if(ui2_do_button(&change_team_button, "Change Team", 0, &button, ui2_draw_menu_button, 0))
		;

}

int menu2_render()
{
	gfx_mapscreen(0,0,800,600);
	
	static bool first = true;
	if(first)
	{
		client_serverbrowse_refresh(0);
		first = false;
	}

	if (inp_key_down('I') && ui2_scale() > 0.2f)
		ui2_set_scale(ui2_scale()-0.1f);
	if (inp_key_down('O'))
		ui2_set_scale(ui2_scale()+0.1f);
	
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
	
	RECT screen = *ui2_screen();
	RECT tab_bar;
	RECT main_view;

	// some margin around the screen
	ui2_margin(&screen, 10.0f, &screen);
	
	// do tab bar
	ui2_hsplit_t(&screen, 26.0f, &tab_bar, &main_view);
	ui2_vmargin(&tab_bar, 20.0f, &tab_bar);
	menu2_render_menubar(tab_bar);
		
	// render current page
	if(config.ui_page == PAGE_GAME)
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
	
	return 0;
}
