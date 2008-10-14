extern "C" {
	#include <engine/e_config.h>
	#include <engine/e_client_interface.h>
}

#include <base/math.hpp>
#include <game/collision.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/component.hpp>

#include "camera.hpp"
#include "controls.hpp"

CAMERA::CAMERA()
{
}

void CAMERA::on_render()
{
	//vec2 center;
	zoom = 1.0f;

	// update camera center		
	if(gameclient.snap.spectate)
		center = gameclient.controls->mouse_pos;
	else
	{

		float l = length(gameclient.controls->mouse_pos);
		float deadzone = config.cl_mouse_deadzone;
		float follow_factor = config.cl_mouse_followfactor/100.0f;
		vec2 camera_offset(0, 0);

		float offset_amount = max(l-deadzone, 0.0f) * follow_factor;
		if(l > 0.0001f) // make sure that this isn't 0
			camera_offset = normalize(gameclient.controls->mouse_pos)*offset_amount;
		
		center = gameclient.local_character_pos + camera_offset;
	}
}
