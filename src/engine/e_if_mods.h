/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_MOD_H
#define ENGINE_IF_MOD_H

/**********************************************************************************
	Section: Server Hooks
**********************************************************************************/
/*
	Function: mods_console_init
		TODO
*/
void mods_console_init();

/*
	Function: mods_init
		Called when the server is started.
	
	Remarks:
		It's called after the map is loaded so all map items are available.
*/
void mods_init();

/*
	Function: mods_shutdown
		Called when the server quits.
	
	Remarks:
		Should be used to clean up all resources used.
*/
void mods_shutdown();

/*
	Function: mods_client_enter
		Called when a client has joined the game.
		
	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
	
	Remarks:
		It's called when the client is finished loading and should enter gameplay.
*/
void mods_client_enter(int cid);

/*
	Function: mods_client_drop
		Called when a client drops from the server.

	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS
*/
void mods_client_drop(int cid);

/*
	Function: mods_client_direct_input
		Called when the server recives new input from a client.

	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
		input - Pointer to the input data.
		size - Size of the data. (NOT IMPLEMENTED YET)
*/
void mods_client_direct_input(int cid, void *input);

/*
	Function: mods_client_predicted_input
		Called when the server applys the predicted input on the client.

	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
		input - Pointer to the input data.
		size - Size of the data. (NOT IMPLEMENTED YET)
*/
void mods_client_predicted_input(int cid, void *input);


/*
	Function: mods_tick
		Called with a regular interval to progress the gameplay.
		
	Remarks:
		The SERVER_TICK_SPEED tells the number of ticks per second.
*/
void mods_tick();

/*
	Function: mods_presnap
		Called before the server starts to construct snapshots for the clients.
*/
void mods_presnap();

/*
	Function: mods_snap
		Called to create the snapshot for a client.
	
	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
	
	Remarks:
		The game should make a series of calls to <snap_new_item> to construct
		the snapshot for the client.
*/
void mods_snap(int cid);

/*
	Function: mods_postsnap
		Called after the server is done sending the snapshots.
*/
void mods_postsnap();


/*
	Function: mods_connected
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void mods_connected(int client_id);


/*
	Function: mods_net_version
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *mods_net_version();

/*
	Function: mods_version
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *mods_version();

/*
	Function: mods_message
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void mods_message(int msg, int client_id);

#endif
