#include <memory.h> // memcmp

extern "C" {
	#include <engine/e_config.h>
}

#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/layers.hpp>

#include <game/client/gameclient.hpp>
#include <game/client/animstate.hpp>
#include <game/client/render.hpp>

//#include "controls.hpp"
//#include "camera.hpp"
#include "debughud.hpp"

void DEBUGHUD::render_netcorrections()
{
	if(!config.debug || !gameclient.snap.local_character || !gameclient.snap.local_prev_character)
		return;

	gfx_mapscreen(0, 0, 300*gfx_screenaspect(), 300);
	
	/*float speed = distance(vec2(netobjects.local_prev_character->x, netobjects.local_prev_character->y),
		vec2(netobjects.local_character->x, netobjects.local_character->y));*/

	float velspeed = length(vec2(gameclient.snap.local_character->vx/256.0f, gameclient.snap.local_character->vy/256.0f))*50;
	
	float ramp = velocity_ramp(velspeed, gameclient.tuning.velramp_start, gameclient.tuning.velramp_range, gameclient.tuning.velramp_curvature);
	
	char buf[512];
	str_format(buf, sizeof(buf), "%.0f\n%.0f\n%.2f\n%d %s\n%d %d",
		velspeed, velspeed*ramp, ramp,
		netobj_num_corrections(), netobj_corrected_on(),
		gameclient.snap.local_character->x,
		gameclient.snap.local_character->y
	);
	gfx_text(0, 150, 50, 12, buf, -1);
}

void DEBUGHUD::render_tuning()
{
	// render tuning debugging
	if(!config.dbg_tuning)
		return;
		
	TUNING_PARAMS standard_tuning;
		
	gfx_mapscreen(0, 0, 300*gfx_screenaspect(), 300);
	
	float y = 50.0f;
	int count = 0;
	for(int i = 0; i < gameclient.tuning.num(); i++)
	{
		char buf[128];
		float current, standard;
		gameclient.tuning.get(i, &current);
		standard_tuning.get(i, &standard);
		
		if(standard == current)
			gfx_text_color(1,1,1,1.0f);
		else
			gfx_text_color(1,0.25f,0.25f,1.0f);

		float w;
		float x = 5.0f;
		
		str_format(buf, sizeof(buf), "%.2f", standard);
		x += 20.0f;
		w = gfx_text_width(0, 5, buf, -1);
		gfx_text(0x0, x-w, y+count*6, 5, buf, -1);

		str_format(buf, sizeof(buf), "%.2f", current);
		x += 20.0f;
		w = gfx_text_width(0, 5, buf, -1);
		gfx_text(0x0, x-w, y+count*6, 5, buf, -1);

		x += 5.0f;
		gfx_text(0x0, x, y+count*6, 5, gameclient.tuning.names[i], -1);
		
		count++;
	}
	
	y = y+count*6;
	
	gfx_texture_set(-1);
	gfx_blend_normal();
	gfx_lines_begin();
	float height = 50.0f;
	float pv = 1;
	for(int i = 0; i < 100; i++)
	{
		float speed = i/100.0f * 3000;
		float ramp = velocity_ramp(speed, gameclient.tuning.velramp_start, gameclient.tuning.velramp_range, gameclient.tuning.velramp_curvature);
		float rampedspeed = (speed * ramp)/1000.0f;
		gfx_lines_draw((i-1)*2, y+height-pv*height, i*2, y+height-rampedspeed*height);
		//gfx_lines_draw((i-1)*2, 200, i*2, 200);
		pv = rampedspeed;
	}
	gfx_lines_end();
	gfx_text_color(1,1,1,1);
}

void DEBUGHUD::on_render()
{
	render_tuning();
	render_netcorrections();
}
