/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_SERVER_H
#define ENGINE_IF_SERVER_H

/*
	Section: Server Hooks
*/

/* server */
/*
	Function: server_getclientinfo
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_getclientinfo(int client_id, CLIENT_INFO *info);

/*
	Function: server_clientname
		TODO	
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *server_clientname(int client_id);

/* grabs the latest input for the client. not withholding anything */

/*
	Function: server_latestinput
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int *server_latestinput(int client_id, int *size);

/*
	Function: server_setclientname
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_setclientname(int client_id, const char *name);

/*
	Function: server_setclientscore
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_setclientscore(int client_id, int score);

/*
	Function: server_setbrowseinfo
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_setbrowseinfo(int game_type, int progression);

/*
	Function: server_kick
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_kick(int client_id, const char *reason);

/*
	Function: server_tick
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_tick();

/*
	Function: server_tickspeed
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_tickspeed();

#endif
