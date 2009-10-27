#include <memory.h> // memcmp

#include <engine/e_client_interface.h>
#include <engine/client/graphics.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/layers.hpp>

#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>
#include <game/client/render.hpp>

#include "controls.hpp"
#include "camera.hpp"
#include "hud.hpp"
#include "voting.hpp"
#include "binds.hpp"

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
	
	float whole = 300*Graphics()->ScreenAspect();
	float half = whole/2.0f;


	Graphics()->MapScreen(0,0,300*Graphics()->ScreenAspect(),300);
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
			Graphics()->BlendNormal();
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			if(t == 0)
				Graphics()->SetColor(1,0,0,0.25f);
			else
				Graphics()->SetColor(0,0,1,0.25f);
			RenderTools()->draw_round_rect(whole-40, 300-40-15+t*20, 50, 18, 5.0f);
			Graphics()->QuadsEnd();

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
						Graphics()->BlendNormal();
						Graphics()->TextureSet(data->images[IMAGE_GAME].id);
						Graphics()->QuadsBegin();

						if(t == 0) RenderTools()->select_sprite(SPRITE_FLAG_RED);
						else RenderTools()->select_sprite(SPRITE_FLAG_BLUE);
						
						float size = 16;					
						Graphics()->QuadsDrawTL(whole-40+5, 300-40-15+t*20+1, size/2, size);
						Graphics()->QuadsEnd();
					}
					else if(gameclient.snap.flags[t]->carried_by >= 0)
					{
						int id = gameclient.snap.flags[t]->carried_by%MAX_CLIENTS;
						const char *name = gameclient.clients[id].name;
						float w = gfx_text_width(0, 10, name, -1);
						gfx_text(0, whole-40-5-w, 300-40-15+t*20+2, 10, name, -1);
						TEE_RENDER_INFO info = gameclient.clients[id].render_info;
						info.size = 18.0f;
						
						RenderTools()->RenderTee(ANIMSTATE::get_idle(), &info, EMOTE_NORMAL, vec2(1,0),
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
		gfx_text(0, 150*Graphics()->ScreenAspect()+-w/2, 50, 24, "Warmup", -1);

		int seconds = gameclient.snap.gameobj->warmup/SERVER_TICK_SPEED;
		if(seconds < 5)
			str_format(buf, sizeof(buf), "%d.%d", seconds, (gameclient.snap.gameobj->warmup*10/SERVER_TICK_SPEED)%10);
		else
			str_format(buf, sizeof(buf), "%d", seconds);
		w = gfx_text_width(0, 24, buf, -1);
		gfx_text(0, 150*Graphics()->ScreenAspect()+-w/2, 75, 24, buf, -1);
	}	
}

void HUD::mapscreen_to_group(float center_x, float center_y, MAPITEM_GROUP *group)
{
	float points[4];
	RenderTools()->mapscreen_to_world(center_x, center_y, group->parallax_x/100.0f, group->parallax_y/100.0f,
		group->offset_x, group->offset_y, Graphics()->ScreenAspect(), 1.0f, points);
	Graphics()->MapScreen(points[0], points[1], points[2], points[3]);
}

void HUD::render_fps()
{
	if(config.cl_showfps)
	{
		char buf[512];
		str_format(buf, sizeof(buf), "%d", (int)(1.0f/client_frametime()));
		gfx_text(0, width-10-gfx_text_width(0,12,buf,-1), 5, 12, buf, -1);
	}
}

void HUD::render_connectionwarning()
{
	if(client_connection_problems())
	{
		const char *text = "Connection Problems...";
		float w = gfx_text_width(0, 24, text, -1);
		gfx_text(0, 150*Graphics()->ScreenAspect()-w/2, 50, 24, text, -1);
	}
}

void HUD::render_teambalancewarning()
{
	// render prompt about team-balance
	bool flash = time_get()/(time_freq()/2)%2 == 0;
	if (gameclient.snap.gameobj && (gameclient.snap.gameobj->flags&GAMEFLAG_TEAMS) != 0)
	{	
		if (config.cl_warning_teambalance && abs(gameclient.snap.team_size[0]-gameclient.snap.team_size[1]) >= 2)
		{
			const char *text = "Please balance teams!";
			if(flash)
				gfx_text_color(1,1,0.5f,1);
			else
				gfx_text_color(0.7f,0.7f,0.2f,1.0f);
			gfx_text(0x0, 5, 50, 6, text, -1);
			gfx_text_color(1,1,1,1);
		}
	}
}


void HUD::render_voting()
{
	if(!gameclient.voting->is_voting())
		return;
	
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.40f);
	RenderTools()->draw_round_rect(-10, 60-2, 100+10+4+5, 28, 5.0f);
	Graphics()->QuadsEnd();

	gfx_text_color(1,1,1,1);

	char buf[512];
	gfx_text(0x0, 5, 60, 6, gameclient.voting->vote_description(), -1);

	str_format(buf, sizeof(buf), "%ds left", gameclient.voting->seconds_left());
	float tw = gfx_text_width(0x0, 6, buf, -1);
	gfx_text(0x0, 5+100-tw, 60, 6, buf, -1);
	

	CUIRect base = {5, 70, 100, 4};
	gameclient.voting->render_bars(base, false);
	
	const char *yes_key = gameclient.binds->get_key("vote yes");
	const char *no_key = gameclient.binds->get_key("vote no");
	str_format(buf, sizeof(buf), "%s - Vote Yes", yes_key);
	base.y += base.h+1;
	UI()->DoLabel(&base, buf, 6.0f, -1);

	str_format(buf, sizeof(buf), "Vote No - %s", no_key);
	UI()->DoLabel(&base, buf, 6.0f, 1);
}

void HUD::render_cursor()
{
	if(!gameclient.snap.local_character)
		return;
		
	mapscreen_to_group(gameclient.camera->center.x, gameclient.camera->center.y, layers_game_group());
	Graphics()->TextureSet(data->images[IMAGE_GAME].id);
	Graphics()->QuadsBegin();

	// render cursor
	RenderTools()->select_sprite(data->weapons.id[gameclient.snap.local_character->weapon%NUM_WEAPONS].sprite_cursor);
	float cursorsize = 64;
	RenderTools()->draw_sprite(gameclient.controls->target_pos.x, gameclient.controls->target_pos.y, cursorsize);
	Graphics()->QuadsEnd();
}

void HUD::render_healthandammo()
{
	//mapscreen_to_group(gacenter_x, center_y, layers_game_group());

	float x = 5;
	float y = 5;

	// render ammo count
	// render gui stuff

	Graphics()->TextureSet(data->images[IMAGE_GAME].id);
	Graphics()->MapScreen(0,0,width,300);
	
	Graphics()->QuadsBegin();
	
	// if weaponstage is active, put a "glow" around the stage ammo
	RenderTools()->select_sprite(data->weapons.id[gameclient.snap.local_character->weapon%NUM_WEAPONS].sprite_proj);
	for (int i = 0; i < min(gameclient.snap.local_character->ammocount, 10); i++)
		Graphics()->QuadsDrawTL(x+i*12,y+24,10,10);

	Graphics()->QuadsEnd();

	Graphics()->QuadsBegin();
	int h = 0;

	// render health
	RenderTools()->select_sprite(SPRITE_HEALTH_FULL);
	for(; h < gameclient.snap.local_character->health; h++)
		Graphics()->QuadsDrawTL(x+h*12,y,10,10);

	RenderTools()->select_sprite(SPRITE_HEALTH_EMPTY);
	for(; h < 10; h++)
		Graphics()->QuadsDrawTL(x+h*12,y,10,10);

	// render armor meter
	h = 0;
	RenderTools()->select_sprite(SPRITE_ARMOR_FULL);
	for(; h < gameclient.snap.local_character->armor; h++)
		Graphics()->QuadsDrawTL(x+h*12,y+12,10,10);

	RenderTools()->select_sprite(SPRITE_ARMOR_EMPTY);
	for(; h < 10; h++)
		Graphics()->QuadsDrawTL(x+h*12,y+12,10,10);
	Graphics()->QuadsEnd();
}

void HUD::on_render()
{
	if(!gameclient.snap.gameobj)
		return;
		
	width = 300*Graphics()->ScreenAspect();

	bool spectate = false;
	if(gameclient.snap.local_info && gameclient.snap.local_info->team == -1)
		spectate = true;
	
	if(gameclient.snap.local_character && !spectate && !(gameclient.snap.gameobj && gameclient.snap.gameobj->game_over))
		render_healthandammo();

	render_goals();
	render_fps();
	if(client_state() != CLIENTSTATE_DEMOPLAYBACK)
		render_connectionwarning();
	render_teambalancewarning();
	render_voting();
	render_cursor();
}
