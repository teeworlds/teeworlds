#include "gc_console.h"
#include "../generated/gc_data.h"

extern "C" {
	#include <engine/e_system.h>
	#include <engine/e_client_interface.h>
	#include <engine/e_config.h>
	#include <engine/e_console.h>
	#include <engine/e_ringbuffer.h>
	#include <engine/client/ec_font.h>
}

#include <cstring>
#include <cstdio>

#include "gc_ui.h"
#include "gc_client.h"

#include "../g_version.h"

enum
{
	CONSOLE_CLOSED,
	CONSOLE_OPENING,
	CONSOLE_OPEN,
	CONSOLE_CLOSING,
};

class CONSOLE
{
public:
	char history_data[65536];
	RINGBUFFER *history;
	char *history_entry;
	
	char backlog_data[65536];
	RINGBUFFER *backlog;


	line_input input;
	
	int type;
	
public:
	CONSOLE(int t)
	{
		// init ringbuffers
		history = ringbuf_init(history_data, sizeof(history_data));
		backlog = ringbuf_init(backlog_data, sizeof(backlog_data));
		
		history_entry = 0x0;
		
		type = t;
	}
	
	void execute_line(const char *line)
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
	
	void handle_event(INPUT_EVENT e)
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
	
	void print_line(const char *line)
	{
		int len = strlen(line);

		if (len > 255)
			len = 255;

		char *entry = (char *)ringbuf_allocate(backlog, len+1);
		mem_copy(entry, line, len+1);
	}
};

static CONSOLE local_console(0);
static CONSOLE remote_console(1);

static int console_type = 0;
static int console_state = CONSOLE_CLOSED;
static float state_change_end = 0.0f;
static const float state_change_duration = 0.1f;

static float time_now()
{
	static long long time_start = time_get();
	return float(time_get()-time_start)/float(time_freq());
}

static CONSOLE *current_console()
{
    if(console_type != 0)
    	return &remote_console;
    return &local_console;
}

static void client_console_print(const char *str)
{
	local_console.print_line(str);
}

void console_rcon_print(const char *line)
{
	remote_console.print_line(line);
}

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



static void con_chat(void *result, void *user_data)
{
	const char *mode = console_arg_string(result, 0);
	if(strcmp(mode, "all") == 0)
		chat_enable_mode(0);
	else if(strcmp(mode, "team") == 0)
		chat_enable_mode(1);
	else
		dbg_msg("console", "expected all or team as mode");
}


void send_kill(int client_id);

static void con_kill(void *result, void *user_data)
{
	send_kill(-1);
}

static char keybindings[KEY_LAST][128] = {{0}};

const char *binds_get(int keyid)
{
	if(keyid > 0 && keyid < KEY_LAST)
		return keybindings[keyid];
	return "";
}

void binds_set(int keyid, const char *str)
{
	if(keyid < 0 && keyid >= KEY_LAST)
		return;
		
	str_copy(keybindings[keyid], str, sizeof(keybindings[keyid]));
	if(!keybindings[keyid][0])
		dbg_msg("binds", "unbound %s (%d)", inp_key_name(keyid), keyid);
	else
		dbg_msg("binds", "bound %s (%d) = %s", inp_key_name(keyid), keyid, keybindings[keyid]);
}

void binds_default()
{
	for(int i = 0; i < KEY_LAST; i++)
		keybindings[i][0] = 0;

	// set default key bindings
	binds_set(KEY_F1, "toggle_local_console");
	binds_set(KEY_F2, "toggle_remote_console");
	binds_set(KEY_TAB, "+scoreboard");
	binds_set(KEY_F10, "screenshot");
	
	binds_set('A', "+left");
	binds_set('D', "+right");
	binds_set(KEY_SPACE, "+jump");
	binds_set(KEY_MOUSE_1, "+fire");
	binds_set(KEY_MOUSE_2, "+hook");
	binds_set(KEY_LSHIFT, "+emote");

	binds_set('1', "+weapon1");
	binds_set('2', "+weapon2");
	binds_set('3', "+weapon3");
	binds_set('4', "+weapon4");
	binds_set('5', "+weapon5");
	
	binds_set(KEY_MOUSE_WHEEL_UP, "+prevweapon");
	binds_set(KEY_MOUSE_WHEEL_DOWN, "+nextweapon");
	
	binds_set('T', "chat all");
	binds_set('Y', "chat team");
}

static int get_key_id(const char *key_name)
{
	// check for numeric
	if(key_name[0] == '#')
	{
		int i = atoi(key_name+1);
		if(i > 0 && i < KEY_LAST)
			return i; // numeric
	}
		
	// search for key
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(strcmp(key_name, inp_key_name(i)) == 0)
			return i;
	}
	
	return 0;
}

static void con_bind(void *result, void *user_data)
{
	const char *key_name = console_arg_string(result, 0);
	int id = get_key_id(key_name);
	
	if(!id)
	{
		dbg_msg("binds", "key %s not found", key_name);
		return;
	}
	
	binds_set(id, console_arg_string(result, 1));
}


static void con_unbind(void *result, void *user_data)
{
	const char *key_name = console_arg_string(result, 0);
	int id = get_key_id(key_name);
	
	if(!id)
	{
		dbg_msg("binds", "key %s not found", key_name);
		return;
	}
	
	binds_set(id, "");
}

static void con_dump_binds(void *result, void *user_data)
{
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(keybindings[i][0] == 0)
			continue;
		dbg_msg("binds", "%s (%d) = %s", inp_key_name(i), i, keybindings[i]);
	}
}

void binds_save()
{
	char buffer[256];
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(keybindings[i][0] == 0)
			continue;
		str_format(buffer, sizeof(buffer), "bind %s %s", inp_key_name(i), keybindings[i]);
		client_save_line(buffer);
	}
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
	MACRO_REGISTER_COMMAND("dump_binds", "", con_dump_binds, 0x0);

	// chatting
	MACRO_REGISTER_COMMAND("say", "r", con_say, 0x0);
	MACRO_REGISTER_COMMAND("say_team", "r", con_sayteam, 0x0);
	MACRO_REGISTER_COMMAND("chat", "s", con_chat, 0x0);
	MACRO_REGISTER_COMMAND("emote", "i", con_emote, 0);

	// game commands
	MACRO_REGISTER_COMMAND("+left", "", con_key_input_state, &input_data.left);
	MACRO_REGISTER_COMMAND("+right", "", con_key_input_state, &input_data.right);
	MACRO_REGISTER_COMMAND("+jump", "", con_key_input_state, &input_data.jump);
	MACRO_REGISTER_COMMAND("+hook", "", con_key_input_state, &input_data.hook);
	MACRO_REGISTER_COMMAND("+fire", "", con_key_input_counter, &input_data.fire);
	MACRO_REGISTER_COMMAND("+weapon1", "", con_key_input_weapon, (void *)1);
	MACRO_REGISTER_COMMAND("+weapon2", "", con_key_input_weapon, (void *)2);
	MACRO_REGISTER_COMMAND("+weapon3", "", con_key_input_weapon, (void *)3);
	MACRO_REGISTER_COMMAND("+weapon4", "", con_key_input_weapon, (void *)4);
	MACRO_REGISTER_COMMAND("+weapon5", "", con_key_input_weapon, (void *)5);

	MACRO_REGISTER_COMMAND("+nextweapon", "", con_key_input_counter, &input_data.next_weapon);
	MACRO_REGISTER_COMMAND("+prevweapon", "", con_key_input_counter, &input_data.prev_weapon);
	
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

bool console_input_special_binds(INPUT_EVENT e, void *user_data)
{
	// only handle function keys
	if(e.key < KEY_F1 || e.key > KEY_F25)
		return false;
	return console_input_normal_binds(e, user_data);
}

bool console_input_normal_binds(INPUT_EVENT e, void *user_data)
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

// only defined for 0<=t<=1
static float console_scale_func(float t)
{
	//return t;
	return sinf(acosf(1.0f-t));
}

void console_render()
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
    
    CONSOLE *console = current_console();

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
			if(client_rcon_authed())
				prompt = "rcon> ";
			else
				prompt = "rcon password> ";
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

int console_active()
{
	return console_state != CONSOLE_CLOSED;
}

