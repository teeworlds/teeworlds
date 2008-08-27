#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>
#include <game/client/gc_client.hpp>

#include "killmessages.hpp"

void KILLMESSAGES::on_reset()
{
	killmsg_current = 0;
	for(int i = 0; i < killmsg_max; i++)
		killmsgs[i].tick = -100000;
}

void KILLMESSAGES::on_message(int msgtype, void *rawmsg)
{
	if(msgtype == NETMSGTYPE_SV_KILLMSG)
	{
		NETMSG_SV_KILLMSG *msg = (NETMSG_SV_KILLMSG *)rawmsg;
		
		// unpack messages
		KILLMSG kill;
		kill.killer = msg->killer;
		kill.victim = msg->victim;
		kill.weapon = msg->weapon;
		kill.mode_special = msg->mode_special;
		kill.tick = client_tick();

		// add the message
		killmsg_current = (killmsg_current+1)%killmsg_max;
		killmsgs[killmsg_current] = kill;		
	}
}

void KILLMESSAGES::on_render()
{
	float width = 400*3.0f*gfx_screenaspect();
	float height = 400*3.0f;

	gfx_mapscreen(0, 0, width*1.5f, height*1.5f);
	float startx = width*1.5f-10.0f;
	float y = 20.0f;

	for(int i = 0; i < killmsg_max; i++)
	{

		int r = (killmsg_current+i+1)%killmsg_max;
		if(client_tick() > killmsgs[r].tick+50*10)
			continue;

		float font_size = 36.0f;
		float killername_w = gfx_text_width(0, font_size, gameclient.clients[killmsgs[r].killer].name, -1);
		float victimname_w = gfx_text_width(0, font_size, gameclient.clients[killmsgs[r].victim].name, -1);

		float x = startx;

		// render victim name
		x -= victimname_w;
		gfx_text(0, x, y, font_size, gameclient.clients[killmsgs[r].victim].name, -1);

		// render victim tee
		x -= 24.0f;
		
		if(gameclient.snap.gameobj && gameclient.snap.gameobj->gametype == GAMETYPE_CTF)
		{
			if(killmsgs[r].mode_special&1)
			{
				gfx_blend_normal();
				gfx_texture_set(data->images[IMAGE_GAME].id);
				gfx_quads_begin();

				if(gameclient.clients[killmsgs[r].victim].team == 0) select_sprite(SPRITE_FLAG_BLUE);
				else select_sprite(SPRITE_FLAG_RED);
				
				float size = 56.0f;
				gfx_quads_drawTL(x, y-16, size/2, size);
				gfx_quads_end();					
			}
		}
		
		render_tee(ANIMSTATE::get_idle(), &gameclient.clients[killmsgs[r].victim].render_info, EMOTE_PAIN, vec2(-1,0), vec2(x, y+28));
		x -= 32.0f;
		
		// render weapon
		x -= 44.0f;
		if (killmsgs[r].weapon >= 0)
		{
			gfx_texture_set(data->images[IMAGE_GAME].id);
			gfx_quads_begin();
			select_sprite(data->weapons.id[killmsgs[r].weapon].sprite_body);
			draw_sprite(x, y+28, 96);
			gfx_quads_end();
		}
		x -= 52.0f;

		if(killmsgs[r].victim != killmsgs[r].killer)
		{
			if(gameclient.snap.gameobj && gameclient.snap.gameobj->gametype == GAMETYPE_CTF)
			{
				if(killmsgs[r].mode_special&2)
				{
					gfx_blend_normal();
					gfx_texture_set(data->images[IMAGE_GAME].id);
					gfx_quads_begin();

					if(gameclient.clients[killmsgs[r].killer].team == 0) select_sprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
					else select_sprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
					
					float size = 56.0f;
					gfx_quads_drawTL(x-56, y-16, size/2, size);
					gfx_quads_end();				
				}
			}				
			
			// render killer tee
			x -= 24.0f;
			render_tee(ANIMSTATE::get_idle(), &gameclient.clients[killmsgs[r].killer].render_info, EMOTE_ANGRY, vec2(1,0), vec2(x, y+28));
			x -= 32.0f;

			// render killer name
			x -= killername_w;
			gfx_text(0, x, y, font_size, gameclient.clients[killmsgs[r].killer].name, -1);
		}

		y += 44;
	}
}
