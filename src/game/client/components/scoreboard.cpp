#include <string.h>

#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>
#include <game/client/gc_render.hpp>
#include "scoreboard.hpp"

void SCOREBOARD::render_goals(float x, float y, float w)
{
	float h = 50.0f;

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 10.0f);
	gfx_quads_end();

	// render goals
	//y = ystart+h-54;
	if(gameclient.snap.gameobj && gameclient.snap.gameobj->time_limit)
	{
		char buf[64];
		str_format(buf, sizeof(buf), "Time Limit: %d min", gameclient.snap.gameobj->time_limit);
		gfx_text(0, x+w/2, y, 24.0f, buf, -1);
	}
	if(gameclient.snap.gameobj && gameclient.snap.gameobj->score_limit)
	{
		char buf[64];
		str_format(buf, sizeof(buf), "Score Limit: %d", gameclient.snap.gameobj->score_limit);
		gfx_text(0, x+40, y, 24.0f, buf, -1);
	}
}

void SCOREBOARD::render_spectators(float x, float y, float w)
{
	char buffer[1024*4];
	int count = 0;
	float h = 120.0f;
	
	str_copy(buffer, "Spectators: ", sizeof(buffer));

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 10.0f);
	gfx_quads_end();
	
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PLAYER_INFO)
		{
			const NETOBJ_PLAYER_INFO *info = (const NETOBJ_PLAYER_INFO *)data;
			if(info->team == -1)
			{
				if(count)
					strcat(buffer, ", ");
				strcat(buffer, gameclient.clients[info->cid].name);
				count++;
			}
		}
	}
	
	gfx_text(0, x+10, y, 32, buffer, (int)w-20);
}

void SCOREBOARD::render_scoreboard(float x, float y, float w, int team, const char *title)
{
	//float ystart = y;
	float h = 750.0f;

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 40.0f);
	gfx_quads_end();

	// render title
	if(!title)
	{
		if(gameclient.snap.gameobj->game_over)
			title = "Game Over";
		else
			title = "Score Board";
	}

	float tw = gfx_text_width(0, 48, title, -1);

	if(team == -1)
	{
		gfx_text(0, x+w/2-tw/2, y, 48, title, -1);
	}
	else
	{
		gfx_text(0, x+10, y, 48, title, -1);

		if(gameclient.snap.gameobj)
		{
			char buf[128];
			int score = team ? gameclient.snap.gameobj->teamscore_blue : gameclient.snap.gameobj->teamscore_red;
			str_format(buf, sizeof(buf), "%d", score);
			tw = gfx_text_width(0, 48, buf, -1);
			gfx_text(0, x+w-tw-30, y, 48, buf, -1);
		}
	}

	y += 54.0f;

	// find players
	const NETOBJ_PLAYER_INFO *players[MAX_CLIENTS] = {0};
	int num_players = 0;
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PLAYER_INFO)
		{
			players[num_players] = (const NETOBJ_PLAYER_INFO *)data;
			num_players++;
		}
	}

	// sort players
	for(int k = 0; k < num_players; k++) // ffs, bubblesort
	{
		for(int i = 0; i < num_players-k-1; i++)
		{
			if(players[i]->score < players[i+1]->score)
			{
				const NETOBJ_PLAYER_INFO *tmp = players[i];
				players[i] = players[i+1];
				players[i+1] = tmp;
			}
		}
	}

	// render headlines
	gfx_text(0, x+10, y, 24.0f, "Score", -1);
	gfx_text(0, x+125, y, 24.0f, "Name", -1);
	gfx_text(0, x+w-70, y, 24.0f, "Ping", -1);
	y += 29.0f;

	// render player scores
	for(int i = 0; i < num_players; i++)
	{
		const NETOBJ_PLAYER_INFO *info = players[i];

		// make sure that we render the correct team
		if(team == -1 || info->team != team)
			continue;

		char buf[128];
		float font_size = 35.0f;
		if(info->local)
		{
			// background so it's easy to find the local player
			gfx_texture_set(-1);
			gfx_quads_begin();
			gfx_setcolor(1,1,1,0.25f);
			draw_round_rect(x, y, w-20, 48, 20.0f);
			gfx_quads_end();
		}

		str_format(buf, sizeof(buf), "%4d", info->score);
		gfx_text(0, x+60-gfx_text_width(0, font_size,buf,-1), y, font_size, buf, -1);
		
		gfx_text(0, x+128, y, font_size, gameclient.clients[info->cid].name, -1);

		str_format(buf, sizeof(buf), "%4d", info->latency);
		float tw = gfx_text_width(0, font_size, buf, -1);
		gfx_text(0, x+w-tw-35, y, font_size, buf, -1);

		// render avatar
		if((gameclient.snap.flags[0] && gameclient.snap.flags[0]->carried_by == info->cid) ||
			(gameclient.snap.flags[1] && gameclient.snap.flags[1]->carried_by == info->cid))
		{
			gfx_blend_normal();
			gfx_texture_set(data->images[IMAGE_GAME].id);
			gfx_quads_begin();

			if(info->team == 0) select_sprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
			else select_sprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
			
			float size = 64.0f;
			gfx_quads_drawTL(x+55, y-15, size/2, size);
			gfx_quads_end();
		}
		
		render_tee(ANIMSTATE::get_idle(), &gameclient.clients[info->cid].render_info, EMOTE_NORMAL, vec2(1,0), vec2(x+90, y+28));

		
		y += 50.0f;
	}
}

void SCOREBOARD::on_render()
{
	
	// TODO: repair me
	/*
	bool do_scoreboard = false;

	// if we are dead
	if(!spectate && (!gameclient.snap.local_character || gameclient.snap.local_character->health < 0))
		do_scoreboard = true;
	
	// if we the game is over
	if(gameclient.snap.gameobj && gameclient.snap.gameobj->game_over)
		do_scoreboard = true;*/

	
	float width = 400*3.0f*gfx_screenaspect();
	float height = 400*3.0f;
	
	gfx_mapscreen(0, 0, width, height);

	float w = 650.0f;

	if(gameclient.snap.gameobj && !(gameclient.snap.gameobj->flags&GAMEFLAG_TEAMS))
	{
		render_scoreboard(width/2-w/2, 150.0f, w, 0, 0);
		//render_scoreboard(gameobj, 0, 0, -1, 0);
	}
	else
	{
			
		if(gameclient.snap.gameobj && gameclient.snap.gameobj->game_over)
		{
			const char *text = "DRAW!";
			if(gameclient.snap.gameobj->teamscore_red > gameclient.snap.gameobj->teamscore_blue)
				text = "Red Team Wins!";
			else if(gameclient.snap.gameobj->teamscore_blue > gameclient.snap.gameobj->teamscore_red)
				text = "Blue Team Wins!";
				
			float w = gfx_text_width(0, 92.0f, text, -1);
			gfx_text(0, width/2-w/2, 45, 92.0f, text, -1);
		}
		
		render_scoreboard(width/2-w-20, 150.0f, w, 0, "Red Team");
		render_scoreboard(width/2 + 20, 150.0f, w, 1, "Blue Team");
	}

	render_goals(width/2-w/2, 150+750+25, w);
	render_spectators(width/2-w/2, 150+750+25+50+25, w);
}
