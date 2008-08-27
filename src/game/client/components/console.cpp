//#include "gc_console.hpp"
#include <game/generated/gc_data.hpp>

#include <base/system.h>

#include <engine/e_client_interface.h>

extern "C" {
	#include <engine/e_ringbuffer.h>
	#include <engine/client/ec_font.h>
}

#include <cstring>
#include <cstdio>

#include <game/client/gc_ui.hpp>
#include <game/client/gc_client.hpp>

#include <game/version.hpp>

#include <game/client/lineinput.hpp>

#include "console.hpp"

enum
{
	CONSOLE_CLOSED,
	CONSOLE_OPENING,
	CONSOLE_OPEN,
	CONSOLE_CLOSING,
};

CONSOLE::INSTANCE::INSTANCE(int t)
{
	// init ringbuffers
	history = ringbuf_init(history_data, sizeof(history_data));
	backlog = ringbuf_init(backlog_data, sizeof(backlog_data));
	
	history_entry = 0x0;
	
	type = t;
}

void CONSOLE::INSTANCE::execute_line(const char *line)
{
	if(type == 0)
		console_execute_line(line);
	else
	{
		if(client_rcon_authed())
			client_rcon(line);
		else
			client_rcon_auth("", line);
	}
}

void CONSOLE::INSTANCE::on_input(INPUT_EVENT e)
{
	bool handled = false;
	
	if(e.flags&INPFLAG_PRESS)
	{
		if(e.key == KEY_ENTER || e.key == KEY_KP_ENTER)
		{
			if(input.get_string()[0])
			{
				char *entry = (char *)ringbuf_allocate(history, input.get_length()+1);
				mem_copy(entry, input.get_string(), input.get_length()+1);
				
				execute_line(input.get_string());
				input.clear();
				history_entry = 0x0;
			}
			
			handled = true;
		}
		else if (e.key == KEY_UP)
		{
			if (history_entry)
			{
				char *test = (char *)ringbuf_prev(history, history_entry);

				if (test)
					history_entry = test;
			}
			else
				history_entry = (char *)ringbuf_last(history);

			if (history_entry)
			{
				unsigned int len = strlen(history_entry);
				if (len < sizeof(input) - 1)
					input.set(history_entry);
			}
			handled = true;
		}
		else if (e.key == KEY_DOWN)
		{
			if (history_entry)
				history_entry = (char *)ringbuf_next(history, history_entry);

			if (history_entry)
			{
				unsigned int len = strlen(history_entry);
				if (len < sizeof(input) - 1)
					input.set(history_entry);
			}
			else
				input.clear();
			handled = true;
		}
	}
	
	if(!handled)
		input.process_input(e);
}

void CONSOLE::INSTANCE::print_line(const char *line)
{
	int len = strlen(line);

	if (len > 255)
		len = 255;

	char *entry = (char *)ringbuf_allocate(backlog, len+1);
	mem_copy(entry, line, len+1);
}

CONSOLE::CONSOLE()
: local_console(0), remote_console(1)
{
	console_type = 0;
	console_state = CONSOLE_CLOSED;
	state_change_end = 0.0f;
	state_change_duration = 0.1f;
}

float CONSOLE::time_now()
{
	static long long time_start = time_get();
	return float(time_get()-time_start)/float(time_freq());
}

CONSOLE::INSTANCE *CONSOLE::current_console()
{
    if(console_type != 0)
    	return &remote_console;
    return &local_console;
}

void CONSOLE::on_reset()
{
}

// only defined for 0<=t<=1
static float console_scale_func(float t)
{
	//return t;
	return sinf(acosf(1.0f-t));
}

void CONSOLE::on_render()
{
    RECT screen = *ui_screen();
	float console_max_height = screen.h*3/5.0f;
	float console_height;

	float progress = (time_now()-(state_change_end-state_change_duration))/float(state_change_duration);

	if (progress >= 1.0f)
	{
		if (console_state == CONSOLE_CLOSING)
			console_state = CONSOLE_CLOSED;
		else if (console_state == CONSOLE_OPENING)
			console_state = CONSOLE_OPEN;

		progress = 1.0f;
	}
	
	if (console_state == CONSOLE_CLOSED)
		return;

	float console_height_scale;

	if (console_state == CONSOLE_OPENING)
		console_height_scale = console_scale_func(progress);
	else if (console_state == CONSOLE_CLOSING)
		console_height_scale = console_scale_func(1.0f-progress);
	else //if (console_state == CONSOLE_OPEN)
		console_height_scale = console_scale_func(1.0f);

	console_height = console_height_scale*console_max_height;

	gfx_mapscreen(screen.x, screen.y, screen.w, screen.h);

	// do console shadow
	gfx_texture_set(-1);
    gfx_quads_begin();
    gfx_setcolorvertex(0, 0,0,0, 0.5f);
    gfx_setcolorvertex(1, 0,0,0, 0.5f);
    gfx_setcolorvertex(2, 0,0,0, 0.0f);
    gfx_setcolorvertex(3, 0,0,0, 0.0f);
    gfx_quads_drawTL(0,console_height,screen.w,10.0f);
    gfx_quads_end();

	// do background
	gfx_texture_set(data->images[IMAGE_CONSOLE_BG].id);
    gfx_quads_begin();
    gfx_setcolor(0.2f, 0.2f, 0.2f,0.9f);
    if(console_type != 0)
	    gfx_setcolor(0.4f, 0.2f, 0.2f,0.9f);
    gfx_quads_setsubset(0,-console_height*0.075f,screen.w*0.075f*0.5f,0);
    gfx_quads_drawTL(0,0,screen.w,console_height);
    gfx_quads_end();

	// do small bar shadow
	gfx_texture_set(-1);
    gfx_quads_begin();
    gfx_setcolorvertex(0, 0,0,0, 0.0f);
    gfx_setcolorvertex(1, 0,0,0, 0.0f);
    gfx_setcolorvertex(2, 0,0,0, 0.25f);
    gfx_setcolorvertex(3, 0,0,0, 0.25f);
    gfx_quads_drawTL(0,console_height-20,screen.w,10);
    gfx_quads_end();

	// do the lower bar
	gfx_texture_set(data->images[IMAGE_CONSOLE_BAR].id);
    gfx_quads_begin();
    gfx_setcolor(1.0f, 1.0f, 1.0f, 0.9f);
    gfx_quads_setsubset(0,0.1f,screen.w*0.015f,1-0.1f);
    gfx_quads_drawTL(0,console_height-10.0f,screen.w,10.0f);
    gfx_quads_end();
    
    console_height -= 10.0f;
    
    INSTANCE *console = current_console();

	{
		float font_size = 10.0f;
		float row_height = font_size*1.25f;
		float x = 3;
		float y = console_height - row_height - 2;

		// render prompt		
		TEXT_CURSOR cursor;
		gfx_text_set_cursor(&cursor, x, y, font_size, TEXTFLAG_RENDER);

		const char *prompt = "> ";
		if(console_type)
		{
			if(client_state() == CLIENTSTATE_ONLINE)
			{
				if(client_rcon_authed())
					prompt = "rcon> ";
				else
					prompt = "ENTER PASSWORD> ";
			}
			else
				prompt = "NOT CONNECTED> ";
		}

		gfx_text_ex(&cursor, prompt, -1);
		
		// render console input
		gfx_text_ex(&cursor, console->input.get_string(), console->input.cursor_offset());
		TEXT_CURSOR marker = cursor;
		gfx_text_ex(&marker, "|", -1);
		gfx_text_ex(&cursor, console->input.get_string()+console->input.cursor_offset(), -1);
		
		// render version
		char buf[128];
		str_format(buf, sizeof(buf), "v%s", GAME_VERSION);
		float version_width = gfx_text_width(0, font_size, buf, -1);
		gfx_text(0, screen.w-version_width-5, y, font_size, buf, -1);

		// render log
		y -= row_height;
		char *entry = (char *)ringbuf_last(console->backlog);
		while (y > 0.0f && entry)
		{
			gfx_text(0, x, y, font_size, entry, -1);
			y -= row_height;

			entry = (char *)ringbuf_prev(console->backlog, entry);
		}
	}	
}

void CONSOLE::on_message(int msgtype, void *rawmsg)
{
}

bool CONSOLE::on_input(INPUT_EVENT e)
{
	if(console_state == CONSOLE_CLOSED)
		return false;
	if(e.key >= KEY_F1 && e.key <= KEY_F25)
		return false;

	if(e.key == KEY_ESC && (e.flags&INPFLAG_PRESS))
		toggle(console_type);
	else
		current_console()->on_input(e);
		
	return true;
}

void CONSOLE::toggle(int type)
{
	if(console_type != type && (console_state == CONSOLE_OPEN || console_state == CONSOLE_OPENING))
	{
		// don't toggle console, just switch what console to use
	}
	else
	{	
		if (console_state == CONSOLE_CLOSED || console_state == CONSOLE_OPEN)
		{
			state_change_end = time_now()+state_change_duration;
		}
		else
		{
			float progress = state_change_end-time_now();
			float reversed_progress = state_change_duration-progress;

			state_change_end = time_now()+reversed_progress;
		}

		if (console_state == CONSOLE_CLOSED || console_state == CONSOLE_CLOSING)
			console_state = CONSOLE_OPENING;
		else
			console_state = CONSOLE_CLOSING;
	}

	console_type = type;
}

void CONSOLE::con_toggle_local_console(void *result, void *user_data)
{
	((CONSOLE *)user_data)->toggle(0);
}

void CONSOLE::con_toggle_remote_console(void *result, void *user_data)
{
	((CONSOLE *)user_data)->toggle(1);
}

void CONSOLE::client_console_print_callback(const char *str, void *user_data)
{
	((CONSOLE *)user_data)->local_console.print_line(str);
}

void CONSOLE::on_init()
{
	//
	console_register_print_callback(client_console_print_callback, this);
	
	MACRO_REGISTER_COMMAND("toggle_local_console", "", con_toggle_local_console, this);
	MACRO_REGISTER_COMMAND("toggle_remote_console", "", con_toggle_remote_console, this);
}


/*
static void client_console_print(const char *str)
{
	// TODO: repair me
	//local_console.print_line(str);
}

void console_rcon_print(const char *line)
{
	// TODO: repair me
	//remote_console.print_line(line);
}*/

/*
static void con_team(void *result, void *user_data)
{
	send_switch_team(console_arg_int(result, 0));
}

static void con_say(void *result, void *user_data)
{
	chat_say(0, console_arg_string(result, 0));
}

static void con_sayteam(void *result, void *user_data)
{
	chat_say(1, console_arg_string(result, 0));
}

void send_kill(int client_id);

static void con_kill(void *result, void *user_data)
{
	send_kill(-1);
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
}

static void con_toggle_local_console(void *result, void *user_data)
{
	console_toggle(0);
}

static void con_toggle_remote_console(void *result, void *user_data)
{
	console_toggle(1);
}

static void con_emote(void *result, void *user_data)
{
	send_emoticon(console_arg_int(result, 0));
}

extern void con_chat(void *result, void *user_data);

void client_console_init()
{
	console_register_print_callback(client_console_print);

	//
	MACRO_REGISTER_COMMAND("toggle_local_console", "", con_toggle_local_console, 0x0);
	MACRO_REGISTER_COMMAND("toggle_remote_console", "", con_toggle_remote_console, 0x0);

	//
	MACRO_REGISTER_COMMAND("team", "i", con_team, 0x0);
	MACRO_REGISTER_COMMAND("kill", "", con_kill, 0x0);
	
	// bindings
	MACRO_REGISTER_COMMAND("bind", "sr", con_bind, 0x0);
	MACRO_REGISTER_COMMAND("unbind", "s", con_unbind, 0x0);
	MACRO_REGISTER_COMMAND("unbindall", "", con_unbindall, 0x0);
	
	MACRO_REGISTER_COMMAND("dump_binds", "", con_dump_binds, 0x0);

	// chatting
	MACRO_REGISTER_COMMAND("say", "r", con_say, 0x0);
	MACRO_REGISTER_COMMAND("say_team", "r", con_sayteam, 0x0);
	MACRO_REGISTER_COMMAND("chat", "s", con_chat, 0x0);
	MACRO_REGISTER_COMMAND("emote", "i", con_emote, 0);

	// game commands
	MACRO_REGISTER_COMMAND("+left", "", con_key_input_state, &input_direction_left);
	MACRO_REGISTER_COMMAND("+right", "", con_key_input_state, &input_direction_right);
	MACRO_REGISTER_COMMAND("+jump", "", con_key_input_state, &input_data.jump);
	MACRO_REGISTER_COMMAND("+hook", "", con_key_input_state, &input_data.hook);
	MACRO_REGISTER_COMMAND("+fire", "", con_key_input_counter, &input_data.fire);
	MACRO_REGISTER_COMMAND("+weapon1", "", con_key_input_weapon, (void *)1);
	MACRO_REGISTER_COMMAND("+weapon2", "", con_key_input_weapon, (void *)2);
	MACRO_REGISTER_COMMAND("+weapon3", "", con_key_input_weapon, (void *)3);
	MACRO_REGISTER_COMMAND("+weapon4", "", con_key_input_weapon, (void *)4);
	MACRO_REGISTER_COMMAND("+weapon5", "", con_key_input_weapon, (void *)5);

	MACRO_REGISTER_COMMAND("+nextweapon", "", con_key_input_nextprev_weapon, &input_data.next_weapon);
	MACRO_REGISTER_COMMAND("+prevweapon", "", con_key_input_nextprev_weapon, &input_data.prev_weapon);
	
	MACRO_REGISTER_COMMAND("+emote", "", con_key_input_state, &emoticon_selector_active);
	MACRO_REGISTER_COMMAND("+scoreboard", "", con_key_input_state, &scoreboard_active);
	
	binds_default();
}

bool console_input_cli(INPUT_EVENT e, void *user_data)
{
	if(!console_active())
		return false;
	
	if(e.key == KEY_ESC && (e.flags&INPFLAG_PRESS))
		console_toggle(console_type);
	else
		current_console()->handle_event(e);
	return true;
}

static bool console_execute_event(INPUT_EVENT e)
{
	// don't handle invalid events and keys that arn't set to anything
	if(e.key <= 0 || e.key >= KEY_LAST || keybindings[e.key][0] == 0)
		return false;

	int stroke = 0;
	if(e.flags&INPFLAG_PRESS)
		stroke = 1;
	console_execute_line_stroked(stroke, keybindings[e.key]);
	return true;
}

bool console_input_special_binds(INPUT_EVENT e, void *user_data)
{
	// only handle function keys
	if(e.key < KEY_F1 || e.key > KEY_F25)
		return false;
	return console_execute_event(e);
}

bool console_input_normal_binds(INPUT_EVENT e, void *user_data)
{
	// need to be ingame for these binds
	if(client_state() != CLIENTSTATE_ONLINE)
		return false;
	return console_execute_event(e);
}

void console_toggle(int type)
{
	if(console_type != type && (console_state == CONSOLE_OPEN || console_state == CONSOLE_OPENING))
	{
		// don't toggle console, just switch what console to use
	}
	else
	{	
		if (console_state == CONSOLE_CLOSED || console_state == CONSOLE_OPEN)
		{
			state_change_end = time_now()+state_change_duration;
		}
		else
		{
			float progress = state_change_end-time_now();
			float reversed_progress = state_change_duration-progress;

			state_change_end = time_now()+reversed_progress;
		}

		if (console_state == CONSOLE_CLOSED || console_state == CONSOLE_CLOSING)
			console_state = CONSOLE_OPENING;
		else
			console_state = CONSOLE_CLOSING;
	}

	console_type = type;
}



int console_active()
{
	return console_state != CONSOLE_CLOSED;
}

*/
