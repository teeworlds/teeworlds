#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <baselib/system.h>
#include <baselib/config.h>

#include <engine/interface.h>

#include <engine/packet.h>
#include <engine/snapshot.h>

#include <engine/compression.h>
#include <engine/versions.h>

#include <engine/network.h>
#include <engine/config.h>

#include <mastersrv/mastersrv.h>

namespace baselib {}
using namespace baselib;

static snapshot_builder builder;

void *snap_new_item(int type, int id, int size)
{
	dbg_assert(type >= 0 && type <=0xffff, "incorrect type");
	dbg_assert(id >= 0 && id <=0xffff, "incorrect id");
	return builder.new_item(type, id, size);
}


struct snap_id
{
	short next;
	short state; // 0 = free, 1 = alloced, 2 = timed
	int timeout_tick;
};

static const int MAX_IDS = 8*1024; // should be lowered
static snap_id snap_ids[8*1024];
static int snap_first_free_id;
static int snap_first_timed_id;
static int snap_last_timed_id;
static int snap_id_usage;
static int snap_id_inusage;

static int snap_id_inited = 0;

void snap_init_id()
{
	for(int i = 0; i < MAX_IDS; i++)
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




//
class client
{
public:
	enum
	{
		STATE_EMPTY = 0,
		STATE_CONNECTING = 1,
		STATE_INGAME = 2,
	};

	struct stored_snapshot
	{
		int64 send_time;
		snapshot snap;
	};

	// connection state info
	int state;
	int latency;
	
	int last_acked_snapshot;
	snapshot_storage snapshots;

	char name[MAX_NAME_LENGTH];
	char clan[MAX_CLANNAME_LENGTH];

	bool is_empty() const { return state == STATE_EMPTY; }
	bool is_ingame() const { return state == STATE_INGAME; }
};

static client clients[MAX_CLIENTS];
static int current_tick = 0;
static net_server net;

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
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i].state = client::STATE_EMPTY;
		clients[i].name[0] = 0;
		clients[i].clan[0] = 0;
		//clients[i].lastactivity = 0;
	}

	current_tick = 0;

	return 0;
}

int server_getclientinfo(int client_id, client_info *info)
{
	dbg_assert(client_id >= 0 && client_id < MAX_CLIENTS, "client_id is not valid");
	dbg_assert(info != 0, "info can not be null");

	if(clients[client_id].is_ingame())
	{
		info->name = clients[client_id].name;
		info->latency = clients[client_id].latency;
		return 1;
	}
	return 0;
}


int server_send_msg(int client_id)
{
	const msg_info *info = msg_get_info();
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
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(clients[i].is_ingame())
			{
				packet.client_id = i;
				net.send(&packet);
			}
	}
	else
		net.send(&packet);
	return 0;
}

static int new_client_callback(int cid, void *user)
{
	clients[cid].state = client::STATE_CONNECTING;
	clients[cid].name[0] = 0;
	clients[cid].clan[0] = 0;
	clients[cid].snapshots.purge_all();
	clients[cid].last_acked_snapshot = -1;
	return 0;
}

static int del_client_callback(int cid, void *user)
{
	clients[cid].state = client::STATE_EMPTY;
	clients[cid].name[0] = 0;
	clients[cid].clan[0] = 0;
	clients[cid].snapshots.purge_all();
	
	mods_client_drop(cid);
	return 0;
}
		
// TODO: remove this class
class server
{
public:
	//socket_udp4 game_socket;
	int64 lasttick;
	int64 lastheartbeat;
	netaddr4 master_server;

	int biggest_snapshot;

	bool run()
	{
		biggest_snapshot = 0;

		net_init(); // For Windows compatibility.
		
		snap_init_id();

		// load map
		if(!map_load(config.sv_map))
		{
			dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
			return false;
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
		
		if(!net.open(bindaddr, config.sv_max_clients, 0))
		{
			dbg_msg("server", "couldn't open socket. port might already be in use");
			return false;
		}
		
		net.set_callbacks(new_client_callback, del_client_callback, 0);
		
		dbg_msg("server", "server name is '%s'", config.sv_name);
		dbg_msg("server", "masterserver is '%s'", config.masterserver);
		if (net_host_lookup(config.masterserver, MASTERSERVER_PORT, &master_server) != 0)
		{
			// TODO: fix me
			//master_server = netaddr4(0, 0, 0, 0, 0);
		}

		mods_init();

		int64 time_per_tick = time_freq()/SERVER_TICK_SPEED;
		int64 time_per_heartbeat = time_freq() * 30;
		int64 starttime = time_get();
		//int64 lasttick = starttime;
		lasttick = starttime;
		lastheartbeat = 0;

		int64 reporttime = time_get();
		int reportinterval = 3;
		//int64 reportinterval = time_freq()*3;

		int64 simulationtime = 0;
		int64 snaptime = 0;
		int64 networktime = 0;
		int64 totaltime = 0;

		while(true)
		{
			int64 t = time_get();
			if(t-lasttick > time_per_tick)
			{
				{
					int64 start = time_get();
					tick();
					simulationtime += time_get()-start;
				}

				{
					int64 start = time_get();
					snap();
					snaptime += time_get()-start;
				}

				lasttick += time_per_tick;
			}

			if(config.sv_sendheartbeats)
			{
				if (t > lastheartbeat+time_per_heartbeat)
				{
					send_heartbeat();
					lastheartbeat = t+time_per_heartbeat;
				}
			}

			{
				int64 start = time_get();
				pump_network();
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
	}

	void tick()
	{
		current_tick++;
		mods_tick();
	}

	void snap()
	{
		//if(current_tick&1)
		//	return;
		mods_presnap();

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(clients[i].is_ingame())
			{
				char data[MAX_SNAPSHOT_SIZE];
				char deltadata[MAX_SNAPSHOT_SIZE];
				char compdata[MAX_SNAPSHOT_SIZE];
				//char intdata[MAX_SNAPSHOT_SIZE];
				builder.start();
				mods_snap(i);

				// finish snapshot
				int snapshot_size = builder.finish(data);

				// remove old snapshos
				// keep 1 seconds worth of snapshots
				clients[i].snapshots.purge_until(current_tick-SERVER_TICK_SPEED);
				
				// save it the snapshot
				clients[i].snapshots.add(current_tick, time_get(), snapshot_size, data);
				
				// find snapshot that we can preform delta against
				static snapshot emptysnap;
				emptysnap.data_size = 0;
				emptysnap.num_items = 0;
				
				snapshot *deltashot = &emptysnap;
				int deltashot_size;
				int delta_tick = -1;
				{
					void *delta_data;
					deltashot_size = clients[i].snapshots.get(clients[i].last_acked_snapshot, 0, (void **)&delta_data);
					if(deltashot_size >= 0)
					{
						delta_tick = clients[i].last_acked_snapshot;
						deltashot = (snapshot *)delta_data;
					}
				}
				
				// create delta
				int deltasize = snapshot_create_delta(deltashot, (snapshot*)data, deltadata);
				
				if(deltasize)
				{
					// compress it
					//int intsize = -1;
					unsigned char intdata[MAX_SNAPSHOT_SIZE];
					int intsize = intpack_compress(deltadata, deltasize, intdata);
					
					int compsize = zerobit_compress(intdata, intsize, compdata);
					//dbg_msg("compress", "%5d --delta-> %5d --int-> %5d --zero-> %5d %5d",
						//snapshot_size, deltasize, intsize, compsize, intsize-compsize);
					snapshot_size = compsize;

					if(snapshot_size > biggest_snapshot)
						biggest_snapshot = snapshot_size;

					const int max_size = MAX_SNAPSHOT_PACKSIZE;
					int numpackets = (snapshot_size+max_size-1)/max_size;
					(void)numpackets;
					for(int n = 0, left = snapshot_size; left; n++)
					{
						int chunk = left < max_size ? left : max_size;
						left -= chunk;

						//if(numpackets == 1)
						//	msg_pack_start_system(NETMSG_SNAPSMALL, 0);
						//else
						msg_pack_start_system(NETMSG_SNAP, 0);
						msg_pack_int(current_tick);
						msg_pack_int(current_tick-delta_tick); // compressed with
						msg_pack_int(chunk);
						msg_pack_raw(&compdata[n*max_size], chunk);
						msg_pack_end();
						//const msg_info *info = msg_get_info();
						//dbg_msg("server", "size=%d", info->size);
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

	void send_map(int cid)
	{
		msg_pack_start_system(NETMSG_MAP, MSGFLAG_VITAL);
		msg_pack_string(config.sv_map, 0);
		msg_pack_end();
		server_send_msg(cid);
	}
	
	void send_heartbeat()
	{
		NETPACKET packet;
		packet.client_id = -1;
		packet.address = master_server;
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = sizeof(SERVERBROWSE_HEARTBEAT);
		packet.data = SERVERBROWSE_HEARTBEAT;
		net.send(&packet);
	}

	void drop(int cid, const char *reason)
	{
		if(clients[cid].state == client::STATE_EMPTY)
			return;

		clients[cid].state = client::STATE_EMPTY;
		mods_client_drop(cid);
		dbg_msg("server", "player dropped. reason='%s' cid=%x name='%s'", reason, cid, clients[cid].name);
	}

	void process_client_packet(NETPACKET *packet)
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
				if(strcmp(version, TEEWARS_NETVERSION_STRING) != 0)
				//if(strcmp(version, "ERROR") != 0)
				{
					// OH FUCK! wrong version, drop him
					char reason[256];
					sprintf(reason, "wrong version. server is running %s.", TEEWARS_NETVERSION_STRING);
					net.drop(cid, reason);
					return;
				}
				
				strncpy(clients[cid].name, msg_unpack_string(), MAX_NAME_LENGTH);
				strncpy(clients[cid].clan, msg_unpack_string(), MAX_CLANNAME_LENGTH);
				const char *password = msg_unpack_string();
				const char *skin = msg_unpack_string();
				(void)password; // ignore these variables
				(void)skin;
				send_map(cid);
			}
			else if(msg == NETMSG_ENTERGAME)
			{
				if(clients[cid].state != client::STATE_INGAME)
				{
					dbg_msg("server", "player as entered the game. cid=%x", cid);
					clients[cid].state = client::STATE_INGAME;
					mods_client_enter(cid);
				}
			}
			else if(msg == NETMSG_INPUT)
			{
				int input[MAX_INPUT_SIZE];
				int size = msg_unpack_int();
				for(int i = 0; i < size/4; i++)
					input[i] = msg_unpack_int();
				mods_client_input(cid, input);
			}
			else if(msg == NETMSG_SNAPACK)
			{
				clients[cid].last_acked_snapshot = msg_unpack_int();
				int64 tagtime;
				if(clients[cid].snapshots.get(clients[cid].last_acked_snapshot, &tagtime, 0) >= 0)
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

	void process_packet(NETPACKET *packet)
	{
	}

	void client_timeout(int clientId)
	{
		drop(clientId, "client timedout");
	}

	void send_serverinfo(NETADDR4 *addr)
	{
		NETPACKET packet;
		
		data_packer packer;
		packer.reset();
		packer.add_raw(SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));
		packer.add_string(config.sv_name, 128);
		packer.add_string(config.sv_map, 128);
		packer.add_int(MAX_CLIENTS); // max_players
		int c = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!clients[i].is_empty())
				c++;
		}
		packer.add_int(c); // num_players
		
		packet.client_id = -1;
		packet.address = *addr;
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = packer.size();
		packet.data = packer.data();
		net.send(&packet);
	}


	void send_fwcheckresponse(NETADDR4 *addr)
	{
		NETPACKET packet;
		packet.client_id = -1;
		packet.address = *addr;
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = sizeof(SERVERBROWSE_FWRESPONSE);
		packet.data = SERVERBROWSE_FWRESPONSE;
		net.send(&packet);
	}

	void pump_network()
	{
		net.update();
		
		// process packets
		NETPACKET packet;
		while(net.recv(&packet))
		{
			if(packet.client_id == -1)
			{
				// stateless
				if(packet.data_size == sizeof(SERVERBROWSE_GETINFO) &&
					memcmp(packet.data, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					send_serverinfo(&packet.address);
				}
				else if(packet.data_size == sizeof(SERVERBROWSE_FWCHECK) &&
					memcmp(packet.data, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
				{
					send_fwcheckresponse(&packet.address);
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
					dbg_msg("server", "ERROR: configure your firewall/nat to let trough udp on port %d.", 8303);
				}
			}
			else
				process_client_packet(&packet);
		}
	}
};

int main(int argc, char **argv)
{
	dbg_msg("server", "starting...");

	config_reset();

#ifdef CONF_PLATFORM_MACOSX
	const char *config_filename = "~/.teewars";
#else
	const char *config_filename = "default.cfg";
#endif

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'f' && argv[i][2] == 0 && argc - i > 1)
		{
			config_filename = argv[i+1];
			i++;
		}
	}

	config_load(config_filename);

	// parse arguments
	for(int i = 1; i < argc; i++)
		config_set(argv[i]);

	server_init();
	server s;
	s.run();
	return 0;
}
