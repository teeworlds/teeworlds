#include "gc_console.h"

extern "C" {
	#include <engine/e_system.h>
	#include <engine/e_client_interface.h>
	#include <engine/e_config.h>
	#include <engine/e_console.h>
	#include <engine/client/ec_font.h>
}

#include <cstring>
#include <cstdio>

#include "gc_ui.h"
#include "gc_client.h"

static unsigned int console_input_len = 0;
static char console_input[256] = {0};
static int active = 0;

static char backlog[256][256] = {{0}};
static int backlog_len;

static void client_console_print(const char *str)
{
	int len = strlen(str);

	if (len > 255)
		len = 255;

	if (backlog_len >= 256)
	{
		puts("console backlog full");
	}

	memcpy(backlog[backlog_len], str, len);
	backlog[backlog_len][len] = 0;

	backlog_len++;

	//dbg_msg("console", "FROM CLIENT!! %s", str);
}

static void connect_command(struct lexer_result *result, void *user_data)
{
	const char *address;
	extract_result_string(result, 1, &address);
	client_connect(address);
}

static void disconnect_command(struct lexer_result *result, void *user_data)
{
	client_disconnect();
}

static void quit_command(struct lexer_result *result, void *user_data)
{
	client_quit();
}

static void con_team(struct lexer_result *result, void *user_data)
{
	int new_team;
	extract_result_int(result, 1, &new_team);
	send_switch_team(new_team);
}

void client_console_init()
{
	console_register_print_callback(client_console_print);
	MACRO_REGISTER_COMMAND("quit", "", quit_command, 0x0);
	MACRO_REGISTER_COMMAND("connect", "s", connect_command, 0x0);
	MACRO_REGISTER_COMMAND("disconnect", "", disconnect_command, 0x0);
	MACRO_REGISTER_COMMAND("team", "i", con_team, 0x0);
}

void console_handle_input()
{
	int was_active = active;

	for(int i = 0; i < inp_num_events(); i++)
	{
		INPUT_EVENT e = inp_get_event(i);

		if (e.key == KEY_F3)
		{
			console_toggle();
		}

		if (active)
		{
			if (!(e.ch >= 0 && e.ch < 32))
			{
				if (console_input_len < sizeof(console_input) - 1)
				{
					console_input[console_input_len] = e.ch;
					console_input[console_input_len+1] = 0;
					console_input_len++;
				}
			}

			if(e.key == KEY_BACKSPACE)
			{
				if(console_input_len > 0)
				{
					console_input[console_input_len-1] = 0;
					console_input_len--;
				}
			}
			else if(e.key == KEY_ENTER || e.key == KEY_KP_ENTER)
			{
				if (console_input_len)
				{
					console_execute(console_input);
					console_input[0] = 0;
					console_input_len = 0;
				}
			}
		}
	}

	if (was_active || active)
	{
		inp_clear_events();
		inp_clear_key_states();
	}
}

void console_toggle()
{
	active ^= 1;
}

void console_render()
{
    RECT screen = *ui_screen();
	float console_height = screen.h*3/5.0f;
	gfx_mapscreen(screen.x, screen.y, screen.w, screen.h);

    gfx_texture_set(-1);
    gfx_quads_begin();
    gfx_setcolor(0.4,0.2,0.2,0.8);
    gfx_quads_drawTL(0,0,screen.w,console_height);
    gfx_quads_end();

	{
		float font_size = 12.0f;
		float row_height = font_size*1.4f;
		float width = gfx_text_width(0, font_size, console_input, -1);
		float x = 3, y = console_height - row_height - 2;
		int backlog_index = backlog_len-1;
		float prompt_width = gfx_text_width(0, font_size, ">", -1)+2;

		gfx_text(0, x, y, font_size, ">", -1);
		gfx_text(0, x+prompt_width, y, font_size, console_input, -1);
		gfx_text(0, x+prompt_width+width+1, y, font_size, "_", -1);

		y -= row_height;

		while (y > 0.0f && backlog_index >= 0)
		{
			const char *line = backlog[backlog_index];
			gfx_text(0, x, y, font_size, line, -1);

			backlog_index--;
			y -= row_height;
		}
	}
}

int console_active()
{
	return active;
}

