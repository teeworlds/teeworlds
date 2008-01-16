#include "gc_console.h"

extern "C" {
	#include <engine/e_system.h>
	#include <engine/e_interface.h>
	#include <engine/e_config.h>
	#include <engine/e_console.h>
	#include <engine/client/ec_font.h>
}

#include <cstring>
#include <cstdio>

#include "gc_ui.h"

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

void client_console_init()
{
	console_register_print_callback(client_console_print);
}

void console_handle_input()
{
	for(int i = 0; i < inp_num_events(); i++)
	{
		INPUTEVENT e = inp_get_event(i);

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
			else if(e.key == KEY_ENTER)
			{
				console_execute(console_input);
				console_input[0] = 0;
				console_input_len = 0;
			}
		}
	}

	if (active)
		inp_clear_events();
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
		float row_spacing = font_size*1.4f;
		float width = gfx_text_width(0, 12, console_input, -1);
		float x = 3, y = console_height - row_spacing - 2;
		int backlog_index = backlog_len-1;

		gfx_text(0, x, y, font_size, console_input, -1);
		gfx_text(0, x+width+1, y, font_size, "_", -1);

		y -= row_spacing;

		while (y > 0.0f && backlog_index >= 0)
		{
			const char *line = backlog[backlog_index];
			gfx_text(0, x, y, font_size, line, -1);

			backlog_index--;
			y -= row_spacing;
		}
	}
}

int console_active()
{
	return active;
}

