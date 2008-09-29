/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_CLIENT_H
#define ENGINE_IF_CLIENT_H

/*
	Title: Client Interface
*/

/*
	Section: Constants
*/

enum
{
	/* Constants: Client States
		CLIENTSTATE_OFFLINE - The client is offline.
		CLIENTSTATE_CONNECTING - The client is trying to connect to a server.
		CLIENTSTATE_LOADING - The client has connected to a server and is loading resources.
		CLIENTSTATE_ONLINE - The client is connected to a server and running the game.
		CLIENTSTATE_QUITING - The client is quiting.
	*/
	CLIENTSTATE_OFFLINE=0,
	CLIENTSTATE_CONNECTING,
	CLIENTSTATE_LOADING,
	CLIENTSTATE_ONLINE,
	CLIENTSTATE_QUITING,

	/* Constants: Image Formats
		IMG_AUTO - Lets the engine choose the format.
		IMG_RGB - 8-Bit uncompressed RGB
		IMG_RGBA - 8-Bit uncompressed RGBA
		IMG_ALPHA - 8-Bit uncompressed alpha
	*/
	IMG_AUTO=-1,
	IMG_RGB=0,
	IMG_RGBA=1,
	IMG_ALPHA=2,
	
	/* Constants: Texture Loading Flags
		TEXLOAD_NORESAMPLE - Prevents the texture from any resampling
	*/
	TEXLOAD_NORESAMPLE=1,
	
	/* Constants: Server Browser Sorting
		BROWSESORT_NAME - Sort by name.
		BROWSESORT_PING - Sort by ping.
		BROWSESORT_MAP - Sort by map
		BROWSESORT_GAMETYPE - Sort by game type. DM, TDM etc.
		BROWSESORT_PROGRESSION - Sort by progression.
		BROWSESORT_NUMPLAYERS - Sort after how many players there are on the server.
	*/
	BROWSESORT_NAME = 0,
	BROWSESORT_PING,
	BROWSESORT_MAP,
	BROWSESORT_GAMETYPE,
	BROWSESORT_PROGRESSION,
	BROWSESORT_NUMPLAYERS,
	
	BROWSEQUICK_SERVERNAME=1,
	BROWSEQUICK_PLAYERNAME=2,
	BROWSEQUICK_MAPNAME=4,
	
	BROWSETYPE_INTERNET = 0,
	BROWSETYPE_LAN = 1,
	BROWSETYPE_FAVORITES = 2
};

/*
	Section: Structures
*/

/*
	Structure: SERVER_INFO_PLAYER
*/
typedef struct
{
	char name[48];
	int score;
} SERVER_INFO_PLAYER;

/*
	Structure: SERVER_INFO
*/
typedef struct
{
	int sorted_index;
	int server_index;
	
	NETADDR netaddr;
	
	int quicksearch_hit;
	
	int progression;
	int max_players;
	int num_players;
	int flags;
	int favorite;
	int latency; /* in ms */
	char gametype[16];
	char name[64];
	char map[32];
	char version[32];
	char address[24];
	SERVER_INFO_PLAYER players[16];
} SERVER_INFO;

/*
	Section: Functions
*/

/**********************************************************************************
	Group: Time
**********************************************************************************/
/*
	Function: client_tick
		Returns the tick of the current snapshot.
*/
int client_tick();

/*
	Function: client_prevtick
		Returns the tick of the previous snapshot.
*/
int client_prevtick();

/*
	Function: client_intratick
		Returns the current intratick.
	
	Remarks:
		The intratick is how far gone the time is from the previous snapshot to the current.
		0.0 means that it on the previous snapshot. 0.5 means that it's halfway to the current,
		and 1.0 means that it is on the current snapshot. It can go beyond 1.0 which means that
		the client has started to extrapolate due to lack of data from the server.

	See Also:
		<client_tick>
*/
float client_intratick();

/*
	Function: client_predtick
		Returns the current predicted tick.
*/
int client_predtick();

/*
	Function: client_predintratick
		Returns the current preticted intra tick.
	
	Remarks:
		This is the same as <client_intratick> but for the current predicted tick and
		previous predicted tick.

	See Also:
		<client_intratick>
*/
float client_predintratick();

/*
	Function: client_ticktime
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float client_ticktime();

/*
	Function: client_tickspeed
		Returns how many ticks per second the client is doing.
	
	Remarks:
		This will be the same as the server tick speed.
*/
int client_tickspeed();

/*
	Function: client_frametime
		Returns how long time the last frame took to process.
*/
float client_frametime();

/*
	Function: client_localtime
		Returns the clients local time.
	
	Remarks:
		The local time is set to 0 when the client starts and when
		it connects to a server. Can be used for client side effects.
*/
float client_localtime();

/**********************************************************************************
	Group: Server Browser
**********************************************************************************/

/*
	Function: client_serverbrowse_refresh
		Issues a refresh of the server browser.
	
	Arguments:
		type - What type of servers to browse, internet, lan or favorites.
	
	Remarks:
		This will cause a broadcast on the local network if the lan argument is set.
		Otherwise it call ask all the master servers for their servers lists.
*/
void client_serverbrowse_refresh(int type);

/*
	Function: client_serverbrowse_sorted_get
		Returns server info from the sorted list.
	
	Arguments:
		index - Zero based index into the sorted list.
	
	See Also:
		<client_serverbrowse_sorted_num>
*/
SERVER_INFO *client_serverbrowse_sorted_get(int index);

/*
	Function: client_serverbrowse_sorted_num
		Returns how many servers there are in the sorted list.
	
	See Also:
		<client_serverbrowse_sorted_get>
*/
int client_serverbrowse_sorted_num();

/*
	Function: client_serverbrowse_get
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
SERVER_INFO *client_serverbrowse_get(int index);

/*
	Function: client_serverbrowse_num
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_serverbrowse_num();

/*
	Function: client_serverbrowse_num_requests
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_serverbrowse_num_requests();

/*
	Function: client_serverbrowse_update
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_serverbrowse_update();

/*
	Function: client_serverbrowse_lan
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_serverbrowse_lan();

/*
	Function: client_serverbrowse_addfavorite
		Adds a server to the favorite list
	
	Arguments:
		addr - Address of the favorite server.
*/
void client_serverbrowse_addfavorite(NETADDR addr);

/*
	Function: client_serverbrowse_removefavorite
		Removes a server to the favorite list
	
	Arguments:
		addr - Address of the favorite server.
*/
void client_serverbrowse_removefavorite(NETADDR addr);

/**********************************************************************************
	Group: Actions
**********************************************************************************/


/*
	Function: client_connect
		Connects to a server at the specified address.
	
	Arguments:
		address - Address of the server to connect to.
	
	See Also:
		<client_disconnect>
*/
void client_connect(const char *address);

/*
	Function: client_disconnect
		Disconnects from the current server.
	
	Remarks:
		Does nothing if not connected to a server.

	See Also:
		<client_connect, client_quit>
*/
void client_disconnect();

/*
	Function: client_quit
		Tells to client to shutdown.

	See Also:
		<client_disconnect>
*/
void client_quit();

void client_entergame();

/*
	Function: client_rcon
		Sends a command to the server to execute on it's console.
	
	Arguments:
		cmd - The command to send.
	
	Remarks:
		The client must have the correct rcon password to connect.

 	See Also:
		<client_rcon_auth, client_rcon_authed>
*/
void client_rcon(const char *cmd);

/*
	Function: client_rcon_auth
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<client_rcon, client_rcon_authed>
*/
void client_rcon_auth(const char *name, const char *password);

/*
	Function: client_rcon_authed
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<client_rcon, client_rcon_auth>
*/
int client_rcon_authed();

/**********************************************************************************
	Group: Other
**********************************************************************************/
/*
	Function: client_latestversion
		Returns 0 if there's no version difference
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *client_latestversion();

/*
	Function: client_get_input
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int *client_get_input(int tick);

/*
	Function: client_direct_input
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_direct_input(int *input, int size);

/*
	Function: client_error_string
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *client_error_string();

/*
	Function: client_connection_problems
		Returns 1 if the client is connection problems.
	
	Remarks:
		Connections problems usually means that the client havn't recvived any data
		from the server in a while.
*/
int client_connection_problems();

/*
	Function: client_state
		Returns the state of the client.
		
	See Also:
		<Client States>
*/
int client_state();

/*
	Function: client_mapdownload_amount
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_mapdownload_amount();

/*
	Function: client_mapdownload_totalsize
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_mapdownload_totalsize();

/*
	Function: client_save_line
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_save_line(const char *line);






enum
{
	BROWSESET_MASTER_ADD=1,
	BROWSESET_FAV_ADD,
	BROWSESET_TOKEN,
	BROWSESET_OLD_INTERNET,
	BROWSESET_OLD_LAN
};

void client_serverbrowse_set(NETADDR *addr, int type, int token, SERVER_INFO *info);


int client_serverbrowse_refreshingmasters();
#endif
