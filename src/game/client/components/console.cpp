//#include "gc_console.hpp"
#include <math.h>

#include <game/generated/gc_data.hpp>

#include <base/system.h>

#include <engine/e_client_interface.h>

extern "C" {
	#include <engine/e_ringbuffer.h>
}

#include <cstring>
#include <cstdio>

#include <game/client/ui.hpp>

#include <game/version.hpp>

#include <game/client/lineinput.hpp>
#include <game/client/render.hpp>

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
	history = ringbuf_init(history_data, sizeof(history_data), RINGBUF_FLAG_RECYCLE);
	backlog = ringbuf_init(backlog_data, sizeof(backlog_data), RINGBUF_FLAG_RECYCLE);
	
	history_entry = 0x0;
	
	type = t;
	
	if(t == 0)
		completion_flagmask = CFGFLAG_CLIENT;
	else
		completion_flagmask = CFGFLAG_SERVER;

	completion_buffer[0] = 0;
	completion_chosen = -1;
	
	command = 0x0;
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

void CONSOLE::INSTANCE::possible_commands_complete_callback(const char *str, void *user)
{
	CONSOLE::INSTANCE *instance = (CONSOLE::INSTANCE *)user;
	if(instance->completion_chosen == instance->completion_enumeration_count)
		instance->input.set(str);
	instance->completion_enumeration_count++;
}

void CONSOLE::INSTANCE::on_input(INPUT_EVENT e)
{
	bool handled = false;
	
	if(e.flags&INPFLAG_PRESS)
	{
		if(e.key == KEY_RETURN || e.key == KEY_KP_ENTER)
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
		else if(e.key == KEY_TAB)
		{
			completion_chosen++;
			completion_enumeration_count = 0;
			console_possible_commands(completion_buffer, completion_flagmask, possible_commands_complete_callback, this);

			// handle wrapping
			if(completion_enumeration_count && completion_chosen >= completion_enumeration_count)
			{
				completion_chosen %= completion_enumeration_count;
				completion_enumeration_count = 0;
				console_possible_commands(completion_buffer, completion_flagmask, possible_commands_complete_callback, this);
			}
		}
		
		if(e.key != KEY_TAB)
		{
			completion_chosen = -1;
			str_copy(completion_buffer, input.get_string(), sizeof(completion_buffer));
		}

		// find the current command
		{
			char buf[64] = {0};
			const char *src = get_string();
			int i = 0;
			for(; i < (int)sizeof(buf) && *src && *src != ' '  && *src != ' '; i++, src++)
				buf[i] = *src;
			buf[i] = 0;
			
			command = console_get_command(buf);
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

struct RENDERINFO
{
	TEXT_CURSOR cursor;
	const char *current_cmd;
	int wanted_completion;
	int enum_count;
};

void CONSOLE::possible_commands_render_callback(const char *str, void *user)
{
	RENDERINFO *info = (RENDERINFO *)user;
	
	if(info->enum_count == info->wanted_completion)
	{
		float tw = gfx_text_width(info->cursor.font, info->cursor.font_size, str, -1);
		gfx_texture_set(-1);
		gfx_quads_begin();
			gfx_setcolor(229.0f/255.0f,185.0f/255.0f,4.0f/255.0f,0.85f);
			draw_round_rect(info->cursor.x-3, info->cursor.y, tw+5, info->cursor.font_size+4, info->cursor.font_size/3);
		gfx_quads_end();
		
		gfx_text_color(0.05f, 0.05f, 0.05f,1);
		gfx_text_ex(&info->cursor, str, -1);
	}
	else
	{
		const char *match_start = str_find_nocase(str, info->current_cmd);
		
		if(match_start)
		{
			gfx_text_color(0.5f,0.5f,0.5f,1);
			gfx_text_ex(&info->cursor, str, match_start-str);
			gfx_text_color(229.0f/255.0f,185.0f/255.0f,4.0f/255.0f,1);
			gfx_text_ex(&info->cursor, match_start, strlen(info->current_cmd));
			gfx_text_color(0.5f,0.5f,0.5f,1);
			gfx_text_ex(&info->cursor, match_start+strlen(info->current_cmd), -1);
		}
		else
		{
			gfx_text_color(0.75f,0.75f,0.75f,1);
			gfx_text_ex(&info->cursor, str, -1);
		}
	}
	
	info->enum_count++;
	info->cursor.x += 7.0f;
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

	if (console_state == CONSOLE_OPEN && config.cl_editor)
		toggle(0);	
		
	if (console_state == CONSOLE_CLOSED)
		return;
		
	if (console_state == CONSOLE_OPEN)
		inp_mouse_mode_absolute();

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
    
    console_height -= 22.0f;
    
    INSTANCE *console = current_console();

	{
		float font_size = 10.0f;
		float row_height = font_size*1.25f;
		float x = 3;
		float y = console_height - row_height - 2;

		// render prompt		
		TEXT_CURSOR cursor;
		gfx_text_set_cursor(&cursor, x, y, font_size, TEXTFLAG_RENDER);

		RENDERINFO info;
		info.wanted_completion = console->completion_chosen;
		info.enum_count = 0;
		info.current_cmd = console->completion_buffer;
		gfx_text_set_cursor(&info.cursor, x, y+12.0f, font_size, TEXTFLAG_RENDER);

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

		// render possible commands
		if(console->input.get_string()[0] != 0)
		{
			console_possible_commands(console->completion_buffer, console->completion_flagmask, possible_commands_render_callback, &info);
			
			if(info.enum_count <= 0)
			{
				if(console->command)
				{
					
					char buf[512];
					str_format(buf, sizeof(buf), "Help: %s ", console->command->help);
					gfx_text_ex(&info.cursor, buf, -1);
					gfx_text_color(0.75f, 0.75f, 0.75f, 1);
					str_format(buf, sizeof(buf), "Syntax: %s %s", console->command->name, console->command->params);
					gfx_text_ex(&info.cursor, buf, -1);
				}
			}
		}
		gfx_text_color(1,1,1,1);

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
	if(e.key >= KEY_F1 && e.key <= KEY_F15)
		return false;

	if(e.key == KEY_ESCAPE && (e.flags&INPFLAG_PRESS))
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
		{
			inp_mouse_mode_absolute();
			console_state = CONSOLE_OPENING;
		}
		else
		{
			inp_mouse_mode_relative();
			console_state = CONSOLE_CLOSING;
		}
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

void CONSOLE::print_line(int type, const char *line)
{
	if(type == 0)
		local_console.print_line(line);
	else if(type == 1)
		remote_console.print_line(line);
}

void CONSOLE::on_console_init()
{
	//
	console_register_print_callback(client_console_print_callback, this);
	
	MACRO_REGISTER_COMMAND("toggle_local_console", "", CFGFLAG_CLIENT, con_toggle_local_console, this, "Toggle local console");
	MACRO_REGISTER_COMMAND("toggle_remote_console", "", CFGFLAG_CLIENT, con_toggle_remote_console, this, "Toggle remote console");
}

/*
static void con_team(void *result, void *user_data)
{
	send_switch_team(console_arg_int(result, 0));
}

static void con_kill(void *result, void *user_data)
{
	send_kill(-1);
}

void send_kill(int client_id);

static void con_emote(void *result, void *user_data)
{
	send_emoticon(console_arg_int(result, 0));
}

extern void con_chat(void *result, void *user_data);

void client_console_init()
{
	//
	MACRO_REGISTER_COMMAND("team", "i", con_team, 0x0);
	MACRO_REGISTER_COMMAND("kill", "", con_kill, 0x0);

	// chatting
	MACRO_REGISTER_COMMAND("emote", "i", con_emote, 0);
	MACRO_REGISTER_COMMAND("+emote", "", con_key_input_state, &emoticon_selector_active);
}
*/
