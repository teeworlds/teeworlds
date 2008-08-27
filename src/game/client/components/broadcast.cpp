#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/gameclient.hpp>
//#include <game/client/gc_anim.hpp>
#include <game/client/gc_client.hpp>

#include "broadcast.hpp"
	
void BROADCAST::on_reset()
{
	broadcast_time = 0;
}

void BROADCAST::on_render()
{
	gfx_mapscreen(0, 0, 300*gfx_screenaspect(), 300);
		
	if(time_get() < broadcast_time)
	{
		float w = gfx_text_width(0, 14, broadcast_text, -1);
		gfx_text(0, 150*gfx_screenaspect()-w/2, 35, 14, broadcast_text, -1);
	}
}

void BROADCAST::on_message(int msgtype, void *rawmsg)
{
	if(msgtype == NETMSGTYPE_SV_BROADCAST)
	{
		NETMSG_SV_BROADCAST *msg = (NETMSG_SV_BROADCAST *)rawmsg;
		str_copy(broadcast_text, msg->message, sizeof(broadcast_text));
		broadcast_time = time_get()+time_freq()*10;
	}
}

