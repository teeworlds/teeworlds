
#include <string.h> // strcmp, strlen, strncpy
#include <stdlib.h> // atoi

#include <engine/e_client_interface.h>

extern "C" {
	#include <engine/client/ec_font.h>
}

#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/ui.hpp>
#include <game/client/render.hpp>
#include "menus.hpp"
#include <game/version.hpp>

void MENUS::render_serverbrowser_serverlist(RECT view)
{
	RECT headers;
	RECT status;
	
	ui_hsplit_t(&view, listheader_height, &headers, &view);
	ui_hsplit_b(&view, 20.0f, &view, &status);
	
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
		
		COL_FLAG_LOCK=0,
		COL_FLAG_PURE,
		COL_FLAG_FAV,
		COL_NAME,
		COL_GAMETYPE,
		COL_MAP,
		COL_PLAYERS,
		COL_PING,
		COL_PROGRESS,
		COL_VERSION,
	};
	
	static column cols[] = {
		{-1,			-1,						" ",		-1, 2.0f, 0, {0}, {0}},
		{COL_FLAG_LOCK,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_PURE,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_FAV,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
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
	int num_players = 0;

	selected_index = -1;

	/*for (int i = 0; i < num_servers; i++)
	{
		SERVER_INFO *item = client_serverbrowse_sorted_get(i);
		num_players += item->num_players;
	}*/
	
	for (int i = 0; i < num_servers; i++)
	{
		int item_index = i;
		SERVER_INFO *item = client_serverbrowse_sorted_get(item_index);
		num_players += item->num_players;
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
			
			if(id == COL_FLAG_LOCK)
			{
				if(item->flags & SRVFLAG_PASSWORD)
					ui_draw_browse_icon(SPRITE_BROWSE_LOCK, &button);
			}
			else if(id == COL_FLAG_PURE)
			{
				if(strncmp(item->gametype, "DM", 2) == 0 || strncmp(item->gametype, "TDM", 3) == 0 || strncmp(item->gametype, "CTF", 3) == 0)
					ui_draw_browse_icon(SPRITE_BROWSE_PURE, &button);
			}
			else if(id == COL_FLAG_FAV)
			{
				if(item->favorite)
					ui_draw_browse_icon(SPRITE_BROWSE_HEART, &button);
			}
			else if(id == COL_NAME)
			{
				TEXT_CURSOR cursor;
				gfx_text_set_cursor(&cursor, button.x, button.y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				cursor.line_width = button.w;
				
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
			{
				ui_do_label(&button, item->gametype, 12.0f, 0);
			}

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
	

	// render status
	ui_draw_rect(&status, vec4(1,1,1,0.25f), CORNER_B, 5.0f);
	ui_vmargin(&status, 50.0f, &status);
	char buf[128];
	if(client_serverbrowse_refreshingmasters())
		str_format(buf, sizeof(buf), "Refreshing master servers...");
	else
		str_format(buf, sizeof(buf), "%d of %d servers, %d players", client_serverbrowse_sorted_num(), client_serverbrowse_num(), num_players);
	ui_do_label(&status, buf, 14.0f, -1);
}

void MENUS::render_serverbrowser_filters(RECT view)
{
	// filters
	RECT button;

	ui_hsplit_t(&view, 5.0f, 0, &view);
	ui_vsplit_l(&view, 5.0f, 0, &view);
	ui_vsplit_r(&view, 5.0f, &view, 0);
	ui_hsplit_b(&view, 5.0f, &view, 0);

	// render filters
	ui_hsplit_t(&view, 20.0f, &button, &view);
	if (ui_do_button(&config.b_filter_empty, "Has people playing", config.b_filter_empty, &button, ui_draw_checkbox, 0))
		config.b_filter_empty ^= 1;

	ui_hsplit_t(&view, 20.0f, &button, &view);
	if (ui_do_button(&config.b_filter_full, "Server not full", config.b_filter_full, &button, ui_draw_checkbox, 0))
		config.b_filter_full ^= 1;

	ui_hsplit_t(&view, 20.0f, &button, &view);
	if (ui_do_button(&config.b_filter_pw, "No password", config.b_filter_pw, &button, ui_draw_checkbox, 0))
		config.b_filter_pw ^= 1;

	ui_hsplit_t(&view, 20.0f, &button, &view);
	if (ui_do_button((char *)&config.b_filter_compatversion, "Compatible Version", config.b_filter_compatversion, &button, ui_draw_checkbox, 0))
		config.b_filter_compatversion ^= 1;
	
	ui_hsplit_t(&view, 20.0f, &button, &view);
	if (ui_do_button((char *)&config.b_filter_pure, "Only pure", config.b_filter_pure, &button, ui_draw_checkbox, 0))
		config.b_filter_pure ^= 1;

	ui_hsplit_t(&view, 20.0f, &button, &view);
	ui_do_label(&button, "Game types: ", 14.0f, -1);
	ui_vsplit_l(&button, 95.0f, 0, &button);
	ui_margin(&button, 1.0f, &button);
	ui_do_edit_box(&config.b_filter_gametype, &button, config.b_filter_gametype, sizeof(config.b_filter_gametype), 14.0f);
	
	ui_hsplit_t(&view, 20.0f, &button, &view);
	ui_do_label(&button, "Quick search: ", 14.0f, -1);
	ui_vsplit_l(&button, 95.0f, 0, &button);
	ui_margin(&button, 1.0f, &button);
	ui_do_edit_box(&config.b_filter_string, &button, config.b_filter_string, sizeof(config.b_filter_string), 14.0f);

	{
		ui_hsplit_t(&view, 20.0f, &button, &view);
		RECT editbox;
		ui_vsplit_l(&button, 40.0f, &editbox, &button);
		ui_vsplit_l(&button, 5.0f, &button, &button);
		
		char buf[8];
		str_format(buf, sizeof(buf), "%d", config.b_filter_ping);
		ui_do_edit_box(&config.b_filter_ping, &editbox, buf, sizeof(buf), 14.0f);
		config.b_filter_ping = atoi(buf);
		
		ui_do_label(&button, "Maximum ping", 14.0f, -1);
	}
	
	//ui_vsplit_r(&buttons, 30.0f, &buttons, &button);
	ui_hsplit_b(&view, button_height, &view, &button);
	static int clear_button = 0;
	if(ui_do_button(&clear_button, "Reset Filter", 0, &button, ui_draw_menu_button, 0))
	{
		config.b_filter_full = 0;
		config.b_filter_empty = 0;
		config.b_filter_pw = 0;
		config.b_filter_ping = 999;
		config.b_filter_gametype[0] = 0;
		config.b_filter_compatversion = 1;
		config.b_filter_string[0] = 0;
	}
}

void MENUS::render_serverbrowser_serverdetail(RECT view)
{
	RECT server_details = view;
	RECT server_scoreboard, server_header;
	
	SERVER_INFO *selected_server = client_serverbrowse_sorted_get(selected_index);
	
	//ui_vsplit_l(&server_details, 10.0f, 0x0, &server_details);

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

		// 
		{
			RECT button;
			ui_hsplit_b(&server_details, 20.0f, &server_details, &button);
			static int add_fav_button = 0;
			if (ui_do_button(&add_fav_button, "Favorite", selected_server->favorite, &button, ui_draw_checkbox, 0))
			{
				if(selected_server->favorite)
					client_serverbrowse_removefavorite(selected_server->netaddr);
				else
					client_serverbrowse_addfavorite(selected_server->netaddr);
			}
		}
		//ui_do_label(&row, temp, font_size, -1);		

		ui_vsplit_l(&server_details, 5.0f, 0x0, &server_details);
		ui_vsplit_l(&server_details, 80.0f, &left_column, &right_column);

		for (unsigned int i = 0; i < sizeof(labels) / sizeof(labels[0]); i++)
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
			
		}
	}
}

void MENUS::render_serverbrowser(RECT main_view)
{
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);
	
	RECT view;
	ui_margin(&main_view, 10.0f, &view);
	
	/*
		+-----------------+ +------+
		|                 | |      |
		|                 | | tool |
		|                 | | box  |
		|                 | |      |
		|                 | +------+
		+-----------------+  button
	      status toolbar      box
	*/
	
	
	//RECT filters;
	RECT status_toolbar;
	RECT toolbox;
	RECT button_box;

	// split off a piece for filters, details and scoreboard
	ui_vsplit_r(&view, 200.0f, &view, &toolbox);
	ui_hsplit_b(&toolbox, 120.0f, &toolbox, &button_box);
	ui_hsplit_b(&view, button_height+5.0f, &view, &status_toolbar);

	render_serverbrowser_serverlist(view);
	
	static int toolbox_page = 0;
	
	ui_vsplit_l(&toolbox, 5.0f, 0, &toolbox);

	// do tabbar
	{
		RECT tab_bar;
		RECT tabbutton0, tabbutton1;
		ui_hsplit_t(&toolbox, 30, &tab_bar, &toolbox);
	
		ui_vsplit_mid(&tab_bar, &tabbutton0, &tabbutton1);
		ui_vsplit_r(&tabbutton0, 5.0f, &tabbutton0, 0);
		ui_vsplit_l(&tabbutton1, 5.0f, 0, &tabbutton1);
		
		static int filters_tab = 0;
		if (ui_do_button(&filters_tab, "Filter", toolbox_page==0, &tabbutton0, ui_draw_menu_tab_button, 0))
			toolbox_page = 0;
			
		static int info_tab = 0;
		if (ui_do_button(&info_tab, "Info", toolbox_page==1, &tabbutton1, ui_draw_menu_tab_button, 0))
			toolbox_page = 1;
	}

	ui_draw_rect(&toolbox, vec4(0,0,0,0.15f), 0, 0);
	
	ui_hsplit_t(&toolbox, 5.0f, 0, &toolbox);
	
	if(toolbox_page == 0)
		render_serverbrowser_filters(toolbox);
	else if(toolbox_page == 1)
		render_serverbrowser_serverdetail(toolbox);

	{
		ui_hsplit_t(&status_toolbar, 5.0f, 0, &status_toolbar);
		
		RECT button;
		//ui_vsplit_r(&buttons, 20.0f, &buttons, &button);
		ui_vsplit_r(&status_toolbar, 100.0f, &status_toolbar, &button);
		ui_vmargin(&button, 2.0f, &button);
		static int refresh_button = 0;
		if(ui_do_button(&refresh_button, "Refresh", 0, &button, ui_draw_menu_button, 0))
		{
			if(config.ui_page == PAGE_INTERNET)
				client_serverbrowse_refresh(0);
			else if(config.ui_page == PAGE_LAN)
				client_serverbrowse_refresh(1);
		}
		
		char buf[512];
		if(strcmp(client_latestversion(), "0") != 0)
			str_format(buf, sizeof(buf), "Teeworlds %s is out! Download it at www.teeworlds.com! Current version: %s", client_latestversion(), GAME_VERSION);
		else
			str_format(buf, sizeof(buf), "Current version: %s", GAME_VERSION);
		ui_do_label(&status_toolbar, buf, 14.0f, -1);
	}
	
	// do the button box
	{
		
		ui_vsplit_l(&button_box, 5.0f, 0, &button_box);
		ui_vsplit_r(&button_box, 5.0f, &button_box, 0);
		
		RECT button;
		ui_hsplit_b(&button_box, button_height, &button_box, &button);
		ui_vsplit_r(&button, 120.0f, 0, &button);
		ui_vmargin(&button, 2.0f, &button);
		//ui_vmargin(&button, 2.0f, &button);
		static int join_button = 0;
		if(ui_do_button(&join_button, "Connect", 0, &button, ui_draw_menu_button, 0)) // || inp_key_down(KEY_ENTER))
			client_connect(config.ui_server_address);
					
		ui_hsplit_b(&button_box, 5.0f, &button_box, &button);
		ui_hsplit_b(&button_box, 20.0f, &button_box, &button);
		ui_do_edit_box(&config.ui_server_address, &button, config.ui_server_address, sizeof(config.ui_server_address), 14.0f);
		ui_hsplit_b(&button_box, 20.0f, &button_box, &button);
		ui_do_label(&button, "Host address:", 14.0f, -1);
	}
}
