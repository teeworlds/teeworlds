#include "system.h"

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
	MAX_SNAPSHOT_PACKSIZE=1200
};
