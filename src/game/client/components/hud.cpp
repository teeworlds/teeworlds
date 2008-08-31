#include <memory.h> // memcmp

#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/layers.hpp>

#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>
#include <game/client/gc_render.hpp>

#include "controls.hpp"
#include "camera.hpp"
#include "hud.hpp"

HUD::HUD()
{
	
}
	
void HUD::on_reset()
{
}

void HUD::render_goals()
{
	// TODO: split this up into these:
	// render_gametimer
	// render_suddendeath
	// render_scorehud
	// render_warmuptimer
	
	int gameflags = gameclient.snap.gameobj->flags;
	
	float whole = 300*gfx_screenaspect();
	float half = whole/2.0f;


	gfx_mapscreen(0,0,300*gfx_screenaspect(),300);
	if(!gameclient.snap.gameobj->sudden_death)
	{
		char buf[32];
		int time = 0;
		if(gameclient.snap.gameobj->time_limit)
		{
			time = gameclient.snap.gameobj->time_limit*60 - ((client_tick()-gameclient.snap.gameobj->round_start_tick)/client_tickspeed());

			if(gameclient.snap.gameobj->game_over)
				time  = 0;
		}
		else
			time = (client_tick()-gameclient.snap.gameobj->round_start_tick)/client_tickspeed();

		str_format(buf, sizeof(buf), "%d:%02d", time /60, time %60);
		float w = gfx_text_width(0, 16, buf, -1);
		gfx_text(0, half-w/2, 2, 16, buf, -1);
	}

	if(gameclient.snap.gameobj->sudden_death)
	{
		const char *text = "Sudden Death";
		float w = gfx_text_width(0, 16, text, -1);
		gfx_text(0, half-w/2, 2, 16, text, -1);
	}

	// render small score hud
	if(!(gameclient.snap.gameobj && gameclient.snap.gameobj->game_over) && (gameflags&GAMEFLAG_TEAMS))
	{
		for(int t = 0; t < 2; t++)
		{
			gfx_blend_normal();
			gfx_texture_set(-1);
			gfx_quads_begin();
			if(t == 0)
				gfx_setcolor(1,0,0,0.25f);
			else
				gfx_setcolor(0,0,1,0.25f);
			draw_round_rect(whole-40, 300-40-15+t*20, 50, 18, 5.0f);
			gfx_quads_end();

			char buf[32];
			str_format(buf, sizeof(buf), "%d", t?gameclient.snap.gameobj->teamscore_blue:gameclient.snap.gameobj->teamscore_red);
			float w = gfx_text_width(0, 14, buf, -1);
			
			if(gameflags&GAMEFLAG_FLAGS)
			{
				gfx_text(0, whole-20-w/2+5, 300-40-15+t*20, 14, buf, -1);
				if(gameclient.snap.flags[t])
				{
					if(gameclient.snap.flags[t]->carried_by == -2 || (gameclient.snap.flags[t]->carried_by == -1 && ((client_tick()/10)&1)))
					{
						gfx_blend_normal();
						gfx_texture_set(data->images[IMAGE_GAME].id);
						gfx_quads_begin();

						if(t == 0) select_sprite(SPRITE_FLAG_RED);
						else select_sprite(SPRITE_FLAG_BLUE);
						
						float size = 16;					
						gfx_quads_drawTL(whole-40+5, 300-40-15+t*20+1, size/2, size);
						gfx_quads_end();
					}
					else if(gameclient.snap.flags[t]->carried_by >= 0)
					{
						int id = gameclient.snap.flags[t]->carried_by%MAX_CLIENTS;
						const char *name = gameclient.clients[id].name;
						float w = gfx_text_width(0, 10, name, -1);
						gfx_text(0, whole-40-5-w, 300-40-15+t*20+2, 10, name, -1);
						TEE_RENDER_INFO info = gameclient.clients[id].render_info;
						info.size = 18.0f;
						
						render_tee(ANIMSTATE::get_idle(), &info, EMOTE_NORMAL, vec2(1,0),
							vec2(whole-40+10, 300-40-15+9+t*20+1));
					}
				}
			}
			else
				gfx_text(0, whole-20-w/2, 300-40-15+t*20, 14, buf, -1);
		}
	}

	// render warmup timer
	if(gameclient.snap.gameobj->warmup)
	{
		char buf[256];
		float w = gfx_text_width(0, 24, "Warmup", -1);
		gfx_text(0, 150*gfx_screenaspect()+-w/2, 50, 24, "Warmup", -1);

		int seconds = gameclient.snap.gameobj->warmup/SERVER_TICK_SPEED;
		if(seconds < 5)
			str_format(buf, sizeof(buf), "%d.%d", seconds, (gameclient.snap.gameobj->warmup*10/SERVER_TICK_SPEED)%10);
		else
			str_format(buf, sizeof(buf), "%d", seconds);
		w = gfx_text_width(0, 24, buf, -1);
		gfx_text(0, 150*gfx_screenaspect()+-w/2, 75, 24, buf, -1);
	}	
}

static void mapscreen_to_group(float center_x, float center_y, MAPITEM_GROUP *group)
{
	float points[4];
	mapscreen_to_world(center_x, center_y, group->parallax_x/100.0f, group->parallax_y/100.0f,
		group->offset_x, group->offset_y, gfx_screenaspect(), 1.0f, points);
	gfx_mapscreen(points[0], points[1], points[2], points[3]);
}

void HUD::render_fps()
{
	if(config.cl_showfps)
	{
		char buf[512];
		str_format(buf, sizeof(buf), "%d", (int)(1.0f/client_frametime()));
		gfx_text(0, width-10-gfx_text_width(0,12,buf,-1), 10, 12, buf, -1);
	}
}

void HUD::render_connectionwarning()
{
	if(client_connection_problems())
	{
		const char *text = "Connection Problems...";
		float w = gfx_text_width(0, 24, text, -1);
		gfx_text(0, 150*gfx_screenaspect()-w/2, 50, 24, text, -1);
	}
}

void HUD::render_tunewarning()
{
	TUNING_PARAMS standard_tuning;

	// render warning about non standard tuning
	bool flash = time_get()/(time_freq()/2)%2 == 0;
	if(config.cl_warning_tuning && memcmp(&standard_tuning, &gameclient.tuning, sizeof(TUNING_PARAMS)) != 0)
	{
		const char *text = "Warning! Server is running non-standard tuning.";
		if(flash)
			gfx_text_color(1,0.4f,0.4f,1.0f);
		else
			gfx_text_color(0.75f,0.2f,0.2f,1.0f);
		gfx_text(0x0, 5, 40, 6, text, -1);
		gfx_text_color(1,1,1,1);
	}
}		

void HUD::render_cursor()
{
	if(!gameclient.snap.local_character)
		return;
		
	mapscreen_to_group(gameclient.camera->center.x, gameclient.camera->center.y, layers_game_group());
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();

	// render cursor
	select_sprite(data->weapons.id[gameclient.snap.local_character->weapon%NUM_WEAPONS].sprite_cursor);
	float cursorsize = 64;
	draw_sprite(gameclient.controls->target_pos.x, gameclient.controls->target_pos.y, cursorsize);
	gfx_quads_end();
}

void HUD::render_healthandammo()
{
	//mapscreen_to_group(gacenter_x, center_y, layers_game_group());

	float x = 5;
	float y = 5;

	// render ammo count
	// render gui stuff

	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_mapscreen(0,0,width,300);
	
	gfx_quads_begin();
	
	// if weaponstage is active, put a "glow" around the stage ammo
	select_sprite(data->weapons.id[gameclient.snap.local_character->weapon%NUM_WEAPONS].sprite_proj);
	for (int i = 0; i < min(gameclient.snap.local_character->ammocount, 10); i++)
		gfx_quads_drawTL(x+i*12,y+24,10,10);

	gfx_quads_end();

	gfx_quads_begin();
	int h = 0;

	// render health
	select_sprite(SPRITE_HEALTH_FULL);
	for(; h < gameclient.snap.local_character->health; h++)
		gfx_quads_drawTL(x+h*12,y,10,10);

	select_sprite(SPRITE_HEALTH_EMPTY);
	for(; h < 10; h++)
		gfx_quads_drawTL(x+h*12,y,10,10);

	// render armor meter
	h = 0;
	select_sprite(SPRITE_ARMOR_FULL);
	for(; h < gameclient.snap.local_character->armor; h++)
		gfx_quads_drawTL(x+h*12,y+12,10,10);

	select_sprite(SPRITE_ARMOR_EMPTY);
	for(; h < 10; h++)
		gfx_quads_drawTL(x+h*12,y+12,10,10);
	gfx_quads_end();
}

void HUD::on_render()
{
	if(!gameclient.snap.gameobj)
		return;
		
	width = 300*gfx_screenaspect();

	bool spectate = false;
	if(gameclient.snap.local_info && gameclient.snap.local_info->team == -1)
		spectate = true;
	
	if(gameclient.snap.local_character && !spectate && !(gameclient.snap.gameobj && gameclient.snap.gameobj->game_over))
		render_healthandammo();

	render_goals();
	render_fps();
	render_connectionwarning();
	render_tunewarning();
	render_cursor();
}
