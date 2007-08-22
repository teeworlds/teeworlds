#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <engine/system.h>
#include <engine/config.h>

#include <engine/interface.h>

#include <engine/protocol.h>
#include <engine/snapshot.h>

#include <engine/compression.h>

#include <engine/network.h>
#include <engine/config.h>
#include <engine/packer.h>

#include <mastersrv/mastersrv.h>

static SNAPBUILD builder;

static int64 lasttick;
static int64 lastheartbeat;
static NETADDR4 master_server;

static int biggest_snapshot;

void *snap_new_item(int type, int id, int size)
{
	dbg_assert(type >= 0 && type <=0xffff, "incorrect type");
	dbg_assert(id >= 0 && id <=0xffff, "incorrect id");
	return snapbuild_new_item(&builder, type, id, size);
}

typedef struct
{
	short next;
	short state; // 0 = free, 1 = alloced, 2 = timed
	int timeout_tick;
} SNAP_ID;

static const int MAX_IDS = 8*1024; // should be lowered
static SNAP_ID snap_ids[8*1024];
static int snap_first_free_id;
static int snap_first_timed_id;
static int snap_last_timed_id;
static int snap_id_usage;
static int snap_id_inusage;
static int snap_id_inited = 0;

static void snap_init_id()
{
	int i;
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

int snap_new_id()
{
	dbg_assert(snap_id_inited == 1, "requesting id too soon");
	
	// process timed ids
	while(snap_first_timed_id != -1 && snap_ids[snap_first_timed_id].timeout_tick < server_tick())
	{
		int next_timed = snap_ids[snap_first_timed_id].next;
		
		// add it to the free list
		snap_ids[snap_first_timed_id].next = snap_first_free_id;
		snap_ids[snap_first_timed_id].state = 0;
		snap_first_free_id = snap_first_timed_id;
		
		// remove it from the timed list
		snap_first_timed_id = next_timed;
		if(snap_first_timed_id == -1)
			snap_last_timed_id = -1;
			
		snap_id_usage--;
	}
	
	int id = snap_first_free_id;
	dbg_assert(id != -1, "id error");
	snap_first_free_id = snap_ids[snap_first_free_id].next;
	snap_ids[id].state = 1;
	snap_id_usage++;
	snap_id_inusage++;
	return id;
}

void snap_free_id(int id)
{
	dbg_assert(snap_ids[id].state == 1, "id is not alloced");

	snap_id_inusage--;
	snap_ids[id].state = 2;
	snap_ids[id].timeout_tick = server_tick() + server_tickspeed()*5;
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

enum
{
	SRVCLIENT_STATE_EMPTY = 0,
	SRVCLIENT_STATE_CONNECTING = 1,
	SRVCLIENT_STATE_INGAME = 2,
};

//
typedef struct
{
	// connection state info
	int state;
	int latency;
	
	int last_acked_snapshot;
	SNAPSTORAGE snapshots;

	char name[MAX_NAME_LENGTH];
	char clan[MAX_CLANNAME_LENGTH];
} CLIENT;

static CLIENT clients[MAX_CLIENTS];
static int current_tick = 0;
static NETSERVER *net;

int server_tick()
{
	return current_tick;
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
	NETPACKET packet;
	mem_zero(&packet, sizeof(NETPACKET));
	
	packet.client_id = client_id;
	packet.data = info->data;
	packet.data_size = info->size;

	if(info->flags&MSGFLAG_VITAL)	
		packet.flags = PACKETFLAG_VITAL;
			
	if(client_id == -1)
	{
		// broadcast
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
	return 0;
}


static void server_do_tick()
{
	current_tick++;
	mods_tick();
}

static void server_do_snap()
{
	mods_presnap();

	int i;
	for( i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i].state == SRVCLIENT_STATE_INGAME)
		{
			char data[MAX_SNAPSHOT_SIZE];
			char deltadata[MAX_SNAPSHOT_SIZE];
			char compdata[MAX_SNAPSHOT_SIZE];
			snapbuild_init(&builder);
			mods_snap(i);

			// finish snapshot
			int snapshot_size = snapbuild_finish(&builder, data);
			int crc = snapshot_crc((SNAPSHOT*)data);

			// remove old snapshos
			// keep 1 seconds worth of snapshots
			snapstorage_purge_until(&clients[i].snapshots, current_tick-SERVER_TICK_SPEED);
			
			// save it the snapshot
			snapstorage_add(&clients[i].snapshots, current_tick, time_get(), snapshot_size, data);
			
			// find snapshot that we can preform delta against
			static SNAPSHOT emptysnap;
			emptysnap.data_size = 0;
			emptysnap.num_items = 0;
			
			SNAPSHOT *deltashot = &emptysnap;
			int deltashot_size;
			int delta_tick = -1;
			{
				deltashot_size = snapstorage_get(&clients[i].snapshots, clients[i].last_acked_snapshot, 0, &deltashot);
				if(deltashot_size >= 0)
					delta_tick = clients[i].last_acked_snapshot;
			}
			
			// create delta
			int deltasize = snapshot_create_delta(deltashot, (SNAPSHOT*)data, deltadata);
			
			if(deltasize)
			{
				// compress it
				unsigned char intdata[MAX_SNAPSHOT_SIZE];
				int intsize = intpack_compress(deltadata, deltasize, intdata);
				
				int compsize = zerobit_compress(intdata, intsize, compdata);
				snapshot_size = compsize;

				if(snapshot_size > biggest_snapshot)
					biggest_snapshot = snapshot_size;

				const int max_size = MAX_SNAPSHOT_PACKSIZE;
				int numpackets = (snapshot_size+max_size-1)/max_size;
				(void)numpackets;
				int n, left;
				for(n = 0, left = snapshot_size; left; n++)
				{
					int chunk = left < max_size ? left : max_size;
					left -= chunk;

					msg_pack_start_system(NETMSG_SNAP, 0);
					msg_pack_int(current_tick);
					msg_pack_int(current_tick-delta_tick); // compressed with
					msg_pack_int(crc);
					msg_pack_int(chunk);
					msg_pack_raw(&compdata[n*max_size], chunk);
					msg_pack_end();
					server_send_msg(i);
				}
			}
			else
			{
				msg_pack_start_system(NETMSG_SNAPEMPTY, 0);
				msg_pack_int(current_tick);
				msg_pack_int(current_tick-delta_tick); // compressed with
				msg_pack_end();
				server_send_msg(i);
			}
		}
	}

	mods_postsnap();
}


static int new_client_callback(int cid, void *user)
{
	clients[cid].state = SRVCLIENT_STATE_CONNECTING;
	clients[cid].name[0] = 0;
	clients[cid].clan[0] = 0;
	snapstorage_purge_all(&clients[cid].snapshots);
	clients[cid].last_acked_snapshot = -1;
	return 0;
}

static int del_client_callback(int cid, void *user)
{
	clients[cid].state = SRVCLIENT_STATE_EMPTY;
	clients[cid].name[0] = 0;
	clients[cid].clan[0] = 0;
	snapstorage_purge_all(&clients[cid].snapshots);
	
	mods_client_drop(cid);
	return 0;
}


static void server_send_map(int cid)
{
	msg_pack_start_system(NETMSG_MAP, MSGFLAG_VITAL);
	msg_pack_string(config.sv_map, 0);
	msg_pack_end();
	server_send_msg(cid);
}
	
static void server_send_heartbeat()
{
	NETPACKET packet;
	packet.client_id = -1;
	packet.address = master_server;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_HEARTBEAT);
	packet.data = SERVERBROWSE_HEARTBEAT;
	netserver_send(net, &packet);
}

static void server_process_client_packet(NETPACKET *packet)
{
	int cid = packet->client_id;
	int sys;
	int msg = msg_unpack_start(packet->data, packet->data_size, &sys);
	if(sys)
	{
		// system message
		if(msg == NETMSG_INFO)
		{
			char version[64];
			strncpy(version, msg_unpack_string(), 64);
			if(strcmp(version, mods_net_version()) != 0)
			{
				// OH FUCK! wrong version, drop him
				char reason[256];
				sprintf(reason, "wrong version. server is running %s.", mods_net_version());
				netserver_drop(net, cid, reason);
				return;
			}
			
			strncpy(clients[cid].name, msg_unpack_string(), MAX_NAME_LENGTH);
			strncpy(clients[cid].clan, msg_unpack_string(), MAX_CLANNAME_LENGTH);
			const char *password = msg_unpack_string();
			const char *skin = msg_unpack_string();
			(void)password; // ignore these variables
			(void)skin;
			server_send_map(cid);
		}
		else if(msg == NETMSG_ENTERGAME)
		{
			if(clients[cid].state != SRVCLIENT_STATE_INGAME)
			{
				dbg_msg("server", "player as entered the game. cid=%x", cid);
				clients[cid].state = SRVCLIENT_STATE_INGAME;
				mods_client_enter(cid);
			}
		}
		else if(msg == NETMSG_INPUT)
		{
			int input[MAX_INPUT_SIZE];
			int size = msg_unpack_int();
			int i;
			for(i = 0; i < size/4; i++)
				input[i] = msg_unpack_int();
			mods_client_input(cid, input);
		}
		else if(msg == NETMSG_SNAPACK)
		{
			clients[cid].last_acked_snapshot = msg_unpack_int();
			int64 tagtime;
			if(snapstorage_get(&clients[cid].snapshots, clients[cid].last_acked_snapshot, &tagtime, 0) >= 0)
				clients[cid].latency = (int)(((time_get()-tagtime)*1000)/time_freq());
		}
		else
		{
			dbg_msg("server", "strange message cid=%d msg=%d data_size=%d", cid, msg, packet->data_size);
		}
	}
	else
	{
		// game message
		mods_message(msg, cid);
	}
}

static void server_send_serverinfo(NETADDR4 *addr)
{
	NETPACKET packet;
	
	PACKER p;
	packer_reset(&p);
	packer_add_raw(&p, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));
	packer_add_string(&p, config.sv_name, 128);
	packer_add_string(&p, config.sv_map, 128);
	packer_add_int(&p, netserver_max_clients(net)); // max_players
	int c = 0;
	int i;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(!clients[i].state != SRVCLIENT_STATE_EMPTY)
			c++;
	}
	packer_add_int(&p, c); // num_players
	
	packet.client_id = -1;
	packet.address = *addr;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = packer_size(&p);
	packet.data = packer_data(&p);
	netserver_send(net, &packet);
}


static void server_send_fwcheckresponse(NETADDR4 *addr)
{
	NETPACKET packet;
	packet.client_id = -1;
	packet.address = *addr;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_FWRESPONSE);
	packet.data = SERVERBROWSE_FWRESPONSE;
	netserver_send(net, &packet);
}

static void server_pump_network()
{
	netserver_update(net);
	
	// process packets
	NETPACKET packet;
	while(netserver_recv(net, &packet))
	{
		if(packet.client_id == -1)
		{
			// stateless
			if(packet.data_size == sizeof(SERVERBROWSE_GETINFO) &&
				memcmp(packet.data, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
			{
				server_send_serverinfo(&packet.address);
			}
			else if(packet.data_size == sizeof(SERVERBROWSE_FWCHECK) &&
				memcmp(packet.data, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
			{
				server_send_fwcheckresponse(&packet.address);
			}
			else if(packet.data_size == sizeof(SERVERBROWSE_FWOK) &&
				memcmp(packet.data, SERVERBROWSE_FWOK, sizeof(SERVERBROWSE_FWOK)) == 0)
			{
				if(config.debug)
					dbg_msg("server", "no firewall/nat problems detected");
			}
			else if(packet.data_size == sizeof(SERVERBROWSE_FWERROR) &&
				memcmp(packet.data, SERVERBROWSE_FWERROR, sizeof(SERVERBROWSE_FWERROR)) == 0)
			{
				dbg_msg("server", "ERROR: the master server reports that clients can not connect to this server.");
				dbg_msg("server", "ERROR: configure your firewall/nat to let trough udp on port %d.", config.sv_port);
			}
		}
		else
			server_process_client_packet(&packet);
	}
}


static int server_run()
{
	biggest_snapshot = 0;

	net_init(); // For Windows compatibility.
	
	snap_init_id();

	// load map
	if(!map_load(config.sv_map))
	{
		dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
		return -1;
	}
	
	// start server
	NETADDR4 bindaddr;

	if(strlen(config.sv_bindaddr) && net_host_lookup(config.sv_bindaddr, config.sv_port, &bindaddr) != 0)
	{
		// sweet!
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
	dbg_msg("server", "masterserver is '%s'", config.masterserver);
	if (net_host_lookup(config.masterserver, MASTERSERVER_PORT, &master_server) != 0)
	{
		// TODO: fix me
		//master_server = netaddr4(0, 0, 0, 0, 0);
	}

	mods_init();
	dbg_msg("server", "version %s", mods_net_version());

	int64 time_per_tick = time_freq()/SERVER_TICK_SPEED;
	int64 time_per_heartbeat = time_freq() * 30;
	int64 starttime = time_get();
	lasttick = starttime;
	lastheartbeat = 0;

	int64 reporttime = time_get();
	int reportinterval = 3;

	int64 simulationtime = 0;
	int64 snaptime = 0;
	int64 networktime = 0;
	int64 totaltime = 0;

	while(1)
	{
		int64 t = time_get();
		if(t-lasttick > time_per_tick)
		{
			{
				int64 start = time_get();
				server_do_tick();
				simulationtime += time_get()-start;
			}

			{
				int64 start = time_get();
				server_do_snap();
				snaptime += time_get()-start;
			}

			lasttick += time_per_tick;
		}

		if(config.sv_sendheartbeats)
		{
			if (t > lastheartbeat+time_per_heartbeat)
			{
				server_send_heartbeat();
				lastheartbeat = t+time_per_heartbeat;
			}
		}

		{
			int64 start = time_get();
			server_pump_network();
			networktime += time_get()-start;
		}

		if(reporttime < time_get())
		{
			if(config.debug)
			{
				dbg_msg("server", "sim=%.02fms snap=%.02fms net=%.02fms total=%.02fms load=%.02f%% ids=%d/%d",
					(simulationtime/reportinterval)/(double)time_freq()*1000,
					(snaptime/reportinterval)/(double)time_freq()*1000,
					(networktime/reportinterval)/(double)time_freq()*1000,
					(totaltime/reportinterval)/(double)time_freq()*1000,
					(totaltime)/reportinterval/(double)time_freq()*100.0f,
					snap_id_inusage, snap_id_usage);
			}

			simulationtime = 0;
			snaptime = 0;
			networktime = 0;
			totaltime = 0;

			reporttime += time_freq()*reportinterval;
		}
		totaltime += time_get()-t;
		thread_sleep(1);
	}

	mods_shutdown();
	map_unload();

	return 0;
}


int main(int argc, char **argv)
{
	dbg_msg("server", "starting...");

	config_reset();

#ifdef CONF_PLATFORM_MACOSX
	const char *config_filename = "~/.teewars";
#else
	const char *config_filename = "default.cfg";
#endif

	int i;
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'f' && argv[i][2] == 0 && argc - i > 1)
		{
			config_filename = argv[i+1];
			i++;
		}
	}

	config_load(config_filename);

	// parse arguments
	for(i = 1; i < argc; i++)
		config_set(argv[i]);

	server_run();
	return 0;
}

