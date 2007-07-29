#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <baselib/system.h>
#include <baselib/stream/file.h>
#include <baselib/stream/line.h>


#include <baselib/system.h>
#include <baselib/input.h>
#include <baselib/network.h>
#include <baselib/math.h>

#include <engine/interface.h>
#include <engine/versions.h>
#include "../mapres.h"

#include <engine/client/ui.h>
#include "mapres_image.h"
#include "mapres_tilemap.h"
#include <engine/config.h>

#include "data.h"
#include <mastersrv/mastersrv.h>

extern data_container *data;

using namespace baselib;

/********************************************************
 MENU                                                  
*********************************************************/

enum gui_tileset_enum
{
	tileset_regular,
	tileset_hot,
	tileset_active,
	tileset_inactive
};

void draw_area(gui_tileset_enum tileset, int areax, int areay, int areaw, int areah, float x, float y, float w, float h)
{
	const float tex_w = 512.0, tex_h = 512.0;

	switch (tileset)
	{
		case tileset_regular:
			break;
		case tileset_hot:
			areax += 192; areay += 192; break;
		case tileset_active:
			areay += 192; break;
		case tileset_inactive:
			areax += 192; break;
		default:
			dbg_msg("menu", "invalid tileset given to draw_part");
	}

	float ts_x = areax / tex_w;
	float ts_y = areay / tex_h;
	float te_x = (areax + areaw) / tex_w;
	float te_y = (areay + areah) / tex_h;

    gfx_blend_normal();
    gfx_texture_set(data->images[IMAGE_GUI_WIDGETS].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
	gfx_quads_setsubset(
		ts_x, // startx
		ts_y, // starty
		te_x, // endx
		te_y); // endy								
    gfx_quads_drawTL(x,y,w,h);
    gfx_quads_end();
}

void draw_part(int part_type, gui_tileset_enum tileset, float x, float y, float w, float h)
{
	gui_box part = data->gui.misc[part_type];

	draw_area(tileset, part.x, part.y, part.w, part.h, x, y, w, h);

	//draw_part(parts[part], tileset, x, y, w, h);
}

void draw_part(int part_type, gui_tileset_enum tileset, float x, float y)
{
	gui_box part = data->gui.misc[part_type];
	
	draw_part(part_type, tileset, x, y, part.w, part.h);
}

void draw_box(int box_type, gui_tileset_enum tileset, float x, float y, float w, float h)
{
	gui_compositebox box = data->gui.boxes[box_type];

	/* A composite box consists of 9 parts. To get the coordinates for all corners, we need A, B, C and D:
	 * A----+----+----+ 
	 * | tl | tm | tr | 
	 * +----B----+----+ 
	 * | ml | mm | mr | 
	 * +----+----C----+ 
	 * | bl | bm | br | 
	 * +----+----+----D
	 */
	
	int ax = box.rect.x;
	int ay = box.rect.y;

	int bx = box.center.x;
	int by = box.center.y;
	
	int cx = box.center.x + box.center.w;
	int cy = box.center.y + box.center.h;

	int dx = box.rect.x + box.rect.w;
	int dy = box.rect.y + box.rect.h;

	draw_area(tileset, ax, ay, bx-ax, by-ay, x, y, bx-ax, by-ay);
	draw_area(tileset, bx, ay, cx-bx, by-ay, x+bx-ax, y, w-(bx-ax)-(dx-cx), by-ay);
	draw_area(tileset, cx, ay, dx-cx, by-ay, x+w-(dx-cx), y, dx-cx, by-ay);
	
	draw_area(tileset, ax, by, bx-ax, min(int(h-(by-ay)-(dy-cy)), cy-by), x, y+(by-ay), bx-ax, h-(by-ay)-(dy-cy));
	draw_area(tileset, bx, by, cx-bx, cy-by, x+bx-ax, y+(by-ay), w-(bx-ax)-(dx-cx), h-(by-ay)-(dy-cy));
	draw_area(tileset, cx, by, dx-cx, min(int(h-(by-ay)-(dy-cy)), cy-by), x+w-(dx-cx), y+(by-ay), dx-cx, h-(by-ay)-(dy-cy));

	draw_area(tileset, ax, cy, bx-ax, dy-cy, x, y+h-(dy-cy), bx-ax, dy-cy);
	draw_area(tileset, bx, cy, cx-bx, dy-cy, x+bx-ax, y+h-(dy-cy), w-(bx-ax)-(dx-cx), dy-cy);
	draw_area(tileset, cx, cy, dx-cx, dy-cy, x+w-(dx-cx), y+h-(dy-cy), dx-cx, dy-cy);
}

struct pretty_font
{
    float m_CharStartTable[256];
    float m_CharEndTable[256];
    int font_texture;
};  

extern pretty_font *current_font;

void render_sun(float x, float y);

void draw_background(float t)
{
	// background color
	gfx_clear(0.65f,0.78f,0.9f);

	gfx_blend_normal();

	render_sun(170, 170);

    gfx_texture_set(data->images[IMAGE_CLOUD_1].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
	gfx_quads_setsubset(
		0.0f, // startx
		0.0f, // starty
		1.0f, // endx
		1.0f); // endy								
    gfx_quads_drawTL(3500 - fmod(t * 20 + 2000, 4524), 0, 512, 512);
    gfx_quads_end();

    gfx_texture_set(data->images[IMAGE_CLOUD_2].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
	gfx_quads_setsubset(
		0.0f, // startx
		0.0f, // starty
		1.0f, // endx
		1.0f); // endy								
    gfx_quads_drawTL(3000 - fmod(t * 50 + 2000, 4024), 150, 512, 512);
    gfx_quads_end();

    gfx_texture_set(data->images[IMAGE_CLOUD_3].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
	gfx_quads_setsubset(
		0.0f, // startx
		0.0f, // starty
		1.0f, // endx
		1.0f); // endy								
    gfx_quads_drawTL(4000 - fmod(t * 60 + 500, 4512), 300, 256, 256);
    gfx_quads_end();

    gfx_texture_set(data->images[IMAGE_MENU_BACKGROUND].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
	gfx_quads_setsubset(
		0.0f, // startx
		0.0f, // starty
		1.0f, // endx
		1.0f); // endy								
    gfx_quads_drawTL(0, -400, 1600, 1600);
    gfx_quads_end();

	int frame = int(t * 10) % 3;

	//float x_path = -t

	float x_nudge = 3*cos(t*10);
	float y_nudge = 8*sin(t*3);

	x_nudge += 150 * cos(t/3);
	y_nudge += 30 * sin(t/3);

	float angl = t/3;
	angl = fmod(angl, 2*pi);

	bool flip = angl > pi;

    gfx_texture_set(data->images[IMAGE_MENU_BUTTERFLY].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1, 1, 1, 1);
	gfx_quads_setsubset(
		flip ? (frame + 1) * 0.25f : frame * 0.25f, // startx
		0.0f, // starty
		flip ? frame * 0.25f : (frame + 1) * 0.25f, // endx
		0.5f); // endy								
    gfx_quads_drawTL(1250 + x_nudge, 480 + y_nudge, 64, 64);
    gfx_quads_end();
}

static int music_menu;
static int music_menu_id = -1;

void draw_image_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra)
{
	ui_do_image(*(int *)id, x, y, w, h);
}

void draw_single_part_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra)
{
	gui_tileset_enum tileset;

	if (ui_active_item() == id && ui_hot_item() == id)
		tileset = tileset_active;
	else if (ui_hot_item() == id)
		tileset = tileset_hot;
	else
		tileset = tileset_regular;

	
	draw_part((int)((char*)extra-(char*)0), tileset, x, y, w, h);
}

void draw_menu_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra)
{
	int box_type;
	if ((int)((char*)extra-(char*)0))
		box_type = GUI_BOX_SCREEN_INFO;
	else
		box_type = GUI_BOX_SCREEN_LIST;
	draw_box(box_type, tileset_regular, x, y, w, h);

	ui_do_label(x + 10, y, text, 28);
}

void draw_teewars_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra)
{
	const float font_size = h-6.0f;//42.0f;

	float text_width = gfx_pretty_text_width(font_size, text);
	gui_tileset_enum tileset;

	if (ui_active_item() == id)
	{
		int inside = ui_mouse_inside(x, y, w, h);
		tileset = inside ? tileset_active : tileset_hot;
	}
	else if (ui_hot_item() == id)
		tileset = tileset_hot;
	else
		tileset = tileset_regular;

	if ((int)((char*)extra-(char*)0) == 1)
		tileset = tileset_inactive;

	draw_box(GUI_BOX_BUTTON, tileset, x, y, w, h);

	ui_do_label(x + w/2 - text_width/2, y + 2, text, font_size);
}

/*
struct server_info
{
	int version;
    int players;
	int max_players;
	netaddr4 address;
	char name[129];
	char map[65];
};*/

struct server_list
{   
	int active_count, info_count;
	int scroll_index;
	int selected_index;
};

int ui_do_key_reader(void *id, float x, float y, float w, float h, int key)
{
	// process
	static bool mouse_released = true;
	int inside = ui_mouse_inside(x, y, w, h);
	int new_key = key;
	
	if(!ui_mouse_button(0))
		mouse_released = true;

	if(ui_active_item() == id)
	{
		int k = input::last_key();
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
	int box_type;
	if (ui_active_item() == id || ui_hot_item() == id)
		box_type = GUI_BOX_SCREEN_INFO;
	else
		box_type = GUI_BOX_SCREEN_TEXTBOX;
	draw_box(box_type, tileset_regular, x, y, w, h);
	
	const char *str = input::key_name(key);
	ui_do_label(x + 10, y, str, 36);
	if (ui_active_item() == id)
	{
		float w = gfx_pretty_text_width(36.0f, str);
		ui_do_label(x + 10 + w, y, "_", 36);
	}

	return new_key;
}

int ui_do_combo_box(void *id, float x, float y, float w, char *lines, int line_count, int selected_index)
{
	float line_height = 36.0f;
	float height = line_count * line_height;

	int inside = (ui_active_item() == id) ? ui_mouse_inside(x, y, w, height) : ui_mouse_inside(x, y, w, line_height);
	int hover_index = (int)((ui_mouse_y() - y) / line_height);

	if (ui_active_item() == id)
	{
		if (!ui_mouse_button(0))
		{
			ui_set_active_item(0);

			if (inside)
				selected_index = hover_index;
		}
	}
	else if(ui_hot_item() == id)
	{
		if (ui_mouse_button(0))
		{
			ui_set_active_item(id);
		}
	}

	if (inside)
	{
		ui_set_hot_item(id);
	}

	if (ui_active_item() == id)
	{
		for (int i = 0; i < line_count; i++)
		{
			int box_type;
			if (inside && hover_index == i)
				box_type = GUI_BOX_SCREEN_INFO;
			else
				box_type = GUI_BOX_SCREEN_LIST;

			draw_box(box_type, tileset_regular, x, y + i * line_height, w, line_height);
			ui_do_label(x + 10 + 10, y + i * line_height, lines + 128 * i, 36);
			if (selected_index == i)
				ui_do_label(x + 10, y + i * line_height, "-", 36);
		}
	}
	else
	{
		int box_type;
		if (ui_active_item() == id || ui_hot_item() == id)
			box_type = GUI_BOX_SCREEN_INFO;
		else
			box_type = GUI_BOX_SCREEN_TEXTBOX;
		draw_box(box_type, tileset_regular, x, y, w, line_height);
		ui_do_label(x + 10, y, lines + 128 * selected_index, 36);
	}

	return selected_index;
}

int ui_do_edit_box(void *id, float x, float y, float w, float h, char *str, int str_size)
{
    int inside = ui_mouse_inside(x, y, w, h);
	int r = 0;
	static int at_index = 0;

	if(ui_last_active_item() == id)
	{
		int c = input::last_char();
		int k = input::last_key();
		int len = strlen(str);

		if (inside && ui_mouse_button(0))
		{
			int mx_rel = (int)(ui_mouse_x() - x);

			for (int i = 1; i <= len; i++)
			{
				if (gfx_pretty_text_width(36.0f, str, i) + 10 > mx_rel)
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

		if (c >= 32 && c < 128)
		{
			if (len < str_size - 1 && at_index < str_size - 1)
			{
				memmove(str + at_index + 1, str + at_index, len - at_index + 1);
				str[at_index] = c;
				at_index++;
			}
		}

		if (k == input::backspace && at_index > 0)
		{
			memmove(str + at_index - 1, str + at_index, len - at_index + 1);
			at_index--;
		}
		else if (k == input::delet && at_index < len)
			memmove(str + at_index, str + at_index + 1, len - at_index);
		else if (k == input::enter)
			ui_clear_last_active_item();
		else if (k == input::left && at_index > 0)
			at_index--;
		else if (k == input::right && at_index < len)
			at_index++;
		else if (k == input::home)
			at_index = 0;
		else if (k == input::end)
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

	draw_box(box_type, tileset_regular, x, y, w, h);

	ui_do_label(x + 10, y, str, 36);

	if (ui_last_active_item() == id && !just_got_active)
	{
		float w = gfx_pretty_text_width(36.0f, str, at_index);
		ui_do_label(x + 10 + w, y, "_", 36);
	}

	return r;
}

int ui_do_check_box(void *id, float x, float y, float w, float h, int value)
{
    int inside = ui_mouse_inside(x, y, w, h);
	int r = value;

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
		{
			ui_set_active_item(0);
			r = r ? 0 : 1;
		}
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
			ui_set_active_item(id);
	}
	
	if(inside)
		ui_set_hot_item(id);

	// render
	gui_tileset_enum tileset;
	int part_type;
	if (ui_active_item() == id)
		tileset = tileset_active;
	else if (ui_hot_item() == id)
		tileset = tileset_hot;
	else
		tileset = tileset_regular;

	part_type = r ? GUI_MISC_RADIO_CHECKED : GUI_MISC_RADIO_UNCHECKED;
	
	draw_part(part_type, tileset, x, y, w, h);

	return r;
}

int do_scroll_bar_horiz(void *id, float x, float y, float width, int steps, int last_index)
{
	int r = last_index;

	static int up_button;
	static int down_button;

    if (ui_do_button(&up_button, "", 0, x, y + 8, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_LEFT))
	{
		if (r > 0)
			--r;
	}
    if (ui_do_button(&down_button, "", 0, x + width - 16, y + 8, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_RIGHT))
	{
		if (r < steps)
			++r;
	}
	if (steps > 0) // only if there's actually stuff to scroll through
	{
		int inside = ui_mouse_inside(x + 16, y, width - 32, 32);
        if (inside && (!ui_active_item() || ui_active_item() == id))
			ui_set_hot_item(id);

		if(ui_active_item() == id)
		{
			if (ui_mouse_button(0))
			{
				float pos = ui_mouse_x() - x - 16;
				float perc = pos / (width - 32);

				r = (int)((steps + 1) * perc);
				if (r < 0)
					r = 0;
				else if (r > steps)
					r = steps;
			}
			else
				ui_set_active_item(0);
		}
		else if (ui_hot_item() == id && ui_mouse_button(0))
			ui_set_active_item(id);
		else if (inside && (!ui_active_item() || ui_active_item() == id))
			ui_set_hot_item(id);
	}

	draw_part(GUI_MISC_SLIDER_BIG_HORIZ_BEGIN, tileset_regular, x + 16, y + 8, 16, 16);
	draw_part(GUI_MISC_SLIDER_BIG_HORIZ_MID, tileset_regular, x + 32, y + 8, width - 32 - 32, 16);
	draw_part(GUI_MISC_SLIDER_BIG_HORIZ_END, tileset_regular, x + width - 32, y + 8, 16, 16);

	draw_part(GUI_MISC_SLIDER_BIG_HANDLE_HORIZ, tileset_regular, x + 16 + r * ((width - 64) / steps), y + 8, 32, 16);

	return r;
}

int do_scroll_bar_vert(void *id, float x, float y, float height, int steps, int last_index)
{
	int r = last_index;

	static int up_button;
	static int down_button;

    if (ui_do_button(&up_button, "", 0, x + 8, y, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_UP))
	{
		if (r > 0)
			--r;
	}
    if (ui_do_button(&down_button, "", 0, x + 8, y + height - 16, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_DOWN))
	{
		if (r < steps)
			++r;
	}
	if (steps > 0) // only if there's actually stuff to scroll through
	{
		int inside = ui_mouse_inside(x, y + 16, 16, height - 32);
        if (inside && (!ui_active_item() || ui_active_item() == id))
			ui_set_hot_item(id);

		if(ui_active_item() == id)
		{
			if (ui_mouse_button(0))
			{
				float pos = ui_mouse_y() - y - 32;
				float perc = pos / (height - 32);

				r = (int)((steps + 1) * perc);
				if (r < 0)
					r = 0;
				else if (r > steps)
					r = steps;
			}
			else
				ui_set_active_item(0);
		}
		else if (ui_hot_item() == id && ui_mouse_button(0))
			ui_set_active_item(id);
		else if (inside && (!ui_active_item() || ui_active_item() == id))
			ui_set_hot_item(id);
	}

	draw_part(GUI_MISC_SLIDER_BIG_VERT_BEGIN, tileset_regular, x + 8, y + 16, 16, 16);
	draw_part(GUI_MISC_SLIDER_BIG_VERT_MID, tileset_regular, x + 8, y + 32, 16, height - 32 - 32);
	draw_part(GUI_MISC_SLIDER_BIG_VERT_END, tileset_regular, x + 8, y + height - 32, 16, 16);

	draw_part(GUI_MISC_SLIDER_BIG_HANDLE_VERT, tileset_regular, x + 8, y + 16 + r * ((height - 64) / steps), 16, 32);

	return r;
}

static int do_server_list(float x, float y, int *scroll_index, int *selected_index, int visible_items)
{
	const float spacing = 3.f;
	const float item_height = 28;
	const float item_width = 728;
	const float real_width = item_width + 20;
	const float real_height = item_height * visible_items + spacing * (visible_items - 1);

	server_info *servers;
	int num_servers = client_serverbrowse_getlist(&servers);

	int r = -1;

	for (int i = 0; i < visible_items; i++)
	{
		int item_index = i + *scroll_index;
		if (item_index >= num_servers)
			;
			//ui_do_image(empty_item_texture, x, y + i * item_height + i * spacing, item_width, item_height);
		else
		{
			server_info *item = &servers[item_index];

			bool clicked = false;
			clicked = ui_do_button(item, item->name, 0, x, y + i * item_height + i * spacing, item_width, item_height,
				draw_menu_button, (*selected_index == item_index) ? (void *)1 : 0);

			char temp[64]; // plenty of extra room so we don't get sad :o
			sprintf(temp, "%i/%i  %3d", item->num_players, item->max_players, item->latency);

			ui_do_label(x + 600, y + i * item_height + i * spacing, temp, item_height);
			ui_do_label(x + 360, y + i * item_height + i * spacing, item->map, item_height);

			if (clicked)
			{
				r = item_index;
				*selected_index = item_index;
			}
		}
	}

	*scroll_index = do_scroll_bar_vert(scroll_index, x + real_width - 16, y, real_height,
		min(num_servers - visible_items, 0), *scroll_index);
	
	return r;
}

enum
{
	SCREEN_MAIN,
	SCREEN_SETTINGS_GENERAL,
	SCREEN_SETTINGS_CONTROLS,
	SCREEN_SETTINGS_VIDEO,
	SCREEN_SETTINGS_VIDEO_SELECT_MODE,
	SCREEN_SETTINGS_SOUND,
	SCREEN_KERNING
};

static int screen = SCREEN_MAIN;
static configuration config_copy;

const float column1_x = 250;
const float column2_x = column1_x + 170;
const float column3_x = column2_x + 170;
const float row1_y = 180;
const float row2_y = row1_y + 40;
const float row3_y = row2_y + 40;
const float row4_y = row3_y + 40;
const float row5_y = row4_y + 40;
const float row6_y = row5_y + 40;
const float row7_y = row6_y + 40;

static int main_render()
{
	static bool inited = false;

	if (!inited)
	{
		inited = true;
		client_serverbrowse_refresh(0);
	}

	static int scoll_index = 0, selected_index = -1;
	int last_selected_index = selected_index;
	do_server_list(20, 160, &scoll_index, &selected_index, 8);
	
	static char address[32] = "localhost:8303";

	ui_do_edit_box(address, 280, 425, 300, 36, address, sizeof(address));

	if (last_selected_index != selected_index && selected_index != -1)
	{
		server_info *servers;
		client_serverbrowse_getlist(&servers);

		strcpy(address, servers[selected_index].address);
	}

	static int refresh_button, join_button, quit_button;
	static int use_lan = 0;

	if (ui_do_button(&refresh_button, "Refresh", 0, 20, 460, 170, 48, draw_teewars_button))
		client_serverbrowse_refresh(use_lan);
	
	ui_do_label(60, 420, "Search LAN ", 36);
	int last_lan = use_lan;
	use_lan = ui_do_check_box(&use_lan, 20, 424, 32, 32, use_lan);
	if (use_lan != last_lan)
		client_serverbrowse_refresh(use_lan);

	if (ui_do_button(&join_button, "Join", 0, 620, 420, 128, 48, draw_teewars_button))
	{
		client_connect(address);

		return 1;
	}

	if (ui_do_button(&quit_button, "Quit", 0, 620, 490, 128, 48, draw_teewars_button))
		return -1;

	static int settings_button;
	if (ui_do_button(&settings_button, "Settings", 0, 400, 490, 170, 48, draw_teewars_button))
	{
		config_copy = config;
		screen = SCREEN_SETTINGS_GENERAL;
	}

	/*static int editor_button;
	if (ui_do_button(&editor_button, "Kerning Editor", 0, 20, 470, 170, 48, draw_teewars_button))
		screen = SCREEN_KERNING;*/

	return 0;
}


static int settings_general_render()
{
	// NAME
	ui_do_label(column1_x, row1_y, "Name:", 36);
	ui_do_edit_box(config_copy.player_name, column2_x, row1_y, 300, 36, config_copy.player_name, sizeof(config_copy.player_name));

	return 0;
}

typedef void (*assign_func_callback)(configuration *config, int value);

struct key_thing
{
	char name[32];
	int *key;
	assign_func_callback assign_func;
};

static int settings_controls_render()
{
	static int scroll_index = 0;

	key_thing keys[] = 
	{
		{ "Move Left:", &config_copy.key_move_left, config_set_key_move_left },
		{ "Move Right:", &config_copy.key_move_right, config_set_key_move_right },
		{ "Jump:", &config_copy.key_jump, config_set_key_jump },
		{ "Fire:", &config_copy.key_fire, config_set_key_fire },
		{ "Hook:", &config_copy.key_hook, config_set_key_hook },
		{ "Pistol:", &config_copy.key_weapon1, config_set_key_weapon1 },
		{ "Grenade:", &config_copy.key_weapon2, config_set_key_weapon2 },
		{ "Shotgun:", &config_copy.key_weapon3, config_set_key_weapon3 },
		{ "Hammer:", &config_copy.key_weapon4, config_set_key_weapon4 },
	};

	for (int i = 0; i < 7; i++)
	{
		key_thing key = keys[i + scroll_index];
		
		ui_do_label(column1_x, row1_y + 40 * i, key.name, 36);
		key.assign_func(&config_copy, ui_do_key_reader(key.key, column2_x, row1_y + 40 * i, 150, 36, *key.key));
	}
	// KEYS
	/*
	ui_do_label(column1_x, row1_y + 0, "Move Left:", 36);
	config_set_key_move_left(&config_copy, ui_do_key_reader(&config_copy.key_move_left, column2_x, row1_y + 0, 150, 36, config_copy.key_move_left));
	ui_do_label(column1_x, row1_y + 40, "Move Right:", 36);
	config_set_key_move_right(&config_copy, ui_do_key_reader(&config_copy.key_move_right, column2_x, row1_y + 40, 150, 36, config_copy.key_move_right));
	ui_do_label(column1_x, row1_y + 80, "Jump:", 36);
	config_set_key_jump(&config_copy, ui_do_key_reader(&config_copy.key_jump, column2_x, row1_y + 80, 150, 36, config_copy.key_jump));
	ui_do_label(column1_x, row1_y + 120, "Fire:", 36);
	config_set_key_fire(&config_copy, ui_do_key_reader(&config_copy.key_fire, column2_x, row1_y + 120, 150, 36, config_copy.key_fire));
	ui_do_label(column1_x, row1_y + 160, "Hook:", 36);
	config_set_key_hook(&config_copy, ui_do_key_reader(&config_copy.key_hook, column2_x, row1_y + 160, 150, 36, config_copy.key_hook));
	*/

	scroll_index = do_scroll_bar_vert(&scroll_index, 600, row1_y, 40 * 7, 9 - 7, scroll_index);

	return 0;
}

static const int MAX_RESOLUTIONS = 128;
static int settings_video_render_select_mode()
{
	video_mode modes[MAX_RESOLUTIONS];
	int num_modes = gfx_get_video_modes(modes, MAX_RESOLUTIONS);
	
	static int scroll_index = 0;
	scroll_index = do_scroll_bar_vert(&scroll_index, 600, row1_y, 40 * 7, num_modes - 7, scroll_index);

	for (int i = 0; i < 7; i++)
	{
		int index = i + scroll_index;
		if(index >= num_modes)
			break;
			
		//key_thing key = keys[i + scroll_index];
		int depth = modes[index].red+modes[index].green+modes[index].blue;
		if(depth < 16)
			depth = 16;
		else if(depth > 16)
			depth = 24;
		
		char buf[128];
		int s = 0;
		if(modes[index].width == config_copy.gfx_screen_width &&
			modes[index].height == config_copy.gfx_screen_height && 
			depth == config_copy.gfx_color_depth)
		{
			s = 1;
		}
		
		sprintf(buf, "%c %dx%d %d bit %c", s?'>':' ', modes[index].width, modes[index].height, depth, s?'<':' ');
		
		if(ui_do_button((void*)&modes[index], buf, 0,
			column1_x, row1_y + 40 * i, 320, 32.0f, draw_teewars_button))
		{
			// select
			config_set_gfx_color_depth(&config_copy, depth);
			config_set_gfx_screen_width(&config_copy, modes[index].width);
			config_set_gfx_screen_height(&config_copy, modes[index].height);
			screen = SCREEN_SETTINGS_VIDEO;
		}
	}

	return 0;
}

static int settings_video_render()
{

	ui_do_label(column1_x, row1_y, "Mode:", 36);

	char buf[128];
	sprintf(buf, "%dx%d %d bit", config_copy.gfx_screen_width, config_copy.gfx_screen_height, config_copy.gfx_color_depth);
	static int select_button = 0;
	if(ui_do_button(&select_button, buf, 0, column2_x, row1_y, 300, 32, draw_teewars_button))
		screen = SCREEN_SETTINGS_VIDEO_SELECT_MODE;
		
	// we need to draw these bottom up, to make overlapping work correctly
	
	ui_do_label(column1_x, row2_y, "Fullscreen:", 36);
	config_set_gfx_fullscreen(&config_copy, ui_do_check_box(&config_copy.gfx_fullscreen, column3_x, row2_y + 5, 32, 32, config_copy.gfx_fullscreen));

	ui_do_label(column1_x, row3_y, "V-sync:", 36);
	config_set_gfx_vsync(&config_copy, ui_do_check_box(&config_copy.gfx_vsync, column3_x, row3_y + 5, 32, 32, config_copy.gfx_vsync));

	ui_do_label(column1_x, row4_y, "Quality Textures:", 36);
	config_set_gfx_texture_quality(&config_copy, ui_do_check_box(&config_copy.gfx_texture_quality, column3_x, row4_y + 5, 32, 32, config_copy.gfx_texture_quality));

	ui_do_label(column1_x, row5_y, "Textures Compression:", 36);
	config_set_gfx_texture_compression(&config_copy, ui_do_check_box(&config_copy.gfx_texture_compression, column3_x, row5_y + 5, 32, 32, config_copy.gfx_texture_compression));

	ui_do_label(column1_x, row6_y, "High Detail:", 36);
	config_set_gfx_high_detail(&config_copy, ui_do_check_box(&config_copy.gfx_high_detail, column3_x, row6_y + 5, 32, 32, config_copy.gfx_high_detail));

	ui_do_label(column1_x, row6_y + 50, "(A restart of the game is required for these settings to take effect.)", 20);

	return 0;
}

static int settings_sound_render()
{
	ui_do_label(column1_x, row1_y, "Volume:", 36);
	
	config_set_volume(&config_copy, do_scroll_bar_horiz(&config_copy.volume, column2_x, row1_y, 200, 255, config_copy.volume));
	snd_set_master_volume(config_copy.volume / 255.0f);

	return 0;
}

static int settings_render()
{
	static int general_button, controls_button, video_button, sound_button;

	if (ui_do_button(&general_button, "General", 0, 30, 200, 170, 48, draw_teewars_button))
		screen = SCREEN_SETTINGS_GENERAL;
	if (ui_do_button(&controls_button, "Controls", 0, 30, 250, 170, 48, draw_teewars_button))
		screen = SCREEN_SETTINGS_CONTROLS;
	if (ui_do_button(&video_button, "Video", 0, 30, 300, 170, 48, draw_teewars_button))
		screen = SCREEN_SETTINGS_VIDEO;
	if (ui_do_button(&sound_button, "Sound", 0, 30, 350, 170, 48, draw_teewars_button))
		screen = SCREEN_SETTINGS_SOUND;

	switch (screen)
	{
		case SCREEN_SETTINGS_GENERAL: settings_general_render(); break;
		case SCREEN_SETTINGS_CONTROLS: settings_controls_render(); break;
		case SCREEN_SETTINGS_VIDEO: settings_video_render(); break;
		case SCREEN_SETTINGS_VIDEO_SELECT_MODE: settings_video_render_select_mode(); break;
		case SCREEN_SETTINGS_SOUND: settings_sound_render(); break;
	}

	// SAVE BUTTON
	static int save_button;
	if (ui_do_button(&save_button, "Save", 0, 482, 490, 128, 48, draw_teewars_button))
	{
		config = config_copy;
		config_save("default.cfg");
		screen = SCREEN_MAIN;
	}
	
	// CANCEL BUTTON
	static int cancel_button;
	if (ui_do_button(&cancel_button, "Cancel", 0, 620, 490, 150, 48, draw_teewars_button))
	{
		snd_set_master_volume(config.volume / 255.0f);
		screen = SCREEN_MAIN;
	}

	return 0;
}

extern double extra_kerning[256*256];

static int kerning_render()
{
	static bool loaded = false;
	static char text[32] = {0};

	if (!loaded)
	{
		file_stream file;

		if (file.open_r("kerning.txt"))
		{
			line_stream lstream(&file);
			int i = 0;
			char *line;

			while ((line = lstream.get_line()))
				extra_kerning[i++] = atof(line);

			file.close();
		}

		if (file.open_r("tracking.txt"))
		{
			line_stream lstream(&file);
			char *line;

			for (int i = 0; i < 256; i++)
			{
				line = lstream.get_line();
				current_font->m_CharStartTable[i] = atof(line);
				line = lstream.get_line();
				current_font->m_CharEndTable[i] = atof(line);
			}

			file.close();
		}

		loaded = true;
	}

	ui_do_edit_box(text, 160, 20, 300, 36, text, sizeof(text));

	ui_do_label(160, 250, text, 70);

	int len = strlen(text);

	for (int i = 0; i < len-1; i++)
	{
		char s[3] = {0};
		s[0] = text[i];
		s[1] = text[i+1];
		ui_do_label(10, 30 * i + 10, s, 45); 

		int index = s[0] + s[1] * 256;

		// less
		if (ui_do_button((void *)(100 + i * 2), "", 0, 50, 30 * i + 10 + 20, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_LEFT))
		{
			extra_kerning[index] -= 0.01;
		}

		// more
		if (ui_do_button((void *)(100 + i * 2 + 1), "", 0, 66, 30 * i + 10 + 20, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_RIGHT))
		{
			extra_kerning[index] += 0.01;
		}

		char num[16];
		sprintf(num, "(%f)", extra_kerning[index]);
		ui_do_label(84, 30 * i + 30, num, 12);
	}

	for (int i = 0; i < len; i++)
	{
		char s[2] = {0};
		s[0] = text[i];

		ui_do_label(700, 35 * i + 10, s, 45);

    	gfx_blend_normal();
    	gfx_texture_set(-1);
    	gfx_quads_begin();
    	gfx_quads_setcolor(0,0,0,0.5);
    	gfx_quads_drawTL(700,35*i+20,1,30);
		gfx_quads_drawTL(700+45*(current_font->m_CharEndTable[(int)s[0]]-current_font->m_CharStartTable[(int)s[0]]),35*i+20,1,30);
    	gfx_quads_end();
		// less
		if (ui_do_button((void *)(200 + i * 2), "", 0, 650, 35 * i + 10 + 15, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_LEFT))
		{
			current_font->m_CharStartTable[(int)s[0]] -= 0.01f;
		}

		// more
		if (ui_do_button((void *)(200 + i * 2 + 1), "", 0, 666, 35 * i + 10 + 15, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_RIGHT))
		{
			current_font->m_CharStartTable[(int)s[0]] += 0.01f;
		}

		char num[16];
		sprintf(num, "(%f)", current_font->m_CharStartTable[(int)s[0]]);
		ui_do_label(645, 35 * i + 40, num, 12);




		// less
		if (ui_do_button((void *)(300 + i * 2), "", 0, 750, 35 * i + 10 + 15, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_LEFT))
		{
			current_font->m_CharEndTable[(int)s[0]] -= 0.01f;
		}

		// more
		if (ui_do_button((void *)(300 + i * 2 + 1), "", 0, 766, 35 * i + 10 + 15, 16, 16, draw_single_part_button, (void *)GUI_MISC_SLIDER_BIG_ARROW_RIGHT))
		{
			current_font->m_CharEndTable[(int)s[0]] += 0.01f;
		}

		sprintf(num, "(%f)", current_font->m_CharEndTable[(int)s[0]]);
		ui_do_label(745, 35 * i + 40, num, 12);

	}

	// SAVE BUTTON
	static int save_button;
	if (ui_do_button(&save_button, "Save", 0, 482, 520, 128, 48, draw_teewars_button))
	{
		file_stream file;

		if (file.open_w("kerning.txt"))
		{
			char t[16];
			
			for (int i = 0; i < 256*256; i++)
			{
				sprintf(t, "%f\n", extra_kerning[i]);
				file.write(t, strlen(t));
			}

			file.close();
		}

		if (file.open_w("tracking.txt"))
		{
			char t[16];

			for (int i = 0; i < 256; i++)
			{
				sprintf(t, "%f\n", current_font->m_CharStartTable[i]);
				file.write(t, strlen(t));
				sprintf(t, "%f\n", current_font->m_CharEndTable[i]);
				file.write(t, strlen(t));
			}

			file.close();
		}

		//screen = 0;
	}
	
	// CANCEL BUTTON
	static int cancel_button;
	if (ui_do_button(&cancel_button, "Cancel", 0, 620, 520, 150, 48, draw_teewars_button))
		screen = SCREEN_MAIN;

	return 0;
}

static int menu_render()
{
	// background color
	gfx_clear(0.65f,0.78f,0.9f);
	//gfx_clear(89/255.f,122/255.f,0.0);

	// GUI coordsys
	gfx_mapscreen(0,0,800.0f,600.0f);

	static int64 start = time_get();

	float t = double(time_get() - start) / double(time_freq());
	gfx_mapscreen(0,0,1600.0f,1200.0f);
	draw_background(t);
	gfx_mapscreen(0,0,800.0f,600.0f);

	if (screen != SCREEN_KERNING)
	{
		ui_do_image(data->images[IMAGE_BANNER].id, 200, 20, 512, 128);
		ui_do_label(20.0f, 600.0f-40.0f, "Version: " TEEWARS_VERSION, 36);
	}

	switch (screen)
	{
		case SCREEN_MAIN: return main_render();
		case SCREEN_SETTINGS_GENERAL:
		case SCREEN_SETTINGS_CONTROLS:
		case SCREEN_SETTINGS_VIDEO:
		case SCREEN_SETTINGS_VIDEO_SELECT_MODE:
		case SCREEN_SETTINGS_SOUND: return settings_render();
		case SCREEN_KERNING: return kerning_render();
		default: dbg_msg("menu", "invalid screen selected..."); return 0;
	}
}

void modmenu_init()
{
	input::enable_char_cache();
	input::enable_key_cache();

	// TODO: should be removed
    current_font->font_texture = gfx_load_texture("data/big_font.png");

	// TODO: should be removed
	music_menu = snd_load_wav("data/audio/Music_Menu.wav");
}

void modmenu_shutdown()
{
}

int modmenu_render()
{
	static int mouse_x = 0;
	static int mouse_y = 0;

	if (music_menu_id == -1)
	{
		dbg_msg("menu", "no music is playing, so let's play some tunes!");
		music_menu_id = snd_play(music_menu, SND_LOOP);
	}

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
        mx = (mouse_x/(float)gfx_screenwidth())*800.0f;
        my = (mouse_y/(float)gfx_screenheight())*600.0f;
            
        int buttons = 0;
        if(inp_key_pressed(input::mouse_1)) buttons |= 1;
        if(inp_key_pressed(input::mouse_2)) buttons |= 2;
        if(inp_key_pressed(input::mouse_3)) buttons |= 4;
            
        ui_update(mx,my,mx*3.0f,my*3.0f,buttons);
    }

    //int r = menu_render(server_address, str, max_len);
	int r = menu_render();

    // render butt ugly mouse cursor
    // TODO: render nice cursor
    gfx_texture_set(data->images[IMAGE_CURSOR].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
    gfx_quads_drawTL(mx,my,24,24);
    gfx_quads_end();

	if (r)
	{
		snd_stop(music_menu_id);
		music_menu_id = -1;
	}

	input::clear_char();
	input::clear_key();

	return r;
}
