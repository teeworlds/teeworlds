/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_PROTOCOL_H
#define ENGINE_SHARED_PROTOCOL_H

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
	
	// the first thing sent by the client
	// contains the version info for the client
	NETMSG_INFO=1,
	
	// sent by server
	NETMSG_MAP_CHANGE,		// sent when client should switch map
	NETMSG_MAP_DATA,		// map transfer, contains a chunk of the map file
	NETMSG_SNAP,			// normal snapshot, multiple parts
	NETMSG_SNAPEMPTY,		// empty snapshot
	NETMSG_SNAPSINGLE,		// ?
	NETMSG_SNAPSMALL,		//
	NETMSG_INPUTTIMING,		// reports how off the input was
	NETMSG_RCON_AUTH_STATUS,// result of the authentication
	NETMSG_RCON_LINE,		// line that should be printed to the remote console

	NETMSG_AUTH_CHALLANGE,	//
	NETMSG_AUTH_RESULT,		//
	
	// sent by client
	NETMSG_READY,			//
	NETMSG_ENTERGAME,
	NETMSG_INPUT,			// contains the inputdata from the client
	NETMSG_RCON_CMD,		// 
	NETMSG_RCON_AUTH,		//
	NETMSG_REQUEST_MAP_DATA,//

	NETMSG_AUTH_START,		//
	NETMSG_AUTH_RESPONSE,	//
	
	// sent by both
	NETMSG_PING,
	NETMSG_PING_REPLY,
	NETMSG_ERROR
};

// this should be revised
enum
{
	SERVER_TICK_SPEED=50,
	//SERVER_FLAG_PASSWORD = 0x1,

	SERVER_FLAG_PASSWORD			=	0x1,		// A
	SERVER_FLAG_TEAMS				=	0x100,		// B
	SERVER_FLAG_STRICTTEAMS			=	0x200,		// C
	SERVER_FLAG_CHEATS				=	0x1000,		// D
	SERVER_FLAG_PAUSE				=	0x2000,		// E
	SERVER_FLAG_ENDLESSSUPERHOOKING	=	0x4000,		// F
	SERVER_FLAG_TIMERCOMMANDS		=	0x10000,	// G
	SERVER_FLAG_TIMECHEAT			=	0x20000,	// H
	SERVER_FLAG_PAUSETIME			=	0x40000,	// I
	SERVER_FLAG_PLAYERCOLLISION		=	0x100000,	// K
	SERVER_FLAG_PLAYERHOOKING		=	0x200000,	// L
	SERVER_FLAG_ENDLESSHOOKING		=	0x400000,	// M
	SERVER_FLAG_HIT					=	0x800000,	// N
	SERVER_FLAG_MAPTEST				=	0x1000000,	// O
	SERVER_FLAG_SERVERTEST			=	0x2000000,	// P

	SERVER_FLAG_VERSION			= 0x10000000,

	SERVER_FLAGS_DEFAULT	=	0x1,
	SERVER_FLAGS_TEAMS		=	0x30,
	SERVER_FLAGS_CHEATS		=	0x700,
	SERVER_FLAGS_TIME		=	0x7000,
	SERVER_FLAGS_GAMEPLAY	=	0xF0000,
	SERVER_FLAGS_TESTING	=	0x300000,
	SERVER_FLAGS_VERSION	=	0xF000000,

	SERVER_FLAGS_ALL	  = 0xF3F7731,

	// version	| testing	| gameplay	| time | cheats | teams | unused	| default
	// 0001		| 00PO		| NMLK		| 0IHG | 0FED	| 00CB	| 0000		| 000A	- 0 means unused, letter means used

	MAX_CLIENTS=16,

	MAX_INPUT_SIZE=128,
	MAX_SNAPSHOT_PACKSIZE=900,

	MAX_CLANNAME_LENGTH=32,
	MAX_NAME_LENGTH=24,
	

	// message packing
	MSGFLAG_VITAL=1,
	MSGFLAG_FLUSH=2,
	MSGFLAG_NORECORD=4,
	MSGFLAG_RECORD=8,
	MSGFLAG_NOSEND=16
};

#endif
