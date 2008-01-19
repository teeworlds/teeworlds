/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_MODC_H
#define ENGINE_IF_MODC_H

/**********************************************************************************
	Section: Client Hooks
*********************************************************************************/
/*
	Function: modc_preinit
		TODO
*/
void modc_preinit();

/*
	Function: modc_init
		Called when the client starts.
	
	Remarks:
		The game should load resources that are used during the entire
		time of the game. No map is loaded.
*/
void modc_init();

/*
	Function: modc_newsnapshot
		Called when the client progressed to a new snapshot.
	
	Remarks:
		The client can check for items in the snapshot and perform one time
		events like playing sounds, spawning client side effects etc.
*/
void modc_newsnapshot();

/*
	Function: modc_entergame
		Called when the client has successfully connect to a server and
		loaded a map.
	
	Remarks:
		The client can check for items in the map and load them.
*/
void modc_entergame();

/*
	Function: modc_shutdown
		Called when the client closes down.
*/
void modc_shutdown();

/*
	Function: modc_render
		Called every frame to let the game render it self.
*/
void modc_render();

/*
	Function: modc_statechange
		Called every time client changes state.
*/
void modc_statechange(int new_state, int old_state);

/* undocumented callbacks */
/*
	Function: modc_connected
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void modc_connected();

/*
	Function: modc_message
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void modc_message(int msg);

/*
	Function: modc_predict
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void modc_predict();

/*
	Function: modc_snap_input
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int modc_snap_input(int *data);

/*
	Function: modc_net_version
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *modc_net_version();

#endif
