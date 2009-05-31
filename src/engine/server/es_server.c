/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <base/system.h>

#include <engine/e_config.h>
#include <engine/e_engine.h>
#include <engine/e_server_interface.h>

#include <engine/e_protocol.h>
#include <engine/e_snapshot.h>

#include <engine/e_compression.h>

#include <engine/e_network.h>
#include <engine/e_config.h>
#include <engine/e_packer.h>
#include <engine/e_datafile.h>
#include <engine/e_demorec.h>

#include <mastersrv/mastersrv.h>

#if defined(CONF_FAMILY_WINDOWS) 
	#define _WIN32_WINNT 0x0500 
	#include <windows.h> 
#endif 

static SNAPBUILD builder;

static int64 game_start_time;
static int current_tick = 0;
static int run_server = 1;

static char browseinfo_gametype[16] = {0};
static int browseinfo_progression = -1;

static int64 lastheartbeat;
/*static NETADDR4 master_server;*/

static char current_map[64];
static int current_map_crc;
static unsigned char *current_map_data = 0;
static int current_map_size = 0;

void *snap_new_item(int type, int id, int size)
{
	dbg_assert(type >= 0 && type <=0xffff, "incorrect type");
	dbg_assert(id >= 0 && id <=0xffff, "incorrect id");
	return snapbuild_new_item(&builder, type, id, size);
}

typedef struct
{
	short next;
	short state; /* 0 = free, 1 = alloced, 2 = timed */
	int timeout;
} SNAP_ID;

static const int MAX_IDS = 16*1024; /* should be lowered */
static SNAP_ID snap_ids[16*1024];
static int snap_first_free_id;
static int snap_first_timed_id;
static int snap_last_timed_id;
static int snap_id_usage;
static int snap_id_inusage;
static int snap_id_inited = 0;

enum
{
	SRVCLIENT_STATE_EMPTY = 0,
	SRVCLIENT_STATE_AUTH,
	SRVCLIENT_STATE_CONNECTING,
	SRVCLIENT_STATE_READY,
	SRVCLIENT_STATE_INGAME,
	
	SRVCLIENT_SNAPRATE_INIT=0,
	SRVCLIENT_SNAPRATE_FULL,
	SRVCLIENT_SNAPRATE_RECOVER
};

typedef struct 
{
	int data[MAX_INPUT_SIZE];
	int game_tick; /* the tick that was chosen for the input */
} CLIENT_INPUT;
	
/* */
typedef struct
{
	/* connection state info */
	int state;
	int latency;
	int snap_rate;
	
	int last_acked_snapshot;
	int last_input_tick;
	SNAPSTORAGE snapshots;
	
	CLIENT_INPUT latestinput;
	CLIENT_INPUT inputs[200]; /* TODO: handle input better */
	int current_input;
	
	char name[MAX_NAME_LENGTH];
	char clan[MAX_CLANNAME_LENGTH];
	int score;
	int authed;
} CLIENT;

static CLIENT clients[MAX_CLIENTS];
NETSERVER *net;

static void snap_init_id()
{
	int i;
	if(snap_id_inited)
		return;
	
	for(i = 0; i < MAX_IDS; i++)
	{
		snap_ids[i].next = i+1;
		snap_ids[i].state = 0;
	}
		
	snap_ids[MAX_IDS-1].next = -1;
	snap_first_free_id = 0;
	snap_first_timed_id = -1;
	snap_last_timed_id = -1;
	snap_id_usage = 0;
	snap_id_inusage = 0;
	
	snap_id_inited = 1;
}

static void snap_remove_first_timeout()
{
	int next_timed = snap_ids[snap_first_timed_id].next;
	
	/* add it to the free list */
	snap_ids[snap_first_timed_id].next = snap_first_free_id;
	snap_ids[snap_first_timed_id].state = 0;
	snap_first_free_id = snap_first_timed_id;
	
	/* remove it from the timed list */
	snap_first_timed_id = next_timed;
	if(snap_first_timed_id == -1)
		snap_last_timed_id = -1;
		
	snap_id_usage--;
}

int snap_new_id()
{
	int id;
	int64 now = time_get();
	if(!snap_id_inited)
		snap_init_id();
	
	/* process timed ids */
	while(snap_first_timed_id != -1 && snap_ids[snap_first_timed_id].timeout < now)
		snap_remove_first_timeout();
	
	id = snap_first_free_id;
	dbg_assert(id != -1, "id error");
	snap_first_free_id = snap_ids[snap_first_free_id].next;
	snap_ids[id].state = 1;
	snap_id_usage++;
	snap_id_inusage++;
	return id;
}

void snap_timeout_ids()
{
	/* process timed ids */
	while(snap_first_timed_id != -1)
		snap_remove_first_timeout();
}

void snap_free_id(int id)
{
	dbg_assert(snap_ids[id].state == 1, "id is not alloced");

	snap_id_inusage--;
	snap_ids[id].state = 2;
	snap_ids[id].timeout = time_get()+time_freq()*5;
	snap_ids[id].next = -1;
	
	if(snap_last_timed_id != -1)
	{
		snap_ids[snap_last_timed_id].next = id;
		snap_last_timed_id = id;
	}
	else
	{
		snap_first_timed_id = id;
		snap_last_timed_id = id;
	}
}

int *server_latestinput(int client_id, int *size)
{
	if(client_id < 0 || client_id > MAX_CLIENTS || clients[client_id].state < SRVCLIENT_STATE_READY)
		return 0;
	return clients[client_id].latestinput.data;
}

const char *server_clientname(int client_id)
{
	if(client_id < 0 || client_id > MAX_CLIENTS || clients[client_id].state < SRVCLIENT_STATE_READY)
		return "(invalid client)";
	return clients[client_id].name;
}

static const char *str_ltrim(const char *str)
{
	while(*str && *str <= 32)
		str++;
	return str;
}

static void str_rtrim(char *str)
{
	int i = str_length(str);
	while(i >= 0)
	{
		if(str[i] > 32)
			break;
		str[i] = 0;
		i--;
	}
}

static int server_try_setclientname(int client_id, const char *name)
{
	int i;
	char trimmed_name[64];

	/* trim the name */
	str_copy(trimmed_name, str_ltrim(name), sizeof(trimmed_name));
	str_rtrim(trimmed_name);
	dbg_msg("", "'%s' -> '%s'", name, trimmed_name);
	name = trimmed_name;
	
	
	/* check for empty names */
	if(!name[0])
		return -1;
	
	/* make sure that two clients doesn't have the same name */
	for(i = 0; i < MAX_CLIENTS; i++)
		if(i != client_id && clients[i].state >= SRVCLIENT_STATE_READY)
		{
			if(strcmp(name, clients[i].name) == 0)
				return -1;
		}

	/* set the client name */
	str_copy(clients[client_id].name, name, MAX_NAME_LENGTH);
	return 0;
}

void server_setclientname(int client_id, const char *name)
{
	char nametry[MAX_NAME_LENGTH];
	int i;
	if(client_id < 0 || client_id > MAX_CLIENTS || clients[client_id].state < SRVCLIENT_STATE_READY)
		return;
		
	if(!name)
		return;
		
	str_copy(nametry, name, MAX_NAME_LENGTH);
	if(server_try_setclientname(client_id, nametry))
	{
		/* auto rename */
		for(i = 1;; i++)
		{
			str_format(nametry, MAX_NAME_LENGTH, "(%d)%s", i, name);
			if(server_try_setclientname(client_id, nametry) == 0)
				break;
		}
	}
}

void server_setclientscore(int client_id, int score)
{
	if(client_id < 0 || client_id > MAX_CLIENTS || clients[client_id].state < SRVCLIENT_STATE_READY)
		return;
	clients[client_id].score = score;
}

void server_setbrowseinfo(const char *game_type, int progression)
{
	str_copy(browseinfo_gametype, game_type, sizeof(browseinfo_gametype));
	browseinfo_progression = progression;
	if(browseinfo_progression > 100)
		browseinfo_progression = 100;
	if(browseinfo_progression < -1)
		browseinfo_progression = -1;
}

void server_kick(int client_id, const char *reason)
{
	if(client_id < 0 || client_id > MAX_CLIENTS)
		return;
		
	if(clients[client_id].state != SRVCLIENT_STATE_EMPTY)
		netserver_drop(net, client_id, reason);
}

int server_tick()
{
	return current_tick;
}

int64 server_tick_start_time(int tick)
{
	return game_start_time + (time_freq()*tick)/SERVER_TICK_SPEED;
}

int server_tickspeed()
{
	return SERVER_TICK_SPEED;
}

int server_init()
{
	int i;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i].state = SRVCLIENT_STATE_EMPTY;
		clients[i].name[0] = 0;
		clients[i].clan[0] = 0;
		snapstorage_init(&clients[i].snapshots);
	}

	current_tick = 0;

	return 0;
}

int server_getclientinfo(int client_id, CLIENT_INFO *info)
{
	dbg_assert(client_id >= 0 && client_id < MAX_CLIENTS, "client_id is not valid");
	dbg_assert(info != 0, "info can not be null");

	if(clients[client_id].state == SRVCLIENT_STATE_INGAME)
	{
		info->name = clients[client_id].name;
		info->latency = clients[client_id].latency;
		return 1;
	}
	return 0;
}

int server_send_msg(int client_id)
{
	const MSG_INFO *info = msg_get_info();
	NETCHUNK packet;
	if(!info)
		return -1;
		
	mem_zero(&packet, sizeof(NETCHUNK));
	
	packet.client_id = client_id;
	packet.data = info->data;
	packet.data_size = info->size;

	if(info->flags&MSGFLAG_VITAL)
		packet.flags |= NETSENDFLAG_VITAL;
	if(info->flags&MSGFLAG_FLUSH)
		packet.flags |= NETSENDFLAG_FLUSH;
	
	/* write message to demo recorder */
	if(!(info->flags&MSGFLAG_NORECORD))
		demorec_record_message(info->data, info->size);

	if(!(info->flags&MSGFLAG_NOSEND))
	{
		if(client_id == -1)
		{
			/* broadcast */
			int i;
			for(i = 0; i < MAX_CLIENTS; i++)
				if(clients[i].state == SRVCLIENT_STATE_INGAME)
				{
					packet.client_id = i;
					netserver_send(net, &packet);
				}
		}
		else
			netserver_send(net, &packet);
	}
	return 0;
}

static void server_do_snap()
{
	int i;
	
	{
		static PERFORMACE_INFO scope = {"presnap", 0};
		perf_start(&scope);
		mods_presnap();
		perf_end();
	}
	
	/* create snapshot for demo recording */
	if(demorec_isrecording())
	{
		char data[MAX_SNAPSHOT_SIZE];
		int snapshot_size;

		/* build snap and possibly add some messages */
		snapbuild_init(&builder);
		mods_snap(-1);
		snapshot_size = snapbuild_finish(&builder, data);
		
		/* write snapshot */
		demorec_record_snapshot(server_tick(), data, snapshot_size);
	}

	/* create snapshots for all clients */
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		/* client must be ingame to recive snapshots */
		if(clients[i].state != SRVCLIENT_STATE_INGAME)
			continue;
			
		/* this client is trying to recover, don't spam snapshots */
		if(clients[i].snap_rate == SRVCLIENT_SNAPRATE_RECOVER && (server_tick()%50) != 0)
			continue;
			
		/* this client is trying to recover, don't spam snapshots */
		if(clients[i].snap_rate == SRVCLIENT_SNAPRATE_INIT && (server_tick()%10) != 0)
			continue;
			
		{
			char data[MAX_SNAPSHOT_SIZE];
			char deltadata[MAX_SNAPSHOT_SIZE];
			char compdata[MAX_SNAPSHOT_SIZE];
			int snapshot_size;
			int crc;
			static SNAPSHOT emptysnap;
			SNAPSHOT *deltashot = &emptysnap;
			int deltashot_size;
			int delta_tick = -1;
			int deltasize;
			static PERFORMACE_INFO scope = {"build", 0};
			perf_start(&scope);

			snapbuild_init(&builder);

			{
				static PERFORMACE_INFO scope = {"modsnap", 0};
				perf_start(&scope);
				mods_snap(i);
				perf_end();
			}

			/* finish snapshot */
			snapshot_size = snapbuild_finish(&builder, data);
			crc = snapshot_crc((SNAPSHOT*)data);

			/* remove old snapshos */
			/* keep 3 seconds worth of snapshots */
			snapstorage_purge_until(&clients[i].snapshots, current_tick-SERVER_TICK_SPEED*3);
			
			/* save it the snapshot */
			snapstorage_add(&clients[i].snapshots, current_tick, time_get(), snapshot_size, data, 0);
			
			/* find snapshot that we can preform delta against */
			emptysnap.data_size = 0;
			emptysnap.num_items = 0;
			
			{
				deltashot_size = snapstorage_get(&clients[i].snapshots, clients[i].last_acked_snapshot, 0, &deltashot, 0);
				if(deltashot_size >= 0)
					delta_tick = clients[i].last_acked_snapshot;
				else
				{
					/* no acked package found, force client to recover rate */
					if(clients[i].snap_rate == SRVCLIENT_SNAPRATE_FULL)
						clients[i].snap_rate = SRVCLIENT_SNAPRATE_RECOVER;
				}
			}
			
			/* create delta */
			{
				static PERFORMACE_INFO scope = {"delta", 0};
				perf_start(&scope);
				deltasize = snapshot_create_delta(deltashot, (SNAPSHOT*)data, deltadata);
				perf_end();
			}

			
			if(deltasize)
			{
				/* compress it */
				int snapshot_size;
				const int max_size = MAX_SNAPSHOT_PACKSIZE;
				int numpackets;
				int n, left;

				{				
					static PERFORMACE_INFO scope = {"compress", 0};
					/*char buffer[512];*/
					perf_start(&scope);
					snapshot_size = intpack_compress(deltadata, deltasize, compdata);
					
					/*str_hex(buffer, sizeof(buffer), compdata, snapshot_size);
					dbg_msg("", "deltasize=%d -> %d : %s", deltasize, snapshot_size, buffer);*/
					
					perf_end();
				}

				numpackets = (snapshot_size+max_size-1)/max_size;
				
				for(n = 0, left = snapshot_size; left; n++)
				{
					int chunk = left < max_size ? left : max_size;
					left -= chunk;

					if(numpackets == 1)
						msg_pack_start_system(NETMSG_SNAPSINGLE, MSGFLAG_FLUSH);
					else
						msg_pack_start_system(NETMSG_SNAP, MSGFLAG_FLUSH);
						
					msg_pack_int(current_tick);
					msg_pack_int(current_tick-delta_tick); /* compressed with */
					
					if(numpackets != 1)
					{
						msg_pack_int(numpackets);
						msg_pack_int(n);
					}
					
					msg_pack_int(crc);
					msg_pack_int(chunk);
					msg_pack_raw(&compdata[n*max_size], chunk);
					msg_pack_end();
					server_send_msg(i);
				}
			}
			else
			{
				msg_pack_start_system(NETMSG_SNAPEMPTY, MSGFLAG_FLUSH);
				msg_pack_int(current_tick);
				msg_pack_int(current_tick-delta_tick); /* compressed with */
				msg_pack_end();
				server_send_msg(i);
			}
			
			perf_end();
		}
	}

	mods_postsnap();
}


static void reset_client(int cid)
{
	/* reset input */
	int i;
	for(i = 0; i < 200; i++)
		clients[cid].inputs[i].game_tick = -1;
	clients[cid].current_input = 0;
	mem_zero(&clients[cid].latestinput, sizeof(clients[cid].latestinput));

	snapstorage_purge_all(&clients[cid].snapshots);
	clients[cid].last_acked_snapshot = -1;
	clients[cid].last_input_tick = -1;
	clients[cid].snap_rate = SRVCLIENT_SNAPRATE_INIT;
	clients[cid].score = 0;

}

static int new_client_callback(int cid, void *user)
{
	clients[cid].state = SRVCLIENT_STATE_AUTH;
	clients[cid].name[0] = 0;
	clients[cid].clan[0] = 0;
	clients[cid].authed = 0;
	reset_client(cid);
	return 0;
}

static int del_client_callback(int cid, void *user)
{
	/* notify the mod about the drop */
	if(clients[cid].state >= SRVCLIENT_STATE_READY)
		mods_client_drop(cid);
	
	clients[cid].state = SRVCLIENT_STATE_EMPTY;
	clients[cid].name[0] = 0;
	clients[cid].clan[0] = 0;
	clients[cid].authed = 0;
	snapstorage_purge_all(&clients[cid].snapshots);
	return 0;
}

static void server_send_map(int cid)
{
	msg_pack_start_system(NETMSG_MAP_CHANGE, MSGFLAG_VITAL|MSGFLAG_FLUSH);
	msg_pack_string(config.sv_map, 0);
	msg_pack_int(current_map_crc);
	msg_pack_end();
	server_send_msg(cid);
}

static void server_send_rcon_line(int cid, const char *line)
{
	msg_pack_start_system(NETMSG_RCON_LINE, MSGFLAG_VITAL);
	msg_pack_string(line, 512);
	msg_pack_end();
	server_send_msg(cid);
}

static void server_send_rcon_line_authed(const char *line, void *user_data)
{
	static volatile int reentry_guard = 0;
	int i;
	
	if(reentry_guard) return;
	reentry_guard++;
	
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i].state != SRVCLIENT_STATE_EMPTY && clients[i].authed)
			server_send_rcon_line(i, line);
	}
	
	reentry_guard--;
}

static void server_process_client_packet(NETCHUNK *packet)
{
	int cid = packet->client_id;
	NETADDR addr;
	
	int sys;
	int msg = msg_unpack_start(packet->data, packet->data_size, &sys);
	
	if(clients[cid].state == SRVCLIENT_STATE_AUTH)
	{
		if(sys && msg == NETMSG_INFO)
		{
			char version[64];
			const char *password;
			str_copy(version, msg_unpack_string(), 64);
			if(strcmp(version, mods_net_version()) != 0)
			{
				/* OH FUCK! wrong version, drop him */
				char reason[256];
				str_format(reason, sizeof(reason), "wrong version. server is running '%s' and client '%s'.", mods_net_version(), version);
				netserver_drop(net, cid, reason);
				return;
			}
			
			str_copy(clients[cid].name, msg_unpack_string(), MAX_NAME_LENGTH);
			str_copy(clients[cid].clan, msg_unpack_string(), MAX_CLANNAME_LENGTH);
			password = msg_unpack_string();
			
			if(config.password[0] != 0 && strcmp(config.password, password) != 0)
			{
				/* wrong password */
				netserver_drop(net, cid, "wrong password");
				return;
			}
			
			clients[cid].state = SRVCLIENT_STATE_CONNECTING;
			server_send_map(cid);
		}
	}
	else
	{
		if(sys)
		{
			/* system message */
			if(msg == NETMSG_REQUEST_MAP_DATA)
			{
				int chunk = msg_unpack_int();
				int chunk_size = 1024-128;
				int offset = chunk * chunk_size;
				int last = 0;
				
				/* drop faulty map data requests */
				if(chunk < 0 || offset > current_map_size)
					return;
				
				if(offset+chunk_size >= current_map_size)
				{
					chunk_size = current_map_size-offset;
					if(chunk_size < 0)
						chunk_size = 0;
					last = 1;
				}
				
				msg_pack_start_system(NETMSG_MAP_DATA, MSGFLAG_VITAL|MSGFLAG_FLUSH);
				msg_pack_int(last);
				msg_pack_int(current_map_size);
				msg_pack_int(chunk_size);
				msg_pack_raw(&current_map_data[offset], chunk_size);
				msg_pack_end();
				server_send_msg(cid);
				
				if(config.debug)
					dbg_msg("server", "sending chunk %d with size %d", chunk, chunk_size);
			}
			else if(msg == NETMSG_READY)
			{
				if(clients[cid].state == SRVCLIENT_STATE_CONNECTING)
				{
					netserver_client_addr(net, cid, &addr);
					
					dbg_msg("server", "player is ready. cid=%x ip=%d.%d.%d.%d",
						cid,
						addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]
						);
					clients[cid].state = SRVCLIENT_STATE_READY;
					mods_connected(cid);
				}
			}
			else if(msg == NETMSG_ENTERGAME)
			{
				if(clients[cid].state == SRVCLIENT_STATE_READY)
				{
					netserver_client_addr(net, cid, &addr);
					
					dbg_msg("server", "player has entered the game. cid=%x ip=%d.%d.%d.%d",
						cid,
						addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]
						);
					clients[cid].state = SRVCLIENT_STATE_INGAME;
					mods_client_enter(cid);
				}
			}
			else if(msg == NETMSG_INPUT)
			{
				int tick, size, i;
				CLIENT_INPUT *input;
				int64 tagtime;
				
				clients[cid].last_acked_snapshot = msg_unpack_int();
				tick = msg_unpack_int();
				size = msg_unpack_int();
				
				/* check for errors */
				if(msg_unpack_error() || size/4 > MAX_INPUT_SIZE)
					return;

				if(clients[cid].last_acked_snapshot > 0)
					clients[cid].snap_rate = SRVCLIENT_SNAPRATE_FULL;
					
				if(snapstorage_get(&clients[cid].snapshots, clients[cid].last_acked_snapshot, &tagtime, 0, 0) >= 0)
					clients[cid].latency = (int)(((time_get()-tagtime)*1000)/time_freq());

				/* add message to report the input timing */
				/* skip packets that are old */
				if(tick > clients[cid].last_input_tick)
				{
					int time_left = ((server_tick_start_time(tick)-time_get())*1000) / time_freq();
					msg_pack_start_system(NETMSG_INPUTTIMING, 0);
					msg_pack_int(tick);
					msg_pack_int(time_left);
					msg_pack_end();
					server_send_msg(cid);
				}

				clients[cid].last_input_tick = tick;

				input = &clients[cid].inputs[clients[cid].current_input];
				
				if(tick <= server_tick())
					tick = server_tick()+1;

				input->game_tick = tick;
				
				for(i = 0; i < size/4; i++)
					input->data[i] = msg_unpack_int();
				
				mem_copy(clients[cid].latestinput.data, input->data, MAX_INPUT_SIZE*sizeof(int));
				
				clients[cid].current_input++;
				clients[cid].current_input %= 200;
			
				/* call the mod with the fresh input data */
				if(clients[cid].state == SRVCLIENT_STATE_INGAME)
					mods_client_direct_input(cid, clients[cid].latestinput.data);
			}
			else if(msg == NETMSG_RCON_CMD)
			{
				const char *cmd = msg_unpack_string();
				
				if(msg_unpack_error() == 0 && clients[cid].authed)
				{
					dbg_msg("server", "cid=%d rcon='%s'", cid, cmd);
					console_execute_line(cmd);
				}
			}
			else if(msg == NETMSG_RCON_AUTH)
			{
				const char *pw;
				msg_unpack_string(); /* login name, not used */
				pw = msg_unpack_string();
				
				if(msg_unpack_error() == 0)
				{
					if(config.sv_rcon_password[0] == 0)
					{
						server_send_rcon_line(cid, "No rcon password set on server. Set sv_rcon_password to enable the remote console.");
					}
					else if(strcmp(pw, config.sv_rcon_password) == 0)
					{
						msg_pack_start_system(NETMSG_RCON_AUTH_STATUS, MSGFLAG_VITAL);
						msg_pack_int(1);
						msg_pack_end();
						server_send_msg(cid);
						
						clients[cid].authed = 1;
						server_send_rcon_line(cid, "Authentication successful. Remote console access granted.");
						dbg_msg("server", "cid=%d authed", cid);
					}
					else
					{
						server_send_rcon_line(cid, "Wrong password.");
					}
				}
			}
			else if(msg == NETMSG_PING)
			{
				msg_pack_start_system(NETMSG_PING_REPLY, 0);
				msg_pack_end();
				server_send_msg(cid);
			}
			else
			{
				char hex[] = "0123456789ABCDEF";
				char buf[512];
				int b;

				for(b = 0; b < packet->data_size && b < 32; b++)
				{
					buf[b*3] = hex[((const unsigned char *)packet->data)[b]>>4];
					buf[b*3+1] = hex[((const unsigned char *)packet->data)[b]&0xf];
					buf[b*3+2] = ' ';
					buf[b*3+3] = 0;
				}

				dbg_msg("server", "strange message cid=%d msg=%d data_size=%d", cid, msg, packet->data_size);
				dbg_msg("server", "%s", buf);
				
			}
		}
		else
		{
			/* game message */
			if(clients[cid].state >= SRVCLIENT_STATE_READY)
				mods_message(msg, cid);
		}
	}
}


int server_ban_add(NETADDR addr, int seconds)
{
	return netserver_ban_add(net, addr, seconds);	
}

int server_ban_remove(NETADDR addr)
{
	return netserver_ban_remove(net, addr);
}

static void server_send_serverinfo(NETADDR *addr, int token)
{
	NETCHUNK packet;
	PACKER p;
	char buf[128];

	/* count the players */	
	int player_count = 0;
	int i;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i].state != SRVCLIENT_STATE_EMPTY)
			player_count++;
	}
	
	packer_reset(&p);

	if(token >= 0)
	{
		/* new token based format */
		packer_add_raw(&p, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));
		str_format(buf, sizeof(buf), "%d", token);
		packer_add_string(&p, buf, 6);
	}
	else
	{
		/* old format */
		packer_add_raw(&p, SERVERBROWSE_OLD_INFO, sizeof(SERVERBROWSE_OLD_INFO));
	}
	
	packer_add_string(&p, mods_version(), 32);
	packer_add_string(&p, config.sv_name, 64);
	packer_add_string(&p, config.sv_map, 32);

	/* gametype */
	packer_add_string(&p, browseinfo_gametype, 16);

	/* flags */
	i = 0;
	if(config.password[0])   /* password set */
		i |= SRVFLAG_PASSWORD;
	str_format(buf, sizeof(buf), "%d", i);
	packer_add_string(&p, buf, 2);

	/* progression */
	str_format(buf, sizeof(buf), "%d", browseinfo_progression);
	packer_add_string(&p, buf, 4);
	
	str_format(buf, sizeof(buf), "%d", player_count); packer_add_string(&p, buf, 3);  /* num players */
	str_format(buf, sizeof(buf), "%d", netserver_max_clients(net)); packer_add_string(&p, buf, 3); /* max players */

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i].state != SRVCLIENT_STATE_EMPTY)
		{
			packer_add_string(&p, clients[i].name, 48);  /* player name */
			str_format(buf, sizeof(buf), "%d", clients[i].score); packer_add_string(&p, buf, 6);  /* player score */
		}
	}
	
	
	packet.client_id = -1;
	packet.address = *addr;
	packet.flags = NETSENDFLAG_CONNLESS;
	packet.data_size = packer_size(&p);
	packet.data = packer_data(&p);
	netserver_send(net, &packet);
}

extern int register_process_packet(NETCHUNK *packet);
extern int register_update();

static void server_pump_network()
{
	NETCHUNK packet;

	netserver_update(net);
	
	/* process packets */
	while(netserver_recv(net, &packet))
	{
		if(packet.client_id == -1)
		{
			/* stateless */
			if(!register_process_packet(&packet))
			{
				if(packet.data_size == sizeof(SERVERBROWSE_GETINFO)+1 &&
					memcmp(packet.data, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					server_send_serverinfo(&packet.address, ((unsigned char *)packet.data)[sizeof(SERVERBROWSE_GETINFO)]);
				}
				
				
				if(packet.data_size == sizeof(SERVERBROWSE_OLD_GETINFO) &&
					memcmp(packet.data, SERVERBROWSE_OLD_GETINFO, sizeof(SERVERBROWSE_OLD_GETINFO)) == 0)
				{
					server_send_serverinfo(&packet.address, -1);
				}
			}
		}
		else
			server_process_client_packet(&packet);
	}
}

static int server_load_map(const char *mapname)
{
	DATAFILE *df;
	char buf[512];
	str_format(buf, sizeof(buf), "maps/%s.map", mapname);
	df = datafile_load(buf);
	if(!df)
		return 0;
	
	/* reinit snapshot ids */
	snap_timeout_ids();
	
	/* get the crc of the map */
	current_map_crc = datafile_crc(buf);
	dbg_msg("server", "%s crc is %08x", buf, current_map_crc);
		
	str_copy(current_map, mapname, sizeof(current_map));
	map_set(df);
	
	/* load compelate map into memory for download */
	{
		IOHANDLE file = engine_openfile(buf, IOFLAG_READ);
		current_map_size = (int)io_length(file);
		if(current_map_data)
			mem_free(current_map_data);
		current_map_data = (unsigned char *)mem_alloc(current_map_size, 1);
		io_read(file, current_map_data, current_map_size);
		io_close(file);
	}
	return 1;
}

static int server_run()
{
	NETADDR bindaddr;

	snap_init_id();
	net_init();
	
	/* */
	console_register_print_callback(server_send_rcon_line_authed, 0);

	/* load map */
	if(!server_load_map(config.sv_map))
	{
		dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
		return -1;
	}
	
	/* start server */
	/* TODO: IPv6 support */
	if(config.sv_bindaddr[0] && net_host_lookup(config.sv_bindaddr, &bindaddr, NETTYPE_IPV4) == 0)
	{
		/* sweet! */
		bindaddr.port = config.sv_port;
	}
	else
	{
		mem_zero(&bindaddr, sizeof(bindaddr));
		bindaddr.port = config.sv_port;
	}
	
	net = netserver_open(bindaddr, config.sv_max_clients, 0);
	if(!net)
	{
		dbg_msg("server", "couldn't open socket. port might already be in use");
		return -1;
	}
	
	netserver_set_callbacks(net, new_client_callback, del_client_callback, 0);
	
	dbg_msg("server", "server name is '%s'", config.sv_name);
	
	mods_init();
	dbg_msg("server", "version %s", mods_net_version());

	/* start game */
	{
		int64 reporttime = time_get();
		int reportinterval = 3;
	
		lastheartbeat = 0;
		game_start_time = time_get();
	
		if(config.debug)
			dbg_msg("server", "baseline memory usage %dk", mem_stats()->allocated/1024);

		while(run_server)
		{
			static PERFORMACE_INFO rootscope = {"root", 0};
			int64 t = time_get();
			int new_ticks = 0;
			
			perf_start(&rootscope);
			
			/* load new map TODO: don't poll this */
			if(strcmp(config.sv_map, current_map) != 0 || config.sv_map_reload)
			{
				config.sv_map_reload = 0;
				
				/* load map */
				if(server_load_map(config.sv_map))
				{
					int c;
					
					/* new map loaded */
					mods_shutdown();
					
					for(c = 0; c < MAX_CLIENTS; c++)
					{
						if(clients[c].state == SRVCLIENT_STATE_EMPTY)
							continue;
						
						server_send_map(c);
						reset_client(c);
						clients[c].state = SRVCLIENT_STATE_CONNECTING;
					}
					
					game_start_time = time_get();
					current_tick = 0;
					mods_init();
				}
				else
				{
					dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
					config_set_sv_map(&config, current_map);
				}
			}
			
			while(t > server_tick_start_time(current_tick+1))
			{
				current_tick++;
				new_ticks++;
				
				/* apply new input */
				{
					static PERFORMACE_INFO scope = {"input", 0};
					int c, i;
					
					perf_start(&scope);
					
					for(c = 0; c < MAX_CLIENTS; c++)
					{
						if(clients[c].state == SRVCLIENT_STATE_EMPTY)
							continue;
						for(i = 0; i < 200; i++)
						{
							if(clients[c].inputs[i].game_tick == server_tick())
							{
								if(clients[c].state == SRVCLIENT_STATE_INGAME)
									mods_client_predicted_input(c, clients[c].inputs[i].data);
								break;
							}
						}
					}
					
					perf_end();
				}
				
				/* progress game */
				{
					static PERFORMACE_INFO scope = {"tick", 0};
					perf_start(&scope);
					mods_tick();
					perf_end();
				}
			}
			
			/* snap game */
			if(new_ticks)
			{
				if(config.sv_high_bandwidth || (current_tick%2) == 0)
				{
					static PERFORMACE_INFO scope = {"snap", 0};
					perf_start(&scope);
					server_do_snap();
					perf_end();
				}
			}
			
			/* master server stuff */
			register_update();
	

			{
				static PERFORMACE_INFO scope = {"net", 0};
				perf_start(&scope);
				server_pump_network();
				perf_end();
			}

			perf_end();
	
			if(reporttime < time_get())
			{
				if(config.debug)
				{
					/*
					static NETSTATS prev_stats;
					NETSTATS stats;
					netserver_stats(net, &stats);
					
					perf_next();
					
					if(config.dbg_pref)
						perf_dump(&rootscope);

					dbg_msg("server", "send=%8d recv=%8d",
						(stats.send_bytes - prev_stats.send_bytes)/reportinterval,
						(stats.recv_bytes - prev_stats.recv_bytes)/reportinterval);
						
					prev_stats = stats;
					*/
				}
	
				reporttime += time_freq()*reportinterval;
			}
			
			/* wait for incomming data */
			net_socket_read_wait(netserver_socket(net), 5);
		}
	}

	mods_shutdown();
	map_unload();

	if(current_map_data)
		mem_free(current_map_data);
	return 0;
}

static void con_kick(void *result, void *user_data)
{
	server_kick(console_arg_int(result, 0), "kicked by console");
}

static int str_allnum(const char *str)
{
	while(*str)
	{
		if(!(*str >= '0' && *str <= '9'))
			return 0;
		str++;
	}
	return 1;
}

static void con_ban(void *result, void *user_data)
{
	NETADDR addr;
	char addrstr[128];
	const char *str = console_arg_string(result, 0);
	int minutes = 30;
	
	if(console_arg_num(result) > 1)
		minutes = console_arg_int(result, 1);
	
	if(net_addr_from_str(&addr, str) == 0)
		server_ban_add(addr, minutes*60);
	else if(str_allnum(str))
	{
		NETADDR addr;
		int cid = atoi(str);

		if(cid < 0 || cid > MAX_CLIENTS || clients[cid].state == SRVCLIENT_STATE_EMPTY)
		{
			dbg_msg("server", "invalid client id");
			return;
		}

		netserver_client_addr(net, cid, &addr);
		server_ban_add(addr, minutes*60);
	}
	
	addr.port = 0;
	net_addr_str(&addr, addrstr, sizeof(addrstr));
	
	if(minutes)
		dbg_msg("server", "banned %s for %d minutes", addrstr, minutes);
	else
		dbg_msg("server", "banned %s for life", addrstr);
}

static void con_unban(void *result, void *user_data)
{
	NETADDR addr;
	const char *str = console_arg_string(result, 0);
	
	if(net_addr_from_str(&addr, str) == 0)
		server_ban_remove(addr);
	else
		dbg_msg("server", "invalid network address");
}

static void con_bans(void *result, void *user_data)
{
	int i;
	unsigned now = time_timestamp();
	NETBANINFO info;
	NETADDR addr;
	int num = netserver_ban_num(net);
	for(i = 0; i < num; i++)
	{
		unsigned t;
		netserver_ban_get(net, i, &info);
		addr = info.addr;
		
		if(info.expires == 0xffffffff)
		{
			dbg_msg("server", "#%d %d.%d.%d.%d for life", i, addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]);
		}
		else
		{
			t = info.expires - now;
			dbg_msg("server", "#%d %d.%d.%d.%d for %d minutes and %d seconds", i, addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], t/60, t%60);
		}
	}
	dbg_msg("server", "%d ban(s)", num);
}

static void con_status(void *result, void *user_data)
{
	int i;
	NETADDR addr;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i].state == SRVCLIENT_STATE_INGAME)
		{
			netserver_client_addr(net, i, &addr);
			dbg_msg("server", "id=%d addr=%d.%d.%d.%d:%d name='%s' score=%d",
				i, addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], addr.port,
				clients[i].name, clients[i].score);
		}
	}
}

static void con_shutdown(void *result, void *user_data)
{
	run_server = 0;
}

static void con_record(void *result, void *user_data)
{
	char filename[512];
	str_format(filename, sizeof(filename), "demos/%s.demo", console_arg_string(result, 0));
	demorec_record_start(filename, mods_net_version(), current_map, current_map_crc, "server");
}

static void con_stoprecord(void *result, void *user_data)
{
	demorec_record_stop();
}

static void server_register_commands()
{
	MACRO_REGISTER_COMMAND("kick", "i", CFGFLAG_SERVER, con_kick, 0, "");
	MACRO_REGISTER_COMMAND("ban", "s?i", CFGFLAG_SERVER, con_ban, 0, "");
	MACRO_REGISTER_COMMAND("unban", "s", CFGFLAG_SERVER, con_unban, 0, "");
	MACRO_REGISTER_COMMAND("bans", "", CFGFLAG_SERVER, con_bans, 0, "");
	MACRO_REGISTER_COMMAND("status", "", CFGFLAG_SERVER, con_status, 0, "");
	MACRO_REGISTER_COMMAND("shutdown", "", CFGFLAG_SERVER, con_shutdown, 0, "");

	MACRO_REGISTER_COMMAND("record", "s", CFGFLAG_SERVER, con_record, 0, "");
	MACRO_REGISTER_COMMAND("stoprecord", "", CFGFLAG_SERVER, con_stoprecord, 0, "");
}

int main(int argc, char **argv)
{
#if defined(CONF_FAMILY_WINDOWS)
	int i;
	for(i = 1; i < argc; i++)
	{
		if(strcmp("-s", argv[i]) == 0 || strcmp("--silent", argv[i]) == 0)
		{
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			break;
		}
	}
#endif

	/* init the engine */
	dbg_msg("server", "starting...");
	engine_init("Teeworlds");
	
	/* register all console commands */
	server_register_commands();
	mods_console_init();
	
	/* parse the command line arguments */
	engine_parse_arguments(argc, argv);
	
	/* run the server */
	server_run();
	return 0;
}

