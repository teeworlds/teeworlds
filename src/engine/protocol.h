#include "system.h"

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
	NETMSG_MAP,
	NETMSG_SNAP,
	NETMSG_SNAPEMPTY,
	NETMSG_SNAPSMALL,
	
	/* sent by client */
	NETMSG_READY,
	NETMSG_ENTERGAME,
	NETMSG_INPUT,
	NETMSG_CMD,
	
	/* sent by both */
	NETMSG_ERROR
};


/* this should be revised */
enum
{
	MAX_NAME_LENGTH=32,
	MAX_CLANNAME_LENGTH=32,
	MAX_INPUT_SIZE=128,
	MAX_SNAPSHOT_PACKSIZE=900
};
