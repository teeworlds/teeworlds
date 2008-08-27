#include <engine/e_client_interface.h>
#include <base/math.hpp>
#include <game/collision.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/component.hpp>

#include "controls.hpp"

CONTROLS::CONTROLS()
{
}

static void con_key_input_state(void *result, void *user_data)
{
	((int *)user_data)[0] = console_arg_int(result, 0);
}

static void con_key_input_counter(void *result, void *user_data)
{
	int *v = (int *)user_data;
	if(((*v)&1) != console_arg_int(result, 0))
		(*v)++;
	*v &= INPUT_STATE_MASK;
}
/*
static void con_key_input_weapon(void *result, void *user_data)
{
	int w = (char *)user_data - (char *)0;
	if(console_arg_int(result, 0))
		input_data.wanted_weapon = w;
}

static void con_key_input_nextprev_weapon(void *result, void *user_data)
{
	con_key_input_counter(result, user_data);
	input_data.wanted_weapon = 0;
}*/

void CONTROLS::on_init()
{
	// game commands
	MACRO_REGISTER_COMMAND("+left", "", con_key_input_state, &input_direction_left);
	MACRO_REGISTER_COMMAND("+right", "", con_key_input_state, &input_direction_right);
	MACRO_REGISTER_COMMAND("+jump", "", con_key_input_state, &input_data.jump);
	MACRO_REGISTER_COMMAND("+hook", "", con_key_input_state, &input_data.hook);
	MACRO_REGISTER_COMMAND("+fire", "", con_key_input_counter, &input_data.fire);
	/*
	MACRO_REGISTER_COMMAND("+weapon1", "", con_key_input_weapon, (void *)1);
	MACRO_REGISTER_COMMAND("+weapon2", "", con_key_input_weapon, (void *)2);
	MACRO_REGISTER_COMMAND("+weapon3", "", con_key_input_weapon, (void *)3);
	MACRO_REGISTER_COMMAND("+weapon4", "", con_key_input_weapon, (void *)4);
	MACRO_REGISTER_COMMAND("+weapon5", "", con_key_input_weapon, (void *)5);

	MACRO_REGISTER_COMMAND("+nextweapon", "", con_key_input_nextprev_weapon, &input_data.next_weapon);
	MACRO_REGISTER_COMMAND("+prevweapon", "", con_key_input_nextprev_weapon, &input_data.prev_weapon);
	*/
}

int CONTROLS::snapinput(int *data)
{
	static NETOBJ_PLAYER_INPUT last_data = {0};
	static int64 last_send_time = 0;
	
	// update player state
	/*if(chat_mode != CHATMODE_NONE) // TODO: repair me
		input_data.player_state = PLAYERSTATE_CHATTING;
	else if(menu_active)
		input_data.player_state = PLAYERSTATE_IN_MENU;
	else
		input_data.player_state = PLAYERSTATE_PLAYING;*/
	last_data.player_state = input_data.player_state;
	
	// we freeze the input if chat or menu is activated
	/* repair me
	if(menu_active || chat_mode != CHATMODE_NONE || console_active())
	{
		last_data.direction = 0;
		last_data.hook = 0;
		last_data.jump = 0;
		
		input_data = last_data;
			
		mem_copy(data, &input_data, sizeof(input_data));
		return sizeof(input_data);
	}*/
	
	input_data.target_x = (int)mouse_pos.x;
	input_data.target_y = (int)mouse_pos.y;
	if(!input_data.target_x && !input_data.target_y)
		input_data.target_y = 1;
		
	// set direction
	input_data.direction = 0;
	if(input_direction_left && !input_direction_right)
		input_data.direction = -1;
	if(!input_direction_left && input_direction_right)
		input_data.direction = 1;

	// stress testing
	if(config.dbg_stress)
	{
		float t = client_localtime();
		mem_zero(&input_data, sizeof(input_data));

		input_data.direction = ((int)t/2)&1;
		input_data.jump = ((int)t);
		input_data.fire = ((int)(t*10));
		input_data.hook = ((int)(t*2))&1;
		input_data.wanted_weapon = ((int)t)%NUM_WEAPONS;
		input_data.target_x = (int)(sinf(t*3)*100.0f);
		input_data.target_y = (int)(cosf(t*3)*100.0f);
	}

	// check if we need to send input
	bool send = false;
	if(input_data.direction != last_data.direction) send = true;
	else if(input_data.jump != last_data.jump) send = true;
	else if(input_data.fire != last_data.fire) send = true;
	else if(input_data.hook != last_data.hook) send = true;
	else if(input_data.player_state != last_data.player_state) send = true;
	else if(input_data.wanted_weapon != last_data.wanted_weapon) send = true;
	else if(input_data.next_weapon != last_data.next_weapon) send = true;
	else if(input_data.prev_weapon != last_data.prev_weapon) send = true;

	if(time_get() > last_send_time + time_freq()/5)
		send = true;

	last_data = input_data;
	if(!send)
		return 0;
		
	// copy and return size	
	last_send_time = time_get();
	mem_copy(data, &input_data, sizeof(input_data));
	return sizeof(input_data);	
}

bool CONTROLS::on_mousemove(float x, float y)
{
	mouse_pos += vec2(x, y); // TODO: ugly

	bool spectate = false;

	//
	float camera_max_distance = 200.0f;
	float follow_factor = config.cl_mouse_followfactor/100.0f;
	float deadzone = config.cl_mouse_deadzone;
	float mouse_max = min(camera_max_distance/follow_factor + deadzone, (float)config.cl_mouse_max_distance);
	
	//vec2 camera_offset(0, 0);

	if(spectate)
	{
		if(mouse_pos.x < 200.0f) mouse_pos.x = 200.0f;
		if(mouse_pos.y < 200.0f) mouse_pos.y = 200.0f;
		if(mouse_pos.x > col_width()*32-200.0f) mouse_pos.x = col_width()*32-200.0f;
		if(mouse_pos.y > col_height()*32-200.0f) mouse_pos.y = col_height()*32-200.0f;
		
		target_pos = mouse_pos;
	}
	else
	{
		float l = length(mouse_pos);
		
		if(l > mouse_max)
		{
			mouse_pos = normalize(mouse_pos)*mouse_max;
			l = mouse_max;
		}
		
		target_pos = gameclient.local_character_pos + mouse_pos;

		//float offset_amount = max(l-deadzone, 0.0f) * follow_factor;
		//if(l > 0.0001f) // make sure that this isn't 0
			//camera_offset = normalize(mouse_pos)*offset_amount;
	}
	
	return true;
}
