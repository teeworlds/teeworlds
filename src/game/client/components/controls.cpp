#include <engine/e_client_interface.h>
#include <base/math.hpp>
#include <game/collision.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/component.hpp>
#include <game/client/components/chat.hpp>
#include <game/client/components/menus.hpp>

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

struct INPUTSET
{
	CONTROLS *controls;
	int *variable;
	int value;
};

static void con_key_input_set(void *result, void *user_data)
{
	INPUTSET *set = (INPUTSET *)user_data;
	if(console_arg_int(result, 0))
		*set->variable = set->value;
}

static void con_key_input_nextprev_weapon(void *result, void *user_data)
{
	INPUTSET *set = (INPUTSET *)user_data;
	con_key_input_counter(result, set->variable);
	set->controls->input_data.wanted_weapon = 0;
}

void CONTROLS::on_console_init()
{
	// game commands
	MACRO_REGISTER_COMMAND("+left", "", CFGFLAG_CLIENT, con_key_input_state, &input_direction_left, "");
	MACRO_REGISTER_COMMAND("+right", "", CFGFLAG_CLIENT, con_key_input_state, &input_direction_right, "");
	MACRO_REGISTER_COMMAND("+jump", "", CFGFLAG_CLIENT, con_key_input_state, &input_data.jump, "");
	MACRO_REGISTER_COMMAND("+hook", "", CFGFLAG_CLIENT, con_key_input_state, &input_data.hook, "");
	MACRO_REGISTER_COMMAND("+fire", "", CFGFLAG_CLIENT, con_key_input_counter, &input_data.fire, "");

	{ static INPUTSET set = {this, &input_data.wanted_weapon, 1};  MACRO_REGISTER_COMMAND("+weapon1", "", CFGFLAG_CLIENT, con_key_input_set, (void *)&set, ""); }
	{ static INPUTSET set = {this, &input_data.wanted_weapon, 2};  MACRO_REGISTER_COMMAND("+weapon2", "", CFGFLAG_CLIENT, con_key_input_set, (void *)&set, ""); }
	{ static INPUTSET set = {this, &input_data.wanted_weapon, 3};  MACRO_REGISTER_COMMAND("+weapon3", "", CFGFLAG_CLIENT, con_key_input_set, (void *)&set, ""); }
	{ static INPUTSET set = {this, &input_data.wanted_weapon, 4};  MACRO_REGISTER_COMMAND("+weapon4", "", CFGFLAG_CLIENT, con_key_input_set, (void *)&set, ""); }
	{ static INPUTSET set = {this, &input_data.wanted_weapon, 5};  MACRO_REGISTER_COMMAND("+weapon5", "", CFGFLAG_CLIENT, con_key_input_set, (void *)&set, ""); }

	{ static INPUTSET set = {this, &input_data.next_weapon, 0};  MACRO_REGISTER_COMMAND("+nextweapon", "", CFGFLAG_CLIENT, con_key_input_nextprev_weapon, (void *)&set, ""); }
	{ static INPUTSET set = {this, &input_data.prev_weapon, 0};  MACRO_REGISTER_COMMAND("+prevweapon", "", CFGFLAG_CLIENT, con_key_input_nextprev_weapon, (void *)&set, ""); }
}

void CONTROLS::on_message(int msg, void *rawmsg)
{
    if(msg == NETMSGTYPE_SV_WEAPONPICKUP)
    {
    	NETMSG_SV_WEAPONPICKUP *msg = (NETMSG_SV_WEAPONPICKUP *)rawmsg;
        if(config.cl_autoswitch_weapons)
        	input_data.wanted_weapon = msg->weapon+1;
    }
}

int CONTROLS::snapinput(int *data)
{
	static NETOBJ_PLAYER_INPUT last_data = {0};
	static int64 last_send_time = 0;
	bool send = false;
	
	// update player state
	if(gameclient.chat->is_active())
		input_data.player_state = PLAYERSTATE_CHATTING;
	else if(gameclient.menus->is_active())
		input_data.player_state = PLAYERSTATE_IN_MENU;
	else
		input_data.player_state = PLAYERSTATE_PLAYING;
	
	if(last_data.player_state != input_data.player_state)
		send = true;
		
	last_data.player_state = input_data.player_state;
	
	// we freeze the input if chat or menu is activated
	if(input_data.player_state != PLAYERSTATE_PLAYING)
	{
		last_data.direction = 0;
		last_data.hook = 0;
		last_data.jump = 0;
		input_data = last_data;
		
		input_direction_left = 0;
		input_direction_right = 0;
			
		mem_copy(data, &input_data, sizeof(input_data));

		// send once a second just to be sure
		if(time_get() > last_send_time + time_freq())
			send = true;
	}
	else
	{
		
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
		if(input_data.direction != last_data.direction) send = true;
		else if(input_data.jump != last_data.jump) send = true;
		else if(input_data.fire != last_data.fire) send = true;
		else if(input_data.hook != last_data.hook) send = true;
		else if(input_data.player_state != last_data.player_state) send = true;
		else if(input_data.wanted_weapon != last_data.wanted_weapon) send = true;
		else if(input_data.next_weapon != last_data.next_weapon) send = true;
		else if(input_data.prev_weapon != last_data.prev_weapon) send = true;

		// send at at least 10hz
		if(time_get() > last_send_time + time_freq()/25)
			send = true;
	}
	
	// copy and return size	
	last_data = input_data;
	
	if(!send)
		return 0;
		
	last_send_time = time_get();
	mem_copy(data, &input_data, sizeof(input_data));
	return sizeof(input_data);	
}

void CONTROLS::on_render()
{
	// update target pos
	if(!(gameclient.snap.gameobj && gameclient.snap.gameobj->paused || gameclient.snap.spectate))
		target_pos = gameclient.local_character_pos + mouse_pos;
}

bool CONTROLS::on_mousemove(float x, float y)
{
	if(gameclient.snap.gameobj && gameclient.snap.gameobj->paused)
		return false;
	mouse_pos += vec2(x, y); // TODO: ugly

	//
	float camera_max_distance = 200.0f;
	float follow_factor = config.cl_mouse_followfactor/100.0f;
	float deadzone = config.cl_mouse_deadzone;
	float mouse_max = min(camera_max_distance/follow_factor + deadzone, (float)config.cl_mouse_max_distance);
	
	//vec2 camera_offset(0, 0);

	if(gameclient.snap.spectate)
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
		
		//float offset_amount = max(l-deadzone, 0.0f) * follow_factor;
		//if(l > 0.0001f) // make sure that this isn't 0
			//camera_offset = normalize(mouse_pos)*offset_amount;
	}
	
	return true;
}
