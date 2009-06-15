
#include <base/math.hpp>

#include <string.h> // strcmp, strlen, strncpy
#include <stdlib.h> // atoi

#include <engine/e_client_interface.h>

#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/ui.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>

#include "menus.hpp"
#include "motd.hpp"
#include "voting.hpp"

void MENUS::render_game(RECT main_view)
{
	RECT button;
	//RECT votearea;
	ui_hsplit_t(&main_view, 45.0f, &main_view, 0);
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);

	ui_hsplit_t(&main_view, 10.0f, 0, &main_view);
	ui_hsplit_t(&main_view, 25.0f, &main_view, 0);
	ui_vmargin(&main_view, 10.0f, &main_view);
	
	ui_vsplit_r(&main_view, 120.0f, &main_view, &button);
	static int disconnect_button = 0;
	if(ui_do_button(&disconnect_button, localize("Disconnect"), 0, &button, ui_draw_menu_button, 0))
		client_disconnect();

	if(gameclient.snap.local_info && gameclient.snap.gameobj)
	{
		if(gameclient.snap.local_info->team != -1)
		{
			ui_vsplit_l(&main_view, 10.0f, &button, &main_view);
			ui_vsplit_l(&main_view, 120.0f, &button, &main_view);
			static int spectate_button = 0;
			if(ui_do_button(&spectate_button, localize("Spectate"), 0, &button, ui_draw_menu_button, 0))
			{
				gameclient.send_switch_team(-1);
				set_active(false);
			}
		}
		
		if(gameclient.snap.gameobj->flags & GAMEFLAG_TEAMS)
		{
			if(gameclient.snap.local_info->team != 0)
			{
				ui_vsplit_l(&main_view, 10.0f, &button, &main_view);
				ui_vsplit_l(&main_view, 120.0f, &button, &main_view);
				static int spectate_button = 0;
				if(ui_do_button(&spectate_button, localize("Join red"), 0, &button, ui_draw_menu_button, 0))
				{
					gameclient.send_switch_team(0);
					set_active(false);
				}
			}

			if(gameclient.snap.local_info->team != 1)
			{
				ui_vsplit_l(&main_view, 10.0f, &button, &main_view);
				ui_vsplit_l(&main_view, 120.0f, &button, &main_view);
				static int spectate_button = 0;
				if(ui_do_button(&spectate_button, localize("Join blue"), 0, &button, ui_draw_menu_button, 0))
				{
					gameclient.send_switch_team(1);
					set_active(false);
				}
			}
		}
		else
		{
			if(gameclient.snap.local_info->team != 0)
			{
				ui_vsplit_l(&main_view, 10.0f, &button, &main_view);
				ui_vsplit_l(&main_view, 120.0f, &button, &main_view);
				static int spectate_button = 0;
				if(ui_do_button(&spectate_button, localize("Join game"), 0, &button, ui_draw_menu_button, 0))
				{
					gameclient.send_switch_team(0);
					set_active(false);
				}
			}
		}
	}
	
	/*
	RECT bars;
	ui_hsplit_t(&votearea, 10.0f, 0, &votearea);
	ui_hsplit_t(&votearea, 25.0f + 10.0f*3 + 25.0f, &votearea, &bars);

	ui_draw_rect(&votearea, color_tabbar_active, CORNER_ALL, 10.0f);

	ui_vmargin(&votearea, 20.0f, &votearea);
	ui_hmargin(&votearea, 10.0f, &votearea);

	ui_hsplit_b(&votearea, 35.0f, &votearea, &bars);

	if(gameclient.voting->is_voting())
	{
		// do yes button
		ui_vsplit_l(&votearea, 50.0f, &button, &votearea);
		static int yes_button = 0;
		if(ui_do_button(&yes_button, "Yes", 0, &button, ui_draw_menu_button, 0))
			gameclient.voting->vote(1);

		// do no button
		ui_vsplit_l(&votearea, 5.0f, 0, &votearea);
		ui_vsplit_l(&votearea, 50.0f, &button, &votearea);
		static int no_button = 0;
		if(ui_do_button(&no_button, "No", 0, &button, ui_draw_menu_button, 0))
			gameclient.voting->vote(-1);
		
		// do time left
		ui_vsplit_r(&votearea, 50.0f, &votearea, &button);
		char buf[256];
		str_format(buf, sizeof(buf), "%d", gameclient.voting->seconds_left());
		ui_do_label(&button, buf, 24.0f, 0);

		// do description and command
		ui_vsplit_l(&votearea, 5.0f, 0, &votearea);
		ui_do_label(&votearea, gameclient.voting->vote_description(), 14.0f, -1);
		ui_hsplit_t(&votearea, 16.0f, 0, &votearea);
		ui_do_label(&votearea, gameclient.voting->vote_command(), 10.0f, -1);

		// do bars
		ui_hsplit_t(&bars, 10.0f, 0, &bars);
		ui_hmargin(&bars, 5.0f, &bars);
		
		gameclient.voting->render_bars(bars, true);

	}		
	else
	{
		ui_do_label(&votearea, "No vote in progress", 18.0f, -1);
	}*/
}

void MENUS::render_serverinfo(RECT main_view)
{
	// fetch server info
	SERVER_INFO current_server_info;
	client_serverinfo(&current_server_info);
	
	if(!gameclient.snap.local_info)
		return;
	
	// count players for server info-box
	int num_players = 0;
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PLAYER_INFO)
		{
			num_players++;
		}
	}

	// render background
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);
	
	RECT view, serverinfo, gameinfo, motd;
	
	float x = 0.0f;
	float y = 0.0f;
	
	char buf[1024];
	
	// set view to use for all sub-modules
	ui_margin(&main_view, 10.0f, &view);
	
	/* serverinfo */
	ui_hsplit_t(&view, view.h/2-5.0f, &serverinfo, &motd);
	ui_vsplit_l(&serverinfo, view.w/2-5.0f, &serverinfo, &gameinfo);
	ui_draw_rect(&serverinfo, vec4(1,1,1,0.25f), CORNER_ALL, 10.0f);
	
	ui_margin(&serverinfo, 5.0f, &serverinfo);
	
	x = 5.0f;
	y = 0.0f;
	
	gfx_text(0, serverinfo.x+x, serverinfo.y+y, 32, localize("Server info"), 250);
	y += 32.0f+5.0f;
	
	mem_zero(buf, sizeof(buf));
	str_format(
		buf,
		sizeof(buf),
		"%s\n\n"
		"%s: %s\n"
		"%s: %d\n"
		"%s: %s\n"
		"%s: %s\n",
		current_server_info.name,
		localize("Address"), config.ui_server_address,
		localize("Ping"), gameclient.snap.local_info->latency,
		localize("Version"), current_server_info.version,
		localize("Password"), current_server_info.flags&1 ? localize("Yes") : localize("No")
	);
	
	gfx_text(0, serverinfo.x+x, serverinfo.y+y, 20, buf, 250);
	
	{
		RECT button;
		int is_favorite = client_serverbrowse_isfavorite(current_server_info.netaddr);
		ui_hsplit_b(&serverinfo, 20.0f, &serverinfo, &button);
		static int add_fav_button = 0;
		if (ui_do_button(&add_fav_button, localize("Favorite"), is_favorite, &button, ui_draw_checkbox, 0))
		{
			if(is_favorite)
				client_serverbrowse_removefavorite(current_server_info.netaddr);
			else
				client_serverbrowse_addfavorite(current_server_info.netaddr);
		}
	}
	
	/* gameinfo */
	ui_vsplit_l(&gameinfo, 10.0f, 0x0, &gameinfo);
	ui_draw_rect(&gameinfo, vec4(1,1,1,0.25f), CORNER_ALL, 10.0f);
	
	ui_margin(&gameinfo, 5.0f, &gameinfo);
	
	x = 5.0f;
	y = 0.0f;
	
	gfx_text(0, gameinfo.x+x, gameinfo.y+y, 32, localize("Game info"), 250);
	y += 32.0f+5.0f;
	
	mem_zero(buf, sizeof(buf));
	str_format(
		buf,
		sizeof(buf),
		"\n\n"
		"%s: %s\n"
		"%s: %s\n"
		"%s: %d\n"
		"%s: %d\n"
		"\n"
		"%s: %d/%d\n",
		localize("Game type"), current_server_info.gametype,
		localize("Map"), current_server_info.map,
		localize("Score limit"), gameclient.snap.gameobj->score_limit,
		localize("Time limit"), gameclient.snap.gameobj->time_limit,
		localize("Players"), gameclient.snap.num_players, current_server_info.max_players
	);
	gfx_text(0, gameinfo.x+x, gameinfo.y+y, 20, buf, 250);
	
	/* motd */
	ui_hsplit_t(&motd, 10.0f, 0, &motd);
	ui_draw_rect(&motd, vec4(1,1,1,0.25f), CORNER_ALL, 10.0f);
	ui_margin(&motd, 5.0f, &motd);
	y = 0.0f;
	x = 5.0f;
	gfx_text(0, motd.x+x, motd.y+y, 32, localize("MOTD"), -1);
	y += 32.0f+5.0f;
	gfx_text(0, motd.x+x, motd.y+y, 16, gameclient.motd->server_motd, (int)motd.w);
}

static const char *format_command(const char *cmd)
{
	return cmd;
}

void MENUS::render_servercontrol_server(RECT main_view)
{
	int num_options = 0;
	for(VOTING::VOTEOPTION *option = gameclient.voting->first; option; option = option->next)
		num_options++;

	static int votelist = 0;
	RECT list = main_view;
	ui_do_listbox_start(&votelist, &list, 24.0f, localize("Settings"), num_options, callvote_selectedoption);
	
	for(VOTING::VOTEOPTION *option = gameclient.voting->first; option; option = option->next)
	{
		LISTBOXITEM item = ui_do_listbox_nextitem(option);
		
		if(item.visible)
			ui_do_label(&item.rect, format_command(option->command), 16.0f, -1);
	}
	
	callvote_selectedoption = ui_do_listbox_end();
}

void MENUS::render_servercontrol_kick(RECT main_view)
{
	// draw header
	RECT header, footer;
	ui_hsplit_t(&main_view, 20, &header, &main_view);
	ui_draw_rect(&header, vec4(1,1,1,0.25f), CORNER_T, 5.0f); 
	ui_do_label(&header, localize("Players"), 18.0f, 0);

	// draw footers	
	ui_hsplit_b(&main_view, 20, &main_view, &footer);
	ui_draw_rect(&footer, vec4(1,1,1,0.25f), CORNER_B, 5.0f); 
	ui_vsplit_l(&footer, 10.0f, 0, &footer);

	// players
	ui_draw_rect(&main_view, vec4(0,0,0,0.15f), 0, 0);
	RECT list = main_view;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!gameclient.snap.player_infos[i])
			continue;

		RECT button;
		ui_hsplit_t(&list, button_height, &button, &list);
		
		if(ui_do_button((char *)&gameclient.snap+i, "", callvote_selectedplayer == i, &button, ui_draw_list_row, 0))
			callvote_selectedplayer = i;

		TEE_RENDER_INFO info = gameclient.clients[i].render_info;
		info.size = button.h;
		render_tee(ANIMSTATE::get_idle(), &info, EMOTE_NORMAL, vec2(1,0), vec2(button.x+button.h/2, button.y+button.h/2));

		button.x += button.h;
		ui_do_label(&button, gameclient.clients[i].name, 18.0f, -1);
	}
}

void MENUS::render_servercontrol(RECT main_view)
{
	static int control_page = 0;
	
	// render background
	RECT temp, tabbar;
	ui_vsplit_r(&main_view, 120.0f, &main_view, &tabbar);
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_B|CORNER_TL, 10.0f);
	ui_hsplit_t(&tabbar, 50.0f, &temp, &tabbar);
	ui_draw_rect(&temp, color_tabbar_active, CORNER_R, 10.0f);
	
	ui_hsplit_t(&main_view, 10.0f, 0, &main_view);
	
	RECT button;
	
	const char *tabs[] = {
		localize("Settings"),
		localize("Kick")};
	int num_tabs = (int)(sizeof(tabs)/sizeof(*tabs));
	
	for(int i = 0; i < num_tabs; i++)
	{
		ui_hsplit_t(&tabbar, 10, &button, &tabbar);
		ui_hsplit_t(&tabbar, 26, &button, &tabbar);
		if(ui_do_button(tabs[i], tabs[i], control_page == i, &button, ui_draw_settings_tab_button, 0))
		{
			control_page = i;
			callvote_selectedplayer = -1;
			callvote_selectedoption = -1;
		}
	}
		
	ui_margin(&main_view, 10.0f, &main_view);
	RECT bottom;
	ui_hsplit_b(&main_view, button_height + 5*2, &main_view, &bottom);
	ui_hmargin(&bottom, 5.0f, &bottom);
	
	// render page		
	if(control_page == 0)
		render_servercontrol_server(main_view);
	else if(control_page == 1)
		render_servercontrol_kick(main_view);
		

	{
		RECT button;
		ui_vsplit_r(&bottom, 120.0f, &bottom, &button);
		
		static int callvote_button = 0;
		if(ui_do_button(&callvote_button, localize("Call vote"), 0, &button, ui_draw_menu_button, 0))
		{
			if(control_page == 0)
			{
				//
				gameclient.voting->callvote_option(callvote_selectedoption);
				/*
				if(callvote_selectedmap >= 0 && callvote_selectedmap < gameclient.maplist->num())
					gameclient.voting->callvote_map(gameclient.maplist->name(callvote_selectedmap));*/
			}
			else if(control_page == 1)
			{
				if(callvote_selectedplayer >= 0 && callvote_selectedplayer < MAX_CLIENTS &&
					gameclient.snap.player_infos[callvote_selectedplayer])
				{
					gameclient.voting->callvote_kick(callvote_selectedplayer);
					set_active(false);
				}
			}
		}
	}		
}

