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

#include <engine/interface.h>
#include <engine/versions.h>
#include "../mapres.h"

#include <engine/client/ui.h>
#include "mapres_image.h"
#include "mapres_tilemap.h"
#include <engine/config.h>

#include "data.h"

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

int gui_tileset_texture;

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
    gfx_texture_set(gui_tileset_texture);
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
	 * |    |    |    | 
	 * | tl | tm | tr | 
	 * |    |    |    | 
	 * +----B----+----+ 
	 * |    |    |    | 
	 * | ml | mm | mr | 
	 * |    |    |    | 
	 * +----+----C----+ 
	 * |    |    |    | 
	 * | bl | bm | br | 
	 * |    |    |    | 
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
	
	draw_area(tileset, ax, by, bx-ax, cy-by, x, y+(by-ay), bx-ax, h-(by-ay)-(dy-cy));
	draw_area(tileset, bx, by, cx-bx, cy-by, x+bx-ax, y+(by-ay), w-(bx-ax)-(dx-cx), h-(by-ay)-(dy-cy));
	draw_area(tileset, cx, by, dx-cx, cy-by, x+w-(dx-cx), y+(by-ay), dx-cx, h-(by-ay)-(dy-cy));

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
float gfx_pretty_text_width(float size, const char *text);

void draw_scrolling_background(int id, float w, float h, float t)
{
	float tx = w/512.0f;
	float ty = h/512.0f;

	float start_x = fmod(t, 1.0f);
	float start_y = 1.0f - fmod(t*0.8f, 1.0f);

    gfx_blend_normal();
    gfx_texture_set(id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
	gfx_quads_setsubset(
		start_x, // startx
		start_y, // starty
		start_x+tx, // endx
		start_y+ty); // endy								
    gfx_quads_drawTL(0.0f,0.0f,w,h);
    gfx_quads_end();
}

static int background_texture;
static int teewars_banner_texture;

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
	float text_width = gfx_pretty_text_width(46.f, text);
	gui_tileset_enum tileset;

	if (ui_active_item() == id && ui_hot_item() == id)
		tileset = tileset_active;
	else if (ui_hot_item() == id)
		tileset = tileset_hot;
	else
		tileset = tileset_regular;

	if ((int)((char*)extra-(char*)0) == 1)
		tileset = tileset_inactive;

	draw_box(GUI_BOX_BUTTON, tileset, x, y, w, h);

	ui_do_label(x + w/2 - text_width/2, y, text, 46);
}

struct server_info
{
	int version;
    int players;
	int max_players;
	netaddr4 address;
	char name[129];
	char map[65];
};

struct server_list
{   
    server_info infos[10];
	int active_count, info_count;
	int scroll_index;
	int selected_index;
};


int ui_do_key_reader(void *id, float x, float y, float w, float h, int key)
{
	// process
	static bool can_be_selected = true;
	int inside = ui_mouse_inside(x, y, w, h);
	int new_key = key;
	
	if (!ui_mouse_button(0) && !can_be_selected)
		can_be_selected = true;

	if (can_be_selected)
	{
		if (inside)
		{
			ui_set_hot_item(id);

			if (ui_mouse_button(0) && ui_active_item() != id)
			{
				ui_set_active_item(id);
				can_be_selected = false;
			}
		}
	}

	if (can_be_selected)
	{
		if (ui_active_item() == id)
		{
			int k = input::last_key();
			if (k)
			{
				new_key = k;
				ui_set_active_item(0);
				can_be_selected = false;
			}
		}
	}

	// draw
	draw_box(GUI_BOX_SCREEN_INFO, tileset_regular, x, y, w, h);
	
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

	int inside = (ui_active_item() == id) ? ui_mouse_inside(x, y, w, line_count * line_height) : ui_mouse_inside(x, y, w, line_height);
	int hover_index = (int)((ui_mouse_y() - y) / line_height);

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

		if (!ui_mouse_button(0))
		{
			ui_set_active_item(0);

			if (inside)
				selected_index = hover_index;
		}
	}
	else
	{
		draw_box(GUI_BOX_SCREEN_LIST, tileset_regular, x, y, w, line_height);
		ui_do_label(x + 10, y, lines + 128 * selected_index, 36);

		if (inside && ui_mouse_button(0))
			ui_set_active_item(id);
	}

	return selected_index;
}

int ui_do_edit_box(void *id, float x, float y, float w, float h, char *str, int str_size)
{
    int inside = ui_mouse_inside(x, y, w, h);
	int r = 0;

	if(inside)
	{
		ui_set_hot_item(id);

		if(ui_mouse_button(0))
			ui_set_active_item(id);
	}

	if (ui_active_item() == id)
	{
		int c = input::last_char();
		int k = input::last_key();
		int len = strlen(str);

		if (c >= 32 && c < 128)
		{
			if (len < str_size - 1)
			{
				str[len] = c;
				str[len+1] = 0;
			}
		}

		if (k == input::backspace)
		{
			if (len > 0)
				str[len-1] = 0;
		}
		r = 1;
	}

	draw_box(GUI_BOX_SCREEN_TEXTBOX, tileset_regular, x, y, w, h);

	ui_do_label(x + 10, y, str, 36);

	if (ui_active_item() == id)
	{
		float w = gfx_pretty_text_width(36.0f, str);
		ui_do_label(x + 10 + w, y, "_", 36);
	}

	return r;
}

int do_scroll_bar(void *id, float x, float y, float height, int steps, int last_index)
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
				float pos = ui_mouse_y() - y - 16;
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

	draw_part(GUI_MISC_SLIDER_BIG_HANDLE_HORIZ, tileset_regular, x, y + 16 + r * ((height - 64) / steps), 32, 16);

	return r;
}

static int do_server_list(server_list *list, float x, float y, int visible_items)
{
	const float spacing = 3.f;
	const float item_height = 28;
	const float item_width = 728;
	const float real_width = item_width + 20;
	const float real_height = item_height * visible_items + spacing * (visible_items - 1);

	int r = -1;

	for (int i = 0; i < visible_items; i++)
	{
		int item_index = i + list->scroll_index;
		if (item_index >= list->active_count);
			//ui_do_image(empty_item_texture, x, y + i * item_height + i * spacing, item_width, item_height);
		else
		{
			server_info *item = &list->infos[item_index];

			bool clicked = false;
			clicked = ui_do_button(item, item->name, 0, x, y + i * item_height + i * spacing, item_width, item_height, draw_menu_button, (list->selected_index == item_index) ? (void *)1 : 0);

			char temp[64]; // plenty of extra room so we don't get sad :o
			sprintf(temp, "%i/%i", item->players, item->max_players);

			ui_do_label(x + 600, y + i * item_height + i * spacing, temp, item_height);
			ui_do_label(x + 360, y + i * item_height + i * spacing, item->map, item_height);

			if (clicked)
			{
				r = item_index;
				list->selected_index = item_index;
			}
		}
	}

	list->scroll_index = do_scroll_bar(&list->scroll_index, x + real_width - 16, y, real_height, list->active_count - visible_items, list->scroll_index);
	
	return r;
}

static char *read_int(char *buffer, int *value)
{
    *value = buffer[0] << 24;
    *value |= buffer[1] << 16;
    *value |= buffer[2] << 8;
    *value |= buffer[3];

	return buffer + 4;
}

static char *read_netaddr(char *buffer, netaddr4 *addr)
{
	addr->ip[0] = *buffer++;
	addr->ip[1] = *buffer++;
	addr->ip[2] = *buffer++;
	addr->ip[3] = *buffer++;

	int port;
	buffer = read_int(buffer, &port);

	addr->port = port;

	return buffer;
}

static void refresh_list(server_list *list)
{
	netaddr4 addr;
	netaddr4 me(0, 0, 0, 0, 0);

	list->selected_index = -1;
	
	if (net_host_lookup(MASTER_SERVER_ADDRESS, MASTER_SERVER_PORT, &addr) == 0)
    {
        socket_tcp4 sock;
        sock.open(&me);

		//sock.set_non_blocking();

		// try and connect with a timeout of 1 second
        if (sock.connect_non_blocking(&addr))
        {
            char data[256];
            int total_received = 0;
            int received;

            int master_server_version = -1;
            int server_count = -1;

            // read header
            while (total_received < 12 && (received = sock.recv(data + total_received, 12 - total_received)) > 0)
                total_received += received;

            // see if we have the header
            if (total_received == 12)
            {
                int signature;
                read_int(data, &signature);
    
                // check signature
                if(signature == 'TWSL')
                {
                    read_int(data + 4, &master_server_version);
                    read_int(data + 8, &server_count);

                    // TODO: handle master server version O.o
                        
                    const int server_info_size = 212;

                    list->active_count = 0;
    
                    for (int i = 0; i < server_count; i++)
                    { 
                         total_received = 0;

                        // read data for a server
                        while (sock.is_connected() && total_received < server_info_size && (received = sock.recv(data + total_received, server_info_size - total_received)) > 0)
                            total_received += received;

                        // check if we got enough data
                        if (total_received == server_info_size)
                        {
                            char *d = data;

                            server_info *info = &list->infos[i];

                            d = read_int(d, &info->version);
                            d = read_netaddr(d, &info->address);

							//dbg_msg("menu/got_serverinfo", "IP: %i.%i.%i.%i:%i", (int)info->address.ip[0], (int)info->address.ip[1], (int)info->address.ip[2], (int)info->address.ip[3], info->address.port);

                            d = read_int(d, &info->players);
                            d = read_int(d, &info->max_players);
                            memcpy(info->name, d, 128);
                            d += 128;
							memcpy(info->map, d, 64);

                            // let's be safe.
                            info->name[128] = 0;
                            info->map[64] = 0;

                            ++list->active_count;
                        }
                        else
                            break;
                    }

                    if (list->scroll_index >= list->active_count)
                        list->scroll_index = list->active_count - 1;

                    if (list->scroll_index < 0)
                        list->scroll_index = 0;
                }
            }

            sock.close();
        }
	}
}

static int screen = 0;
static configuration config_copy;

static int main_screen_render(netaddr4 *server_address)
{
	static server_list list;
	static bool inited = false;

	if (!inited)
	{
		list.info_count = 256;

		list.scroll_index = 0;
		list.selected_index = -1;

		inited = true;

		refresh_list(&list);
	}

	do_server_list(&list, 20, 160, 8);

	static int refresh_button, join_button, quit_button;

	if (ui_do_button(&refresh_button, "Refresh", 0, 440, 420, 170, 48, draw_teewars_button))
	{
		refresh_list(&list);
	} 

	if (list.selected_index == -1)
	{
		ui_do_button(&join_button, "Join", 0, 620, 420, 128, 48, draw_teewars_button, (void *)1);
	}
	else if (ui_do_button(&join_button, "Join", 0, 620, 420, 128, 48, draw_teewars_button))
	{
		*server_address = list.infos[list.selected_index].address;

		dbg_msg("menu/join_button", "IP: %i.%i.%i.%i:%i", (int)server_address->ip[0], (int)server_address->ip[1], (int)server_address->ip[2], (int)server_address->ip[3], server_address->port);

		return 1;
	}

	if (ui_do_button(&quit_button, "Quit", 0, 620, 490, 128, 48, draw_teewars_button))
		return -1;

	static int settings_button;
	if (ui_do_button(&settings_button, "Settings", 0, 20, 420, 170, 48, draw_teewars_button))
	{
		config_copy = config;
		screen = 1;
	}

	static int editor_button;
	if (ui_do_button(&editor_button, "Kerning Editor", 0, 20, 470, 170, 48, draw_teewars_button))
		screen = 2;

	return 0;
}

static int settings_screen_render()
{
	const float column1_x = 150;
	const float column2_x = column1_x + 150;
	const float column3_x = column2_x + 170;
	const float name_y = 160;
	const float resolution_y = 200;
	const float keys_y = 240;

	// NAME
	ui_do_label(column1_x, name_y, "Name:", 36);
	ui_do_edit_box(config_copy.player_name, column2_x, name_y, 300, 36, config_copy.player_name, sizeof(config_copy.player_name));

	// KEYS
	ui_do_label(column1_x, keys_y, "Keys:", 36);
	ui_do_label(column2_x, keys_y + 0, "Move Left:", 36);
	config_set_key_move_left(&config_copy, ui_do_key_reader(&config_copy.key_move_left, column3_x, keys_y + 0, 150, 40, config_copy.key_move_left));
	ui_do_label(column2_x, keys_y + 40, "Move Right:", 36);
	config_set_key_move_right(&config_copy, ui_do_key_reader(&config_copy.key_move_right, column3_x, keys_y + 40, 150, 40, config_copy.key_move_right));
	ui_do_label(column2_x, keys_y + 80, "Jump:", 36);
	config_set_key_jump(&config_copy, ui_do_key_reader(&config_copy.key_jump, column3_x, keys_y + 80, 150, 40, config_copy.key_jump));
	ui_do_label(column2_x, keys_y + 120, "Fire:", 36);
	config_set_key_fire(&config_copy, ui_do_key_reader(&config_copy.key_fire, column3_x, keys_y + 120, 150, 40, config_copy.key_fire));
	ui_do_label(column2_x, keys_y + 160, "Hook:", 36);
	config_set_key_hook(&config_copy, ui_do_key_reader(&config_copy.key_hook, column3_x, keys_y + 160, 150, 40, config_copy.key_hook));

	// RESOLUTION
	static char resolutions[][128] =
	{
		"800x600",
		"1024x764",
		"1280x960",
		"1600x1200",
	};
	static int res[][2] =
	{
		{ 800, 600 },
		{ 1024, 768 },
		{ 1280, 960 },
		{ 1600, 1200 },
	};

	static int selected_index = -1;
	if (selected_index == -1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (config.screen_width == res[i][0])
			{
				selected_index = i;
				break;
			}
		}

		if (selected_index == -1)
			selected_index = 1;
	}

	ui_do_label(column1_x, resolution_y, "Resolution:", 36);
	selected_index = ui_do_combo_box(&selected_index, column2_x, resolution_y, 180, (char *)resolutions, 4, selected_index);

	config_set_screen_width(&config_copy, res[selected_index][0]);
	config_set_screen_height(&config_copy, res[selected_index][1]);

	// SAVE BUTTON
	static int save_button;
	if (ui_do_button(&save_button, "Save", 0, 482, 490, 128, 48, draw_teewars_button))
	{
		config = config_copy;
		config_save("teewars.cfg");
		screen = 0;
	}
	
	// CANCEL BUTTON
	static int cancel_button;
	if (ui_do_button(&cancel_button, "Cancel", 0, 620, 490, 150, 48, draw_teewars_button))
		screen = 0;

	return 0;
}

extern double extra_kerning[256*256];

static int editor_screen_render()
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
		screen = 0;

	return 0;
}

static int menu_render(netaddr4 *server_address)
{
	// background color
	gfx_clear(89/255.f,122/255.f,0.0);

	// GUI coordsys
	gfx_mapscreen(0,0,800.0f,600.0f);

	static int64 start = time_get();

	float t = double(time_get() - start) / double(time_freq());
	draw_scrolling_background(background_texture, 800, 600, t * 0.01);

	if (screen != 2)
	{
		ui_do_image(teewars_banner_texture, 140, 20, 512, 128);
		ui_do_label(20.0f, 600.0f-40.0f, "Version: " TEEWARS_VERSION, 36);
	}

	switch (screen)
	{
	case 1:
		return settings_screen_render();
	case 2:
		return editor_screen_render();
	default:
		return main_screen_render(server_address);
	}
}

void modmenu_init()
{
	input::enable_char_cache();
	input::enable_key_cache();

    current_font->font_texture = gfx_load_texture("data/big_font.png");

	background_texture = gfx_load_texture("data/gui_bg.png");
	gui_tileset_texture = gfx_load_texture("data/gui/gui_widgets.png");
    teewars_banner_texture = gfx_load_texture("data/gui_logo.png");

	music_menu = snd_load_wav("data/audio/Music_Menu.wav");
}

void modmenu_shutdown()
{
}

int modmenu_render(void *ptr)
{
	static int mouse_x = 0;
	static int mouse_y = 0;

	if (music_menu_id == -1)
	{
		dbg_msg("menu", "no music is playing, so let's play some tunes!");
		music_menu_id = snd_play(music_menu, SND_LOOP);
	}

	netaddr4 *server_address = (netaddr4 *)ptr;	

    // handle mouse movement
    float mx, my, mwx, mwy;
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
        mwx = mx*3.0f; // adjust to zoom and offset
        mwy = mx*3.0f; // adjust to zoom and offset
            
        int buttons = 0;
        if(inp_key_pressed(input::mouse_1)) buttons |= 1;
        if(inp_key_pressed(input::mouse_2)) buttons |= 2;
        if(inp_key_pressed(input::mouse_3)) buttons |= 4;
            
        ui_update(mx,my,mx*3.0f,my*3.0f,buttons);
    }

    //int r = menu_render(server_address, str, max_len);
	int r = menu_render(server_address);

    // render butt ugly mouse cursor
    gfx_texture_set(-1);
    gfx_quads_begin();
    gfx_quads_setcolor(0,0,0,1);
    gfx_quads_draw_freeform(mx,my,mx,my,
                                mx+14,my,
                                mx,my+14);
    gfx_quads_setcolor(1,1,1,1);
    gfx_quads_draw_freeform(mx+1,my+1,mx+1,my+1,
                                mx+10,my+1,
                                mx+1,my+10);
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
