/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>

/*
	Connection diagram - How the initilization works.
	
	Client -> INFO -> Server
		Contains version info, name, and some other info.
		
	Client <- MAP <- Server
		Contains current map.
	
	Client -> READY -> Server
		The client has loaded the map and is ready to go,
		but the mod needs to send it's information aswell.
		modc_connected is called on the client and
		mods_connected is called on the server.
		The client should call client_entergame when the
		mod has done it's initilization.
		
	Client -> ENTERGAME -> Server
		Tells the server to start sending snapshots.
		client_entergame and server_client_enter is called.
*/


enum
{
	NETMSG_NULL=0,
	
	/* the first thing sent by the client
	contains the version info for the client */
	NETMSG_INFO=1,
	
	/* sent by server */
	NETMSG_MAP_CHANGE,		/* sent when client should switch map */
	NETMSG_MAP_DATA,		/* map transfer, contains a chunk of the map file */
	NETMSG_SNAP,			/* normal snapshot, multiple parts */
	NETMSG_SNAPEMPTY,		/* empty snapshot */
	NETMSG_SNAPSINGLE,		/* ? */
	NETMSG_SNAPSMALL,		/* */
	NETMSG_INPUTTIMING,		/* reports how off the input was */
	NETMSG_RCON_AUTH_STATUS,/* result of the authentication */
	NETMSG_RCON_LINE,		/* line that should be printed to the remote console */

	NETMSG_AUTH_CHALLANGE,	/* */
	NETMSG_AUTH_RESULT,		/* */
	
	/* sent by client */
	NETMSG_READY,			/* */
	NETMSG_ENTERGAME,
	NETMSG_INPUT,			/* contains the inputdata from the client */
	NETMSG_RCON_CMD,		/* */ 
	NETMSG_RCON_AUTH,		/* */
	NETMSG_REQUEST_MAP_DATA,/* */

	NETMSG_AUTH_START,		/* */
	NETMSG_AUTH_RESPONSE,	/* */
	
	/* sent by both */
	NETMSG_PING,
	NETMSG_PING_REPLY,
	NETMSG_ERROR
};


/* this should be revised */
enum
{
	MAX_CLANNAME_LENGTH=32,
	MAX_INPUT_SIZE=128,
	MAX_SNAPSHOT_PACKSIZE=900
};
