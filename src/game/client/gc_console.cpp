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

	unsigned int input_len;
	char input[256];
	
	int type;
	
public:
	CONSOLE(int t)
	{
		// clear input
		input_len = 0;
		mem_zero(input, sizeof(input));
	
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
		if (!(e.ch >= 0 && e.ch < 32))
		{
			if (input_len < sizeof(input) - 1)
			{
				input[input_len] = e.ch;
				input[input_len+1] = 0;
				input_len++;

				history_entry = 0x0;
			}
		}

		if(e.key == KEY_BACKSPACE)
		{
			if(input_len > 0)
			{
				input[input_len-1] = 0;
				input_len--;

				history_entry = 0x0;
			}
		}
		else if(e.key == KEY_ENTER || e.key == KEY_KP_ENTER)
		{
			if (input_len)
			{
				char *entry = (char *)ringbuf_allocate(history, input_len+1);
				mem_copy(entry, input, input_len+1);
				
				execute_line(input);
				input[0] = 0;
				input_len = 0;

				history_entry = 0x0;
			}
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
				{
					mem_copy(input, history_entry, len+1);
					input_len = len;
				}
			}

		}
		else if (e.key == KEY_DOWN)
		{
			if (history_entry)
				history_entry = (char *)ringbuf_next(history, history_entry);

			if (history_entry)
			{
				unsigned int len = strlen(history_entry);
				if (len < sizeof(input) - 1)
				{
					mem_copy(input, history_entry, len+1);

					input_len = len;
				}
			}
			else
			{
				input[0] = 0;
				input_len = 0;
			}
		}		
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
	int new_team;
	console_result_int(result, 1, &new_team);
	send_switch_team(new_team);
}

/*
static void con_history(void *result, void *user_data)
{
	char *entry = (char *)ringbuf_first(console_history);

	while (entry)
	{
		dbg_msg("console/history", entry);

		entry = (char *)ringbuf_next(console_history, entry);
	}
}*/

void send_kill(int client_id);

static void con_kill(void *result, void *user_data)
{
	send_kill(-1);
}

void client_console_init()
{
	console_register_print_callback(client_console_print);
	MACRO_REGISTER_COMMAND("team", "i", con_team, 0x0);
	//MACRO_REGISTER_COMMAND("history", "", con_history, 0x0);
	MACRO_REGISTER_COMMAND("kill", "", con_kill, 0x0);
}

void console_handle_input()
{
	int was_active = console_active();

	for(int i = 0; i < inp_num_events(); i++)
	{
		INPUT_EVENT e = inp_get_event(i);

		if (e.key == config.key_toggleconsole)
			console_toggle(0);
		else if (e.key == config.key_toggleconsole+1)
			console_toggle(1);
		else if(console_active())
			current_console()->handle_event(e);
	}

	if (was_active || console_active())
	{
		inp_clear_events();
		inp_clear_key_states();
	}
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

	// do shadow
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
		float width = gfx_text_width(0, font_size, console->input, -1);
		float x = 3, y = console_height - row_height - 2;
		const char *prompt = ">";
		if(console_type)
		{
			if(client_rcon_authed())
				prompt = "rcon>";
			else
				prompt = "rcon password>";
		}
		
		float prompt_width = gfx_text_width(0, font_size,prompt, -1)+2;

		gfx_text(0, x, y, font_size, prompt, -1);
		gfx_text(0, x+prompt_width, y, font_size, console->input, -1);
		gfx_text(0, x+prompt_width+width+1, y, font_size, "_", -1);

		char buf[128];
		str_format(buf, sizeof(buf), "Teewars v%s %s", TEEWARS_VERSION);
		float version_width = gfx_text_width(0, font_size, buf, -1);
		gfx_text(0, screen.w-version_width-5, y, font_size, buf, -1);

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

