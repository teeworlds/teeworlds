
#include <string.h> // strcmp, strlen, strncpy
#include <stdlib.h> // atoi

#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/ui.hpp>
#include <game/client/render.hpp>
#include "menus.hpp"
#include <game/localization.hpp>
#include <game/version.hpp>

void MENUS::render_serverbrowser_serverlist(CUIRect view)
{
	CUIRect headers;
	CUIRect status;
	
	view.HSplitTop(listheader_height, &headers, &view);
	view.HSplitBottom(28.0f, &view, &status);
	
	// split of the scrollbar
	RenderTools()->DrawUIRect(&headers, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f);
	headers.VSplitRight(20.0f, &headers, 0);
	
	struct column
	{
		int id;
		int sort;
		LOC_CONSTSTRING caption;
		int direction;
		float width;
		int flags;
		CUIRect rect;
		CUIRect spacer;
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
		COL_VERSION,
	};
	
	static column cols[] = {
		{-1,			-1,						" ",		-1, 2.0f, 0, {0}, {0}},
		{COL_FLAG_LOCK,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_PURE,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_FAV,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_NAME,		BROWSESORT_NAME,		localize("Name"),		0, 300.0f, 0, {0}, {0}},
		{COL_GAMETYPE,	BROWSESORT_GAMETYPE,	localize("Type"),		1, 50.0f, 0, {0}, {0}},
		{COL_MAP,		BROWSESORT_MAP,			localize("Map"), 		1, 100.0f, 0, {0}, {0}},
		{COL_PLAYERS,	BROWSESORT_NUMPLAYERS,	localize("Players"),	1, 60.0f, 0, {0}, {0}},
		{-1,			-1,						" ",		1, 10.0f, 0, {0}, {0}},
		{COL_PING,		BROWSESORT_PING,		localize("Ping"),		1, 40.0f, FIXED, {0}, {0}},
	};
	
	int num_cols = sizeof(cols)/sizeof(column);
	
	// do layout
	for(int i = 0; i < num_cols; i++)
	{
		if(cols[i].direction == -1)
		{
			headers.VSplitLeft(cols[i].width, &cols[i].rect, &headers);
			
			if(i+1 < num_cols)
			{
				//cols[i].flags |= SPACER;
				headers.VSplitLeft(2, &cols[i].spacer, &headers);
			}
		}
	}
	
	for(int i = num_cols-1; i >= 0; i--)
	{
		if(cols[i].direction == 1)
		{
			headers.VSplitRight(cols[i].width, &headers, &cols[i].rect);
			headers.VSplitRight(2, &headers, &cols[i].spacer);
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
		if(DoButton_GridHeader(cols[i].caption, cols[i].caption, config.b_sort == cols[i].sort, &cols[i].rect))
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
	
	RenderTools()->DrawUIRect(&view, vec4(0,0,0,0.15f), 0, 0);
	
	CUIRect scroll;
	view.VSplitRight(15, &view, &scroll);
	
	int num_servers = client_serverbrowse_sorted_num();
	
	// display important messages in the middle of the screen so no
	// users misses it
	{
		CUIRect msgbox = view;
		msgbox.y += view.h/3;
		
		if(active_page == PAGE_INTERNET && client_serverbrowse_refreshingmasters())
			UI()->DoLabel(&msgbox, localize("Refreshing master servers"), 16.0f, 0);
		else if(!client_serverbrowse_num())
			UI()->DoLabel(&msgbox, localize("No servers found"), 16.0f, 0);
		else if(client_serverbrowse_num() && !num_servers)
			UI()->DoLabel(&msgbox, localize("No servers match your filter criteria"), 16.0f, 0);
	}

	int num = (int)(view.h/cols[0].rect.h);
	static int scrollbar = 0;
	static float scrollvalue = 0;
	//static int selected_index = -1;
	scroll.HMargin(5.0f, &scroll);
	scrollvalue = DoScrollbarV(&scrollbar, &scroll, scrollvalue);
	
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
	UI()->ClipEnable(&view);
	
	int start = (int)(scrollnum*scrollvalue);
	if(start < 0)
		start = 0;
	
	CUIRect original_view = view;
	view.y -= scrollvalue*scrollnum*cols[0].rect.h;
	
	int new_selected = -1;
	int num_players = 0;

	selected_index = -1;

	for (int i = 0; i < num_servers; i++)
	{
		SERVER_INFO *item = client_serverbrowse_sorted_get(i);
		num_players += item->num_players;
	}
	
	for (int i = 0; i < num_servers; i++)
	{
		int item_index = i;
		SERVER_INFO *item = client_serverbrowse_sorted_get(item_index);
		CUIRect row;
        CUIRect select_hit_box;
		
		int selected = strcmp(item->address, config.ui_server_address) == 0; //selected_index==item_index;
		
		view.HSplitTop(17.0f, &row, &view);
		select_hit_box = row;
		
		if(selected)
		{
			selected_index = i;
			CUIRect r = row;
			r.Margin(1.5f, &r);
			RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
		}


		// make sure that only those in view can be selected
		if(row.y+row.h > original_view.y)
		{
			if(select_hit_box.y < original_view.y) // clip the selection
			{
				select_hit_box.h -= original_view.y-select_hit_box.y;
				select_hit_box.y = original_view.y;
			}
			
			if(UI()->DoButtonLogic(item, "", selected, &select_hit_box))
			{
				new_selected = item_index;
			}
		}
		
		// check if we need to do more
		if(row.y > original_view.y+original_view.h)
			break;

		for(int c = 0; c < num_cols; c++)
		{
			CUIRect button;
			char temp[64];
			button.x = cols[c].rect.x;
			button.y = row.y;
			button.h = row.h;
			button.w = cols[c].rect.w;
			
			//int s = 0;
			int id = cols[c].id;

			//s = UI()->DoButton(item, "L", l, &button, ui_draw_browse_icon, 0);
			
			if(id == COL_FLAG_LOCK)
			{
				if(item->flags & SRVFLAG_PASSWORD)
					DoButton_BrowseIcon(SPRITE_BROWSE_LOCK, &button);
			}
			else if(id == COL_FLAG_PURE)
			{
				if(strcmp(item->gametype, "DM") == 0 || strcmp(item->gametype, "TDM") == 0 || strcmp(item->gametype, "CTF") == 0)
				{
					// pure server
				}
				else
				{
					// unpure
					DoButton_BrowseIcon(SPRITE_BROWSE_UNPURE, &button);
				}
			}
			else if(id == COL_FLAG_FAV)
			{
				if(item->favorite)
					DoButton_BrowseIcon(SPRITE_BROWSE_HEART, &button);
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
				UI()->DoLabel(&button, item->map, 12.0f, -1);
			else if(id == COL_PLAYERS)
			{
				str_format(temp, sizeof(temp), "%i/%i", item->num_players, item->max_players);
				if(config.b_filter_string[0] && (item->quicksearch_hit&BROWSEQUICK_PLAYERNAME))
					gfx_text_color(0.4f,0.4f,1.0f,1);
				UI()->DoLabel(&button, temp, 12.0f, 1);
				gfx_text_color(1,1,1,1);
			}
			else if(id == COL_PING)
			{
				str_format(temp, sizeof(temp), "%i", item->latency);
				UI()->DoLabel(&button, temp, 12.0f, 1);
			}
			else if(id == COL_VERSION)
			{
				const char *version = item->version;
				if(strcmp(version, "0.3 e2d7973c6647a13c") == 0) // TODO: remove me later on
					version = "0.3.0";
				UI()->DoLabel(&button, version, 12.0f, 1);
			}			
			else if(id == COL_GAMETYPE)
			{
				UI()->DoLabel(&button, item->gametype, 12.0f, 0);
			}

		}
	}

	UI()->ClipDisable();
	
	if(new_selected != -1)
	{
		// select the new server
		SERVER_INFO *item = client_serverbrowse_sorted_get(new_selected);
		strncpy(config.ui_server_address, item->address, sizeof(config.ui_server_address));
		if(inp_mouse_doubleclick())
			client_connect(config.ui_server_address);
	}

	RenderTools()->DrawUIRect(&status, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	status.Margin(5.0f, &status);
	
	// render quick search
	CUIRect quicksearch;
	status.VSplitLeft(250.0f, &quicksearch, &status);
	const char *label = localize("Quick search");
	UI()->DoLabel(&quicksearch, label, 14.0f, -1);
	quicksearch.VSplitLeft(gfx_text_width(0, 14.0f, label, -1), 0, &quicksearch);
	quicksearch.VSplitLeft(5, 0, &quicksearch);
	DoEditBox(&config.b_filter_string, &quicksearch, config.b_filter_string, sizeof(config.b_filter_string), 14.0f);
	
	// render status
	char buf[128];
	str_format(buf, sizeof(buf), localize("%d of %d servers, %d players"), client_serverbrowse_sorted_num(), client_serverbrowse_num(), num_players);
	status.VSplitRight(gfx_text_width(0, 14.0f, buf, -1), 0, &status);
	UI()->DoLabel(&status, buf, 14.0f, -1);
}

void MENUS::render_serverbrowser_filters(CUIRect view)
{
	// filters
	CUIRect button;

	view.HSplitTop(5.0f, 0, &view);
	view.VSplitLeft(5.0f, 0, &view);
	view.VSplitRight(5.0f, &view, 0);
	view.HSplitBottom(5.0f, &view, 0);

	// render filters
	view.HSplitTop(20.0f, &button, &view);
	if (DoButton_CheckBox(&config.b_filter_empty, localize("Has people playing"), config.b_filter_empty, &button))
		config.b_filter_empty ^= 1;

	view.HSplitTop(20.0f, &button, &view);
	if (DoButton_CheckBox(&config.b_filter_full, localize("Server not full"), config.b_filter_full, &button))
		config.b_filter_full ^= 1;

	view.HSplitTop(20.0f, &button, &view);
	if (DoButton_CheckBox(&config.b_filter_pw, localize("No password"), config.b_filter_pw, &button))
		config.b_filter_pw ^= 1;

	view.HSplitTop(20.0f, &button, &view);
	if (DoButton_CheckBox((char *)&config.b_filter_compatversion, localize("Compatible version"), config.b_filter_compatversion, &button))
		config.b_filter_compatversion ^= 1;
	
	view.HSplitTop(20.0f, &button, &view);
	if (DoButton_CheckBox((char *)&config.b_filter_pure, localize("Standard gametype"), config.b_filter_pure, &button))
		config.b_filter_pure ^= 1;

	view.HSplitTop(20.0f, &button, &view);
	/*button.VSplitLeft(20.0f, 0, &button);*/
	if (DoButton_CheckBox((char *)&config.b_filter_pure_map, localize("Standard map"), config.b_filter_pure_map, &button))
		config.b_filter_pure_map ^= 1;
		
	view.HSplitTop(20.0f, &button, &view);
	UI()->DoLabel(&button, localize("Game types"), 14.0f, -1);
	button.VSplitLeft(95.0f, 0, &button);
	button.Margin(1.0f, &button);
	DoEditBox(&config.b_filter_gametype, &button, config.b_filter_gametype, sizeof(config.b_filter_gametype), 14.0f);

	{
		view.HSplitTop(20.0f, &button, &view);
		CUIRect editbox;
		button.VSplitLeft(40.0f, &editbox, &button);
		button.VSplitLeft(5.0f, &button, &button);
		
		char buf[8];
		str_format(buf, sizeof(buf), "%d", config.b_filter_ping);
		DoEditBox(&config.b_filter_ping, &editbox, buf, sizeof(buf), 14.0f);
		config.b_filter_ping = atoi(buf);
		
		UI()->DoLabel(&button, localize("Maximum ping"), 14.0f, -1);
	}
	
	view.HSplitBottom(button_height, &view, &button);
	static int clear_button = 0;
	if(DoButton_Menu(&clear_button, localize("Reset filter"), 0, &button))
	{
		config.b_filter_full = 0;
		config.b_filter_empty = 0;
		config.b_filter_pw = 0;
		config.b_filter_ping = 999;
		config.b_filter_gametype[0] = 0;
		config.b_filter_compatversion = 1;
		config.b_filter_string[0] = 0;
		config.b_filter_pure = 1;
	}
}

void MENUS::render_serverbrowser_serverdetail(CUIRect view)
{
	CUIRect server_details = view;
	CUIRect server_scoreboard, server_header;
	
	SERVER_INFO *selected_server = client_serverbrowse_sorted_get(selected_index);
	
	//server_details.VSplitLeft(10.0f, 0x0, &server_details);

	// split off a piece to use for scoreboard
	server_details.HSplitTop(140.0f, &server_details, &server_scoreboard);
	server_details.HSplitBottom(10.0f, &server_details, 0x0);

	// server details
	const float font_size = 12.0f;
	server_details.HSplitTop(20.0f, &server_header, &server_details);
	RenderTools()->DrawUIRect(&server_header, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&server_details, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	server_header.VSplitLeft(8.0f, 0x0, &server_header);
	UI()->DoLabel(&server_header, localize("Server details"), font_size+2.0f, -1);

	server_details.VSplitLeft(5.0f, 0x0, &server_details);

	server_details.Margin(3.0f, &server_details);

	if (selected_server)
	{
		CUIRect row;
		static LOC_CONSTSTRING labels[] = {
			localize("Version"),
			localize("Game type"),
			localize("Ping")};

		CUIRect left_column;
		CUIRect right_column;

		// 
		{
			CUIRect button;
			server_details.HSplitBottom(20.0f, &server_details, &button);
			static int add_fav_button = 0;
			if(DoButton_CheckBox(&add_fav_button, localize("Favorite"), selected_server->favorite, &button))
			{
				if(selected_server->favorite)
					client_serverbrowse_removefavorite(selected_server->netaddr);
				else
					client_serverbrowse_addfavorite(selected_server->netaddr);
			}
		}
		//UI()->DoLabel(&row, temp, font_size, -1);		

		server_details.VSplitLeft(5.0f, 0x0, &server_details);
		server_details.VSplitLeft(80.0f, &left_column, &right_column);

		for (unsigned int i = 0; i < sizeof(labels) / sizeof(labels[0]); i++)
		{
			left_column.HSplitTop(15.0f, &row, &left_column);
			UI()->DoLabel(&row, labels[i], font_size, -1);
		}

		right_column.HSplitTop(15.0f, &row, &right_column);
		UI()->DoLabel(&row, selected_server->version, font_size, -1);

		right_column.HSplitTop(15.0f, &row, &right_column);
		UI()->DoLabel(&row, selected_server->gametype, font_size, -1);

		char temp[16];
		str_format(temp, sizeof(temp), "%d", selected_server->latency);
		right_column.HSplitTop(15.0f, &row, &right_column);
		UI()->DoLabel(&row, temp, font_size, -1);

	}
	
	// server scoreboard
	
	server_scoreboard.HSplitBottom(10.0f, &server_scoreboard, 0x0);
	server_scoreboard.HSplitTop(20.0f, &server_header, &server_scoreboard);
	RenderTools()->DrawUIRect(&server_header, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&server_scoreboard, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	server_header.VSplitLeft(8.0f, 0x0, &server_header);
	UI()->DoLabel(&server_header, localize("Scoreboard"), font_size+2.0f, -1);

	server_scoreboard.VSplitLeft(5.0f, 0x0, &server_scoreboard);

	server_scoreboard.Margin(3.0f, &server_scoreboard);

	if (selected_server)
	{
		for (int i = 0; i < selected_server->num_players; i++)
		{
			CUIRect row;
			char temp[16];
			server_scoreboard.HSplitTop(16.0f, &row, &server_scoreboard);

			str_format(temp, sizeof(temp), "%d", selected_server->players[i].score);
			UI()->DoLabel(&row, temp, font_size, -1);

			row.VSplitLeft(25.0f, 0x0, &row);
		
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

void MENUS::render_serverbrowser(CUIRect main_view)
{
	RenderTools()->DrawUIRect(&main_view, color_tabbar_active, CUI::CORNER_ALL, 10.0f);
	
	CUIRect view;
	main_view.Margin(10.0f, &view);
	
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
	
	
	//CUIRect filters;
	CUIRect status_toolbar;
	CUIRect toolbox;
	CUIRect button_box;

	// split off a piece for filters, details and scoreboard
	view.VSplitRight(200.0f, &view, &toolbox);
	toolbox.HSplitBottom(80.0f, &toolbox, &button_box);
	view.HSplitBottom(button_height+5.0f, &view, &status_toolbar);

	render_serverbrowser_serverlist(view);
	
	static int toolbox_page = 0;
	
	toolbox.VSplitLeft(5.0f, 0, &toolbox);

	// do tabbar
	{
		CUIRect tab_bar;
		CUIRect tabbutton0, tabbutton1;
		toolbox.HSplitTop(22.0f, &tab_bar, &toolbox);
	
		tab_bar.VSplitMid(&tabbutton0, &tabbutton1);
		tabbutton0.VSplitRight(5.0f, &tabbutton0, 0);
		tabbutton1.VSplitLeft(5.0f, 0, &tabbutton1);
		
		static int filters_tab = 0;
		if (DoButton_MenuTab(&filters_tab, localize("Filter"), toolbox_page==0, &tabbutton0, 0))
			toolbox_page = 0;
			
		static int info_tab = 0;
		if (DoButton_MenuTab(&info_tab, localize("Info"), toolbox_page==1, &tabbutton1, 0))
			toolbox_page = 1;
	}

	RenderTools()->DrawUIRect(&toolbox, vec4(0,0,0,0.15f), 0, 0);
	
	toolbox.HSplitTop(5.0f, 0, &toolbox);
	
	if(toolbox_page == 0)
		render_serverbrowser_filters(toolbox);
	else if(toolbox_page == 1)
		render_serverbrowser_serverdetail(toolbox);

	{
		status_toolbar.HSplitTop(5.0f, 0, &status_toolbar);
		
		CUIRect button;
		//buttons.VSplitRight(20.0f, &buttons, &button);
		status_toolbar.VSplitRight(110.0f, &status_toolbar, &button);
		button.VMargin(2.0f, &button);
		static int refresh_button = 0;
		if(DoButton_Menu(&refresh_button, localize("Refresh"), 0, &button))
		{
			if(config.ui_page == PAGE_INTERNET)
				client_serverbrowse_refresh(BROWSETYPE_INTERNET);
			else if(config.ui_page == PAGE_LAN)
				client_serverbrowse_refresh(BROWSETYPE_LAN);
			else if(config.ui_page == PAGE_FAVORITES)
				client_serverbrowse_refresh(BROWSETYPE_FAVORITES);
		}
		
		char buf[512];
		if(strcmp(client_latestversion(), "0") != 0)
			str_format(buf, sizeof(buf), localize("Teeworlds %s is out! Download it at www.teeworlds.com!"), client_latestversion());
		else
			str_format(buf, sizeof(buf), localize("Current version: %s"), GAME_VERSION);
		UI()->DoLabel(&status_toolbar, buf, 14.0f, -1);
	}
	
	// do the button box
	{
		
		button_box.VSplitLeft(5.0f, 0, &button_box);
		button_box.VSplitRight(5.0f, &button_box, 0);
		
		CUIRect button;
		button_box.HSplitBottom(button_height, &button_box, &button);
		button.VSplitRight(120.0f, 0, &button);
		button.VMargin(2.0f, &button);
		//button.VMargin(2.0f, &button);
		static int join_button = 0;
		if(DoButton_Menu(&join_button, localize("Connect"), 0, &button) || enter_pressed)
		{
			client_connect(config.ui_server_address);
			enter_pressed = false;
		}
					
		button_box.HSplitBottom(5.0f, &button_box, &button);
		button_box.HSplitBottom(20.0f, &button_box, &button);
		DoEditBox(&config.ui_server_address, &button, config.ui_server_address, sizeof(config.ui_server_address), 14.0f);
		button_box.HSplitBottom(20.0f, &button_box, &button);
		UI()->DoLabel(&button, localize("Host address"), 14.0f, -1);
	}
}
