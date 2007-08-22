#include <engine/external/glfw/include/GL/glfw.h>

#include <engine/system.h>
#include <engine/interface.h>

static int keyboard_state[2][1024]; // TODO: fix this!!
static int keyboard_current = 0;
static int keyboard_first = 1;

void inp_mouse_relative(int *x, int *y)
{
	static int last_x = 0, last_y = 0;
	int nx, ny;
	glfwGetMousePos(&nx, &ny);
	*x = nx-last_x;
	*y = ny-last_y;
	last_x = nx;
	last_y = ny;
}

static char last_c = 0;
static int last_k = 0;

static void char_callback(int character, int action)
{
	if(action == GLFW_PRESS && character < 256)
	last_c = (char)character;
}

static void key_callback(int key, int action)
{
    if(action == GLFW_PRESS)
	    last_k = key;
}

void inp_init()
{
	glfwEnable(GLFW_KEY_REPEAT);
	glfwSetCharCallback(char_callback);
	glfwSetKeyCallback(key_callback);
}

char inp_last_char()
{
	return last_c;
}

int inp_last_key()
{
	return last_k;
}

void inp_clear()
{
	last_k = 0;
	last_c = 0;
}

void inp_mouse_mode_absolute()
{
	glfwEnable(GLFW_MOUSE_CURSOR);
}

void inp_mouse_mode_relative()
{
	glfwDisable(GLFW_MOUSE_CURSOR);
}

//int inp_mouse_scroll() { return input::mouse_scroll(); }
int inp_key_pressed(int key) { return keyboard_state[keyboard_current][key]; }
int inp_key_was_pressed(int key) { return keyboard_state[keyboard_current^1][key]; }
int inp_key_down(int key) { return inp_key_pressed(key)&&!inp_key_was_pressed(key); }
int inp_button_pressed(int button) { return keyboard_state[keyboard_current][button]; }

void inp_update()
{
    if(keyboard_first)
    {
        // make sure to reset
        keyboard_first = 0;
        inp_update();
    }

    keyboard_current = keyboard_current^1;
    int i, v;
    for(i = 0; i < KEY_LAST; i++)
    {
	    if (i >= KEY_MOUSE_FIRST)
			v = glfwGetMouseButton(i-KEY_MOUSE_FIRST) == GLFW_PRESS ? 1 : 0;
		else
			v = glfwGetKey(i) == GLFW_PRESS ? 1 : 0;
        keyboard_state[keyboard_current][i] = v;
    }

}
