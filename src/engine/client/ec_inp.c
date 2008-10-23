/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include "SDL.h"

#include <base/system.h>
#include <engine/e_client_interface.h>
#include <engine/e_config.h>

static struct
{
	unsigned char presses;
	unsigned char releases;
} input_count[2][1024] = {{{0}}, {{0}}};

static unsigned char input_state[2][1024] = {{0}, {0}};

static int input_current = 0;
static int input_grabbed = 0;
static int input_use_grab = 0;

static unsigned int last_release = 0;
static unsigned int release_delta = -1;

void inp_mouse_relative(int *x, int *y)
{
	static int last_x = 0, last_y = 0;
	static int last_sens = 100.0f;
	int nx = 0, ny = 0;
	float sens = config.inp_mousesens/100.0f;
	
	if(last_sens != config.inp_mousesens)
	{
		last_x = (last_x/(float)last_sens)*(float)config.inp_mousesens;
		last_y = (last_y/(float)last_sens)*(float)config.inp_mousesens;
		last_sens = config.inp_mousesens;
	}
	
	if(input_use_grab)
		SDL_GetRelativeMouseState(&nx, &ny);
	else
	{
		if(input_grabbed)
		{
			SDL_GetMouseState(&nx,&ny);
			SDL_WarpMouse(gfx_screenwidth()/2,gfx_screenheight()/2);
			nx -= gfx_screenwidth()/2; ny -= gfx_screenheight()/2;
		}
	}

	*x = nx*sens;
	*y = ny*sens;
}

enum
{
	INPUT_BUFFER_SIZE=32
};

static INPUT_EVENT input_events[INPUT_BUFFER_SIZE];
static int num_events = 0;

static void add_event(char c, int key, int flags)
{
	if(num_events != INPUT_BUFFER_SIZE)
	{
		input_events[num_events].ch = c;
		input_events[num_events].key = key;
		input_events[num_events].flags = flags;
		num_events++;
	}
}

int inp_num_events()
{
	return num_events;
}

void inp_clear_events()
{
	num_events = 0;
}

INPUT_EVENT inp_get_event(int index)
{
	if(index < 0 || index >= num_events)
	{
		INPUT_EVENT e = {0,0};
		return e;
	}
	
	return input_events[index];
}

void inp_init()
{
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL); 
}

void inp_mouse_mode_absolute()
{
	SDL_ShowCursor(1);
	input_grabbed = 0;
	if(input_use_grab)
		SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void inp_mouse_mode_relative()
{
	SDL_ShowCursor(0);
	input_grabbed = 1;
	if(input_use_grab)
		SDL_WM_GrabInput(SDL_GRAB_ON);
}

int inp_mouse_doubleclick()
{
	return release_delta < (time_freq() >> 2);
}

void inp_clear_key_states()
{
	mem_zero(input_state, sizeof(input_state));
	mem_zero(input_count, sizeof(input_count));
}

int inp_key_presses(int key)
{
	return input_count[input_current][key].presses;
}

int inp_key_releases(int key)
{
	return input_count[input_current][key].releases;
}

int inp_key_state(int key)
{
	return input_state[input_current][key];
}

int inp_key_pressed(int key) { return input_state[input_current][key]; }
int inp_key_was_pressed(int key) { return input_state[input_current^1][key]; }
int inp_key_down(int key) { return inp_key_pressed(key)&&!inp_key_was_pressed(key); }
int inp_button_pressed(int button) { return input_state[input_current][button]; }

void inp_update()
{
	int i;
	
	if(input_grabbed && !gfx_window_active())
		inp_mouse_mode_absolute();

	/*if(!input_grabbed && gfx_window_active())
		inp_mouse_mode_relative();*/
	
	/* clear and begin count on the other one */
	mem_zero(&input_count[input_current], sizeof(input_count[input_current]));
	input_current^=1;
	
	{
		Uint8 *state = SDL_GetKeyState(&i);
		if(i >= KEY_LAST)
			i = KEY_LAST-1;
		mem_copy(input_state[input_current], state, i);
	}
	
	input_state[input_current][KEY_MOUSE_1] = 0;
	input_state[input_current][KEY_MOUSE_2] = 0;
	input_state[input_current][KEY_MOUSE_3] = 0;
	input_state[input_current][KEY_MOUSE_WHEEL_UP] = 0;
	input_state[input_current][KEY_MOUSE_WHEEL_DOWN] = 0;
	i = SDL_GetMouseState(NULL, NULL);
	if(i&SDL_BUTTON(1)) input_state[input_current][KEY_MOUSE_1] = 1; /* 1 is left */
	if(i&SDL_BUTTON(3)) input_state[input_current][KEY_MOUSE_2] = 1; /* 3 is right */
	if(i&SDL_BUTTON(2)) input_state[input_current][KEY_MOUSE_3] = 1; /* 2 is middle */
	if(i&SDL_BUTTON(4)) input_state[input_current][KEY_MOUSE_WHEEL_UP] = 1;
	if(i&SDL_BUTTON(5)) input_state[input_current][KEY_MOUSE_WHEEL_DOWN] = 1;
	
	{
		SDL_Event event;
	
		while(SDL_PollEvent(&event))
		{
			int key = -1;
			int action = INPFLAG_PRESS;
			switch (event.type)
			{
				/* handle keys */
				case SDL_KEYDOWN:
					if(event.key.keysym.unicode < 255)
						add_event(event.key.keysym.unicode, 0, 0);
					key = event.key.keysym.sym;
					break;
				case SDL_KEYUP:
					action = INPFLAG_RELEASE;
					key = event.key.keysym.sym;
					break;
				
				/* handle mouse buttons */
				case SDL_MOUSEBUTTONUP:
					action = INPFLAG_RELEASE;
					
					if(event.button.button == 1) key = KEY_MOUSE_1;
					{
						release_delta = time_get() - last_release;
						last_release = time_get();
					}
					
				case SDL_MOUSEBUTTONDOWN:
					if(event.button.button == 1) key = KEY_MOUSE_1;
					if(event.button.button == 3) key = KEY_MOUSE_2;
					if(event.button.button == 2) key = KEY_MOUSE_3;
					if(event.button.button == 4) key = KEY_MOUSE_WHEEL_UP;
					if(event.button.button == 5) key = KEY_MOUSE_WHEEL_DOWN;
					break;
					
				/* other messages */
				case SDL_QUIT:
					exit(0);
			}
			
			/* */
			if(key != -1)
			{
				input_count[input_current^1][key].presses++;
				input_state[input_current^1][key] = 1;
				add_event(0, key, action);
			}

		}
	}
}
