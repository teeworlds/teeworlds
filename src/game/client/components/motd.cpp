#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/gameclient.hpp>
#include <game/client/gc_client.hpp>

#include "motd.hpp"
	
void MOTD::on_reset()
{
	clear();
}

void MOTD::clear()
{
	server_motd_time = 0;
}

bool MOTD::is_active()
{
	return time_get() < server_motd_time;	
}

void MOTD::on_render()
{
	if(!is_active())
		return;
		
	float width = 400*3.0f*gfx_screenaspect();
	float height = 400*3.0f;

	gfx_mapscreen(0, 0, width, height);
	
	float h = 800.0f;
	float w = 650.0f;
	float x = width/2 - w/2;
	float y = 150.0f;

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x, y, w, h, 40.0f);
	gfx_quads_end();

	gfx_text(0, x+40.0f, y+40.0f, 32.0f, server_motd, (int)(w-80.0f));
}

void MOTD::on_message(int msgtype, void *rawmsg)
{
	if(msgtype == NETMSGTYPE_SV_MOTD)
	{
		NETMSG_SV_MOTD *msg = (NETMSG_SV_MOTD *)rawmsg;

		// process escaping			
		str_copy(server_motd, msg->message, sizeof(server_motd));
		for(int i = 0; server_motd[i]; i++)
		{
			if(server_motd[i] == '\\')
			{
				if(server_motd[i+1] == 'n')
				{
					server_motd[i] = ' ';
					server_motd[i+1] = '\n';
					i++;
				}
			}
		}

		if(server_motd[0] && config.cl_motd_time)
			server_motd_time = time_get()+time_freq()*config.cl_motd_time;
		else
			server_motd_time = 0;
	}
}

bool MOTD::on_input(INPUT_EVENT e)
{
	if(is_active() && e.flags&INPFLAG_PRESS && e.key == KEY_ESC)
	{
		clear();
		return true;
	}
	return false;
}

