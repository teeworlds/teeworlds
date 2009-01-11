#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>
#include "nameplates.hpp"
#include "controls.hpp"

void NAMEPLATES::render_nameplate(
	const NETOBJ_CHARACTER *prev_char,
	const NETOBJ_CHARACTER *player_char,
	const NETOBJ_PLAYER_INFO *player_info
	)
{
	float intratick = client_intratick();
	
	vec2 position = mix(vec2(prev_char->x, prev_char->y), vec2(player_char->x, player_char->y), intratick);
	
	// render name plate
	if(!player_info->local)
	{
		//gfx_text_color
		float a = 1;
		if(config.cl_nameplates_always == 0)
			a = clamp(1-powf(distance(gameclient.controls->target_pos, position)/200.0f,16.0f), 0.0f, 1.0f);
			
		const char *name = gameclient.clients[player_info->cid].name;
		float tw = gfx_text_width(0, 28.0f, name, -1);
		gfx_text_color(1,1,1,a);
		gfx_text(0, position.x-tw/2.0f, position.y-60, 28.0f, name, -1);
		
		if(config.debug) // render client id when in debug aswell
		{
			char buf[128];
			str_format(buf, sizeof(buf),"%d", player_info->cid);
			gfx_text(0, position.x, position.y-90, 28.0f, buf, -1);
		}

		gfx_text_color(1,1,1,1);
	}
}

void NAMEPLATES::on_render()
{
	if (!config.cl_nameplates)
		return;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// only render active characters
		if(!gameclient.snap.characters[i].active)
			continue;

		const void *info = snap_find_item(SNAP_CURRENT, NETOBJTYPE_PLAYER_INFO, i);

		if(info)
		{
			render_nameplate(
				&gameclient.snap.characters[i].prev,
				&gameclient.snap.characters[i].cur,
				(const NETOBJ_PLAYER_INFO *)info);
		}
	}
}
