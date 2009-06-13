/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_INP_H
#define ENGINE_IF_INP_H

/*
	Section: Input
*/


/*
	Structure: INPUT_EVENT
*/
enum
{
	INPFLAG_PRESS=1,
	INPFLAG_RELEASE=2,
	INPFLAG_REPEAT=4
};

typedef struct
{
	int flags;
	int unicode;
	int key;
} INPUT_EVENT;

/*
	Function: inp_mouse_relative
		Fetches the mouse movements.
		
	Arguments:
		x - Pointer to the variable that should get the X movement.
		y - Pointer to the variable that should get the Y movement.
*/
void inp_mouse_relative(int *x, int *y);

/*
	Function: inp_mouse_scroll
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_mouse_scroll();

/*
	Function: inp_key_pressed
		Checks if a key is pressed.
		
	Arguments:
		key - Index to the key to check
		
	Returns:
		Returns 1 if the button is pressed, otherwise 0.
	
	Remarks:
		Check keys.h for the keys.
*/
int inp_key_pressed(int key);


/* input */
/*
	Function: inp_key_was_pressed
		TODO	
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_was_pressed(int key);

/*
	Function: inp_key_down
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_down(int key);


/*
	Function: inp_num_events
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_num_events();

/*
	Function: inp_get_event
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
INPUT_EVENT inp_get_event(int index);

/*
	Function: inp_clear_events
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void inp_clear_events();

/*
	Function: inp_mouse_doubleclick
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_mouse_doubleclick();

/*
	Function: inp_key_presses
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_presses(int key);

/*
	Function: inp_key_releases
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_releases(int key);

/*
	Function: inp_key_state
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_state(int key);

/*
	Function: inp_key_name
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *inp_key_name(int k);

/*
	Function: inp_key_code
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_code(const char *key_name);



/*
	Function: inp_clear_key_states
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void inp_clear_key_states(); 

void inp_update();
void inp_init();
void inp_mouse_mode_absolute();
void inp_mouse_mode_relative();

#endif
