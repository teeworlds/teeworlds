
#include <string.h> // strcmp, strlen, strncpy
#include <stdlib.h> // atoi

#include <engine/e_client_interface.h>

extern "C" {
	#include <engine/client/ec_font.h>
}

#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/gc_ui.hpp>
#include <game/client/gc_render.hpp>
#include "menus.hpp"

void MENUS::render_serverbrowser(RECT main_view)
{
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);
	
	RECT view;
	ui_margin(&main_view, 10.0f, &view);
	
	RECT headers;
	RECT filters;
	RECT status;
	RECT toolbox;
	RECT server_details;
	RECT server_scoreboard;

	//ui_hsplit_t(&view, 20.0f, &status, &view);
	ui_hsplit_b(&view, 110.0f, &view, &filters);

	// split off a piece for details and scoreboard
	ui_vsplit_r(&view, 200.0f, &view, &server_details);

	// server list
	ui_hsplit_t(&view, 16.0f, &headers, &view);
	//ui_hsplit_b(&view, 110.0f, &view, &filters);
	ui_hsplit_b(&view, 5.0f, &view, 0);
	ui_hsplit_b(&view, 20.0f, &view, &status);

	//ui_vsplit_r(&filters, 300.0f, &filters, &toolbox);
	//ui_vsplit_r(&filters, 150.0f, &filters, 0);

	ui_vsplit_mid(&filters, &filters, &toolbox);
	ui_vsplit_r(&filters, 50.0f, &filters, 0);
	
	// split of the scrollbar
	ui_draw_rect(&headers, vec4(1,1,1,0.25f), CORNER_T, 5.0f);
	ui_vsplit_r(&headers, 20.0f, &headers, 0);
	
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
		{COL_PING,		BROWSESORT_PING,		"Ping",		1, 40.0f, FIXED, {0}, {0}},
	};
	
	int num_cols = sizeof(cols)/sizeof(column);
	
	// do layout
	for(int i = 0; i < num_cols; i++)
	{
		if(cols[i].direction == -1)
		{
			ui_vsplit_l(&headers, cols[i].width, &cols[i].rect, &headers);
			
			if(i+1 < num_cols)
			{
				//cols[i].flags |= SPACER;
				ui_vsplit_l(&headers, 2, &cols[i].spacer, &headers);
			}
		}
	}
	
	for(int i = num_cols-1; i >= 0; i--)
	{
		if(cols[i].direction == 1)
		{
			ui_vsplit_r(&headers, cols[i].width, &headers, &cols[i].rect);
			ui_vsplit_r(&headers, 2, &headers, &cols[i].spacer);
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
		if(ui_do_button(cols[i].caption, cols[i].caption, config.b_sort == cols[i].sort, &cols[i].rect, ui_draw_grid_header, 0))
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
	
	ui_draw_rect(&view, vec4(0,0,0,0.15f), 0, 0);
	
	RECT scroll;
	ui_vsplit_r(&view, 15, &view, &scroll);
	
	int num_servers = client_serverbrowse_sorted_num();
	
	int num = (int)(view.h/cols[0].rect.h);
	static int scrollbar = 0;
	static float scrollvalue = 0;
	//static int selected_index = -1;
	ui_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui_do_scrollbar_v(&scrollbar, &scroll, scrollvalue);
	
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
	ui_clip_enable(&view);
	
	int start = (int)(scrollnum*scrollvalue);
	if(start < 0)
		start = 0;
	
	RECT original_view = view;
	view.y -= scrollvalue*scrollnum*cols[0].rect.h;
	
	int new_selected = -1;
	int selected_index = -1;
	int num_players = 0;

	for (int i = 0; i < num_servers; i++)
	{
		SERVER_INFO *item = client_serverbrowse_sorted_get(i);
		num_players += item->num_players;
	}
	
	for (int i = 0; i < num_servers; i++)
	{
		int item_index = i;
		SERVER_INFO *item = client_serverbrowse_sorted_get(item_index);
		RECT row;
        RECT select_hit_box;
			
		int selected = strcmp(item->address, config.ui_server_address) == 0; //selected_index==item_index;
				
		ui_hsplit_t(&view, 17.0f, &row, &view);
		select_hit_box = row;
	
		if(selected)
		{
			selected_index = i;
			RECT r = row;
			ui_margin(&r, 1.5f, &r);
			ui_draw_rect(&r, vec4(1,1,1,0.5f), CORNER_ALL, 4.0f);
		}


		// make sure that only those in view can be selected
		if(row.y+row.h > original_view.y)
		{
			if(select_hit_box.y < original_view.y) // clip the selection
			{
				select_hit_box.h -= original_view.y-select_hit_box.y;
				select_hit_box.y = original_view.y;
			}
			
			if(ui_do_button(item, "", selected, &select_hit_box, 0, 0))
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

			//s = ui_do_button(item, "L", l, &button, ui_draw_browse_icon, 0);
			
			if(id == COL_FLAGS)
			{
				if(item->flags&1)
					ui_draw_browse_icon(0x100, &button);
			}
			else if(id == COL_NAME)
			{
				TEXT_CURSOR cursor;
				gfx_text_set_cursor(&cursor, button.x, button.y, 12.0f, TEXTFLAG_RENDER);
				
				if(config.b_filter_string[0] && (item->quicksearch_hit&BROWSEQUICK_SERVERNAME))
				{
					// highlight the parts that matches
					const char *s = str_find_nocase(item->name, config.b_filter_string);
					if(s)
					{
						gfx_text_ex(&cursor, item->name, (int)(s-item->name));
						gfx_text_color(0.4f,0.4f,1.0f,1);
						gfx_text_ex(&cursor, s, strlen(config.b_filter_string));
						gfx_text_color(1,1,1,1);
						gfx_text_ex(&cursor, s+strlen(config.b_filter_string), -1);
					}
					else
						gfx_text_ex(&cursor, item->name, -1);
				}
				else
					gfx_text_ex(&cursor, item->name, -1);
			}
			else if(id == COL_MAP)
				ui_do_label(&button, item->map, 12.0f, -1);
			else if(id == COL_PLAYERS)
			{
				str_format(temp, sizeof(temp), "%i/%i", item->num_players, item->max_players);
				if(config.b_filter_string[0] && (item->quicksearch_hit&BROWSEQUICK_PLAYERNAME))
					gfx_text_color(0.4f,0.4f,1.0f,1);
				ui_do_label(&button, temp, 12.0f, 1);
				gfx_text_color(1,1,1,1);
			}
			else if(id == COL_PING)
			{
				str_format(temp, sizeof(temp), "%i", item->latency);
				ui_do_label(&button, temp, 12.0f, 1);
			}
			else if(id == COL_PROGRESS)
			{
				if(item->progression > 100)
					item->progression = 100;
				ui_draw_browse_icon(item->progression, &button);
			}
			else if(id == COL_VERSION)
			{
				const char *version = item->version;
				if(strcmp(version, "0.3 e2d7973c6647a13c") == 0) // TODO: remove me later on
					version = "0.3.0";
				ui_do_label(&button, version, 12.0f, 1);
			}			
			else if(id == COL_GAMETYPE)
				ui_do_label(&button, item->gametype, 12.0f, 0);
		}
	}

	ui_clip_disable();
	
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

	ui_vsplit_l(&server_details, 10.0f, 0x0, &server_details);

	// split off a piece to use for scoreboard
	ui_hsplit_t(&server_details, 140.0f, &server_details, &server_scoreboard);
	ui_hsplit_b(&server_details, 10.0f, &server_details, 0x0);

	// server details
	const float font_size = 12.0f;
	ui_hsplit_t(&server_details, 20.0f, &server_header, &server_details);
	ui_draw_rect(&server_header, vec4(1,1,1,0.25f), CORNER_T, 4.0f);
	ui_draw_rect(&server_details, vec4(0,0,0,0.15f), CORNER_B, 4.0f);
	ui_vsplit_l(&server_header, 8.0f, 0x0, &server_header);
	ui_do_label(&server_header, "Server Details: ", font_size+2.0f, -1);

	ui_vsplit_l(&server_details, 5.0f, 0x0, &server_details);

	ui_margin(&server_details, 3.0f, &server_details);

	if (selected_server)
	{
		RECT row;
		static const char *labels[] = { "Version:", "Game Type:", "Progression:", "Ping:" };

		RECT left_column;
		RECT right_column;

		ui_vsplit_l(&server_details, 5.0f, 0x0, &server_details);
		ui_vsplit_l(&server_details, 80.0f, &left_column, &right_column);

		for (int i = 0; i < 4; i++)
		{
			ui_hsplit_t(&left_column, 15.0f, &row, &left_column);
			ui_do_label(&row, labels[i], font_size, -1);
		}

		ui_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui_do_label(&row, selected_server->version, font_size, -1);

		ui_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui_do_label(&row, selected_server->gametype, font_size, -1);

		char temp[16];

		if(selected_server->progression < 0)
			str_format(temp, sizeof(temp), "N/A");
		else
			str_format(temp, sizeof(temp), "%d%%", selected_server->progression);
		ui_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui_do_label(&row, temp, font_size, -1);

		str_format(temp, sizeof(temp), "%d", selected_server->latency);
		ui_hsplit_t(&right_column, 15.0f, &row, &right_column);
		ui_do_label(&row, temp, font_size, -1);
	}
	
	// server scoreboard
	ui_hsplit_b(&server_scoreboard, 10.0f, &server_scoreboard, 0x0);
	ui_hsplit_t(&server_scoreboard, 20.0f, &server_header, &server_scoreboard);
	ui_draw_rect(&server_header, vec4(1,1,1,0.25f), CORNER_T, 4.0f);
	ui_draw_rect(&server_scoreboard, vec4(0,0,0,0.15f), CORNER_B, 4.0f);
	ui_vsplit_l(&server_header, 8.0f, 0x0, &server_header);
	ui_do_label(&server_header, "Scoreboard: ", font_size+2.0f, -1);

	ui_vsplit_l(&server_scoreboard, 5.0f, 0x0, &server_scoreboard);

	ui_margin(&server_scoreboard, 3.0f, &server_scoreboard);

	if (selected_server)
	{
		for (int i = 0; i < selected_server->num_players; i++)
		{
			RECT row;
			char temp[16];
			ui_hsplit_t(&server_scoreboard, 16.0f, &row, &server_scoreboard);

			str_format(temp, sizeof(temp), "%d", selected_server->players[i].score);
			ui_do_label(&row, temp, font_size, -1);

			ui_vsplit_l(&row, 25.0f, 0x0, &row);
		
			TEXT_CURSOR cursor;
			gfx_text_set_cursor(&cursor, row.x, row.y, 12.0f, TEXTFLAG_RENDER);
			
			const char *name = selected_server->players[i].name;
			if(config.b_filter_string[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(name, config.b_filter_string);
				if(s)
				{
					gfx_text_ex(&cursor, name, (int)(s-name));
					gfx_text_color(0.4f,0.4f,1,1);
					gfx_text_ex(&cursor, s, strlen(config.b_filter_string));
					gfx_text_color(1,1,1,1);
					gfx_text_ex(&cursor, s+strlen(config.b_filter_string), -1);
				}
				else
					gfx_text_ex(&cursor, name, -1);
			}
			else
				gfx_text_ex(&cursor, name, -1);
			
			/*ui_do_label(&row, selected_server->player_names[i], font_size, -1);*/
		}
	}
	
	RECT button;
	RECT types;
	ui_hsplit_t(&filters, 20.0f, &button, &filters);
	ui_do_label(&button, "Quick search: ", 14.0f, -1);
	ui_vsplit_l(&button, 95.0f, 0, &button);
	ui_do_edit_box(&config.b_filter_string, &button, config.b_filter_string, sizeof(config.b_filter_string), 14.0f);

	ui_vsplit_l(&filters, 180.0f, &filters, &types);

	// render filters
	ui_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui_do_button(&config.b_filter_empty, "Has people playing", config.b_filter_empty, &button, ui_draw_checkbox, 0))
		config.b_filter_empty ^= 1;

	ui_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui_do_button(&config.b_filter_full, "Server not full", config.b_filter_full, &button, ui_draw_checkbox, 0))
		config.b_filter_full ^= 1;

	ui_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui_do_button(&config.b_filter_pw, "No password", config.b_filter_pw, &button, ui_draw_checkbox, 0))
		config.b_filter_pw ^= 1;

	ui_hsplit_t(&filters, 20.0f, &button, &filters);
	if (ui_do_button((char *)&config.b_filter_compatversion, "Compatible Version", config.b_filter_compatversion, &button, ui_draw_checkbox, 0))
		config.b_filter_compatversion ^= 1;

	// game types
	/*
	ui_hsplit_t(&types, 20.0f, &button, &types);
	if (ui_do_button(&config.b_filter_gametype, "DM", config.b_filter_gametype&(1<<GAME_TYPE_DM), &button, ui_draw_checkbox, 0))
		config.b_filter_gametype ^= (1<<GAME_TYPE_DM);

	ui_hsplit_t(&types, 20.0f, &button, &types);
	if (ui_do_button((char *)&config.b_filter_gametype + 1, "TDM", config.b_filter_gametype&(1<<GAME_TYPE_TDM), &button, ui_draw_checkbox, 0))
		config.b_filter_gametype ^= (1<<GAME_TYPE_TDM);

	ui_hsplit_t(&types, 20.0f, &button, &types);
	if (ui_do_button((char *)&config.b_filter_gametype + 2, "CTF", config.b_filter_gametype&(1<<GAME_TYPE_CTF), &button, ui_draw_checkbox, 0))
		config.b_filter_gametype ^= (1<<GAME_TYPE_CTF);
	*/

	// ping
	ui_hsplit_t(&types, 2.0f, &button, &types);
	ui_hsplit_t(&types, 20.0f, &button, &types);
	{
		RECT editbox;
		ui_vsplit_l(&button, 40.0f, &editbox, &button);
		ui_vsplit_l(&button, 5.0f, &button, &button);
		
		char buf[8];
		str_format(buf, sizeof(buf), "%d", config.b_filter_ping);
		ui_do_edit_box(&config.b_filter_ping, &editbox, buf, sizeof(buf), 14.0f);
		config.b_filter_ping = atoi(buf);
		
		ui_do_label(&button, "Maximum ping", 14.0f, -1);
	}


	// render status
	ui_draw_rect(&status, vec4(1,1,1,0.25f), CORNER_B, 5.0f);
	ui_vmargin(&status, 50.0f, &status);
	char buf[128];
	str_format(buf, sizeof(buf), "%d of %d servers, %d players", client_serverbrowse_sorted_num(), client_serverbrowse_num(), num_players);
	ui_do_label(&status, buf, 14.0f, -1);

	// render toolbox
	{
		RECT buttons, button;
		ui_hsplit_b(&toolbox, 25.0f, &toolbox, &buttons);

		ui_vsplit_r(&buttons, 100.0f, &buttons, &button);
		ui_vmargin(&button, 2.0f, &button);
		static int join_button = 0;
		if(ui_do_button(&join_button, "Connect", 0, &button, ui_draw_menu_button, 0))
			client_connect(config.ui_server_address);

		ui_vsplit_r(&buttons, 20.0f, &buttons, &button);
		ui_vsplit_r(&buttons, 100.0f, &buttons, &button);
		ui_vmargin(&button, 2.0f, &button);
		static int refresh_button = 0;
		if(ui_do_button(&refresh_button, "Refresh", 0, &button, ui_draw_menu_button, 0))
		{
			if(config.ui_page == PAGE_INTERNET)
				client_serverbrowse_refresh(0);
			else if(config.ui_page == PAGE_LAN)
				client_serverbrowse_refresh(1);
		}

		//ui_vsplit_r(&buttons, 30.0f, &buttons, &button);
		ui_vsplit_l(&buttons, 120.0f, &button, &buttons);
		static int clear_button = 0;
		if(ui_do_button(&clear_button, "Reset Filter", 0, &button, ui_draw_menu_button, 0))
		{
			config.b_filter_full = 0;
			config.b_filter_empty = 0;
			config.b_filter_pw = 0;
			config.b_filter_ping = 999;
			config.b_filter_gametype = 0xf;
			config.b_filter_compatversion = 1;
			config.b_filter_string[0] = 0;
		}

		
		ui_hsplit_t(&toolbox, 20.0f, &button, &toolbox);
		ui_do_label(&button, "Host address:", 14.0f, -1);
		ui_vsplit_l(&button, 100.0f, 0, &button);
		ui_do_edit_box(&config.ui_server_address, &button, config.ui_server_address, sizeof(config.ui_server_address), 14.0f);
	}
}
