
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <engine/system.h>
#include <engine/interface.h>
#include "ui.h"

#include <engine/protocol.h>
#include <engine/snapshot.h>
#include <engine/compression.h>
#include <engine/network.h>
#include <engine/config.h>
#include <engine/packer.h>

#include <mastersrv/mastersrv.h>

/*
	Server Time
	Client Mirror Time
	Client Predicted Time
	
	Snapshot Latency
		Downstream latency
	
	Prediction Latency
		Upstream latency
*/
static int info_request_begin;
static int info_request_end;
static int snapshot_part;
static int64 local_start_time;
static int64 game_start_time;

static float snapshot_latency = 0;
static float prediction_latency = 0;

static int extra_polating = 0;
static int debug_font;
static float frametime = 0.0001f;
static NETCLIENT *net;
static NETADDR4 master_server;
static NETADDR4 server_address;
static int window_must_refocus = 0;
static int snaploss = 0;
static int snapcrcerrors = 0;

static int current_recv_tick = 0;

// current time
static int current_tick = 0;
static float intratick = 0;

// predicted time
static int current_predtick = 0;
static float intrapredtick = 0;

static struct // TODO: handle input better
{
	int data[MAX_INPUT_SIZE]; // the input data
	int tick; // the tick that the input is for
	float latency; // prediction latency when we sent this input
} inputs[200];
static int current_input = 0;


// --- input snapping ---
static int input_data[MAX_INPUT_SIZE] = {0};
static int input_data_size;
static int input_is_changed = 1;
void snap_input(void *data, int size)
{
	if(input_data_size != size || memcmp(input_data, data, size))
		input_is_changed = 1;
	mem_copy(input_data, data, size);
	input_data_size = size;
}

// -- snapshot handling ---
enum
{
	NUM_SNAPSHOT_TYPES=2,
};

SNAPSTORAGE snapshot_storage;
static SNAPSTORAGE_HOLDER *snapshots[NUM_SNAPSHOT_TYPES];
static int recived_snapshots;
static char snapshot_incomming_data[MAX_SNAPSHOT_SIZE];

// ---

const void *snap_get_item(int snapid, int index, SNAP_ITEM *item)
{
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	SNAPSHOT_ITEM *i = snapshot_get_item(snapshots[snapid]->snap, index);
	item->type = snapitem_type(i);
	item->id = snapitem_id(i);
	return (void *)snapitem_data(i);
}

const void *snap_find_item(int snapid, int type, int id)
{
	// TODO: linear search. should be fixed.
	int i;
	for(i = 0; i < snapshots[snapid]->snap->num_items; i++)
	{
		SNAPSHOT_ITEM *itm = snapshot_get_item(snapshots[snapid]->snap, i);
		if(snapitem_type(itm) == type && snapitem_id(itm) == id)
			return (void *)snapitem_data(itm);
	}
	return 0x0;
}

int snap_num_items(int snapid)
{
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	return snapshots[snapid]->snap->num_items;
}

static void snap_init()
{
	snapshots[SNAP_CURRENT] = 0;
	snapshots[SNAP_PREV] = 0;
	recived_snapshots = 0;
	game_start_time = -1;
	
}

// ------ time functions ------
float client_intratick() { return intratick; }
float client_intrapredtick() { return intrapredtick; }
int client_tick() { return current_tick; }
int client_predtick() { return current_predtick; }
int client_tickspeed() { return SERVER_TICK_SPEED; }
float client_frametime() { return frametime; }
float client_localtime() { return (time_get()-local_start_time)/(float)(time_freq()); }

// ----- send functions -----
int client_send_msg()
{
	const MSG_INFO *info = msg_get_info();
	NETPACKET packet;
	mem_zero(&packet, sizeof(NETPACKET));
	
	packet.client_id = 0;
	packet.data = info->data;
	packet.data_size = info->size;

	if(info->flags&MSGFLAG_VITAL)	
		packet.flags = PACKETFLAG_VITAL;
	
	netclient_send(net, &packet);
	return 0;
}

static void client_send_info()
{
	recived_snapshots = 0;
	game_start_time = -1;

	msg_pack_start_system(NETMSG_INFO, MSGFLAG_VITAL);
	msg_pack_string(modc_net_version(), 128);
	msg_pack_string(config.player_name, 128);
	msg_pack_string(config.clan_name, 128);
	msg_pack_string(config.password, 128);
	msg_pack_string("myskin", 128);
	msg_pack_end();
	client_send_msg();
}

static void client_send_entergame()
{
	msg_pack_start_system(NETMSG_ENTERGAME, MSGFLAG_VITAL);
	msg_pack_end();
	client_send_msg();
}

static void client_send_error(const char *error)
{
	/*
	packet p(NETMSG_CLIENT_ERROR);
	p.write_str(error);
	send_packet(&p);
	//send_packet(&p);
	//send_packet(&p);
	*/
}


static void client_send_input()
{
	msg_pack_start_system(NETMSG_INPUT, 0);
	msg_pack_int(current_predtick);
	msg_pack_int(input_data_size);
	
	inputs[current_input].tick = current_predtick;
	inputs[current_input].latency = prediction_latency;
	
	int i;
	for(i = 0; i < input_data_size/4; i++)
	{
		inputs[current_input].data[i] = input_data[i];
		msg_pack_int(input_data[i]);
	}
	
	current_input++;
	current_input%=200;
	
	msg_pack_end();
	client_send_msg();
}

/* TODO: OPT: do this alot smarter! */
int *client_get_input(int tick)
{
	int i;
	int best = -1;
	for(i = 0; i < 200; i++)
	{
		if(inputs[i].tick <= tick && (best == -1 || inputs[best].tick < inputs[i].tick))
			best = i;
	}
	
	if(best != -1)
		return (int *)inputs[best].data;
	return 0;
}

// ------ server browse ----
static struct 
{
	SERVER_INFO infos[MAX_SERVERS];
	int64 request_times[MAX_SERVERS];
	NETADDR4 addresses[MAX_SERVERS];
	int num;
} servers;

static int serverlist_lan = 1;

int client_serverbrowse_getlist(SERVER_INFO **serverlist)
{
	*serverlist = servers.infos;
	return servers.num;
}

static void client_serverbrowse_init()
{
	servers.num = 0;
}

void client_serverbrowse_refresh(int lan)
{
	serverlist_lan = lan;
	
	if(serverlist_lan)
	{
		if(config.debug)
			dbg_msg("client", "broadcasting for servers");
		NETPACKET packet;
		packet.client_id = -1;
		mem_zero(&packet, sizeof(packet));
		packet.address.ip[0] = 0;
		packet.address.ip[1] = 0;
		packet.address.ip[2] = 0;
		packet.address.ip[3] = 0;
		packet.address.port = 8303;
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = sizeof(SERVERBROWSE_GETINFO);
		packet.data = SERVERBROWSE_GETINFO;
		netclient_send(net, &packet);	
		
		// reset the list
		servers.num = 0;		
	}
	else
	{
		if(config.debug)
			dbg_msg("client", "requesting server list");
		NETPACKET packet;
		mem_zero(&packet, sizeof(packet));
		packet.client_id = -1;
		packet.address = master_server;
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = sizeof(SERVERBROWSE_GETLIST);
		packet.data = SERVERBROWSE_GETLIST;
		netclient_send(net, &packet);	
		
		// reset the list
		servers.num = 0;
	}
}


static void client_serverbrowse_request(int id)
{
	if(config.debug)
	{
		dbg_msg("client", "requesting server info from %d.%d.%d.%d:%d",
			servers.addresses[id].ip[0], servers.addresses[id].ip[1], servers.addresses[id].ip[2],
			servers.addresses[id].ip[3], servers.addresses[id].port);
	}
	NETPACKET packet;
	packet.client_id = -1;
	packet.address = servers.addresses[id];
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_GETINFO);
	packet.data = SERVERBROWSE_GETINFO;
	netclient_send(net, &packet);
	servers.request_times[id] = time_get();
}

static void client_serverbrowse_update()
{
	int64 timeout = time_freq();
	int64 now = time_get();
	int max_requests = 10;
	
	// timeout old requests
	while(info_request_begin < servers.num && info_request_begin < info_request_end)
	{
		if(now > servers.request_times[info_request_begin]+timeout)
			info_request_begin++;
		else
			break;
	}
	
	// send new requests
	while(info_request_end < servers.num && info_request_end-info_request_begin < max_requests)
	{
		client_serverbrowse_request(info_request_end);
		info_request_end++;
	}
}

// ------ state handling -----
static int state;
int client_state() { return state; }
static void client_set_state(int s)
{
	if(config.debug)
		dbg_msg("client", "state change. last=%d current=%d", state, s);
	int old = state;
	state = s;
	if(old != s)
		modc_statechange(state, old);
}


void client_connect(const char *server_address_str)
{
	dbg_msg("client", "connecting to '%s'", server_address_str);
	char buf[512];
	strncpy(buf, server_address_str, 512);
	
	const char *port_str = 0;
	int k;
	for(k = 0; buf[k]; k++)
	{
		if(buf[k] == ':')
		{
			port_str = &(buf[k+1]);
			buf[k] = 0;
			break;
		}
	}
	
	int port = 8303;
	if(port_str)
		port = atoi(port_str);
		
	if(net_host_lookup(buf, port, &server_address) != 0)
		dbg_msg("client", "could not find the address of %s, connecting to localhost", buf);
	
	netclient_connect(net, &server_address);
	client_set_state(CLIENTSTATE_CONNECTING);	
	
	current_recv_tick = 0;
	
	// reset input
	int i;
	for(i = 0; i < 200; i++)
		inputs[i].tick = -1;
	current_input = 0;
	
	snapshot_latency = 0;
	prediction_latency = 0;
}

void client_disconnect()
{
	client_send_error("disconnected");
	netclient_disconnect(net, "disconnected");
	client_set_state(CLIENTSTATE_OFFLINE);
	map_unload();
}

static int client_load_data()
{
	debug_font = gfx_load_texture("data/debug_font.png");
	return 1;
}

static void client_debug_render()
{
	if(!config.debug)
		return;
		
	gfx_blend_normal();
	gfx_texture_set(debug_font);
	gfx_mapscreen(0,0,gfx_screenwidth(),gfx_screenheight());
	
	static NETSTATS prev, current;
	static int64 last_snap = 0;
	if(time_get()-last_snap > time_freq()/10)
	{
		last_snap = time_get();
		prev = current;
		netclient_stats(net, &current);
	}
	
	static float frametime_avg = 0;
	frametime_avg = frametime_avg*0.9f + frametime*0.1f;
	char buffer[512];
	sprintf(buffer, "send: %6d recv: %6d snaploss: %d snaplatency: %4.2f %c  predlatency: %4.2f  mem %dk   gfxmem: %dk  fps: %3d",
		(current.send_bytes-prev.send_bytes)*10,
		(current.recv_bytes-prev.recv_bytes)*10,
		snaploss,
		snapshot_latency*1000.0f, extra_polating?'E':' ',
		prediction_latency*1000.0f,
		mem_allocated()/1024,
		gfx_memory_usage()/1024,
		(int)(1.0f/frametime_avg));
	gfx_quads_text(2, 2, 16, buffer);
	
}

void client_quit()
{
	client_set_state(CLIENTSTATE_QUITING);
}

const char *client_error_string()
{
	return netclient_error_string(net);
}

static void client_render()
{
	gfx_clear(0.0f,0.0f,0.0f);
	modc_render();
	client_debug_render();
}

static void client_error(const char *msg)
{
	dbg_msg("client", "error: %s", msg);
	client_send_error(msg);
	client_set_state(CLIENTSTATE_QUITING);
}

static void client_process_packet(NETPACKET *packet)
{
	if(packet->client_id == -1)
	{
		// connectionlesss
		if(packet->data_size >= (int)sizeof(SERVERBROWSE_LIST) &&
			memcmp(packet->data, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST)) == 0)
		{
			// server listing
			int size = packet->data_size-sizeof(SERVERBROWSE_LIST);
			mem_copy(servers.addresses, (char*)packet->data+sizeof(SERVERBROWSE_LIST), size);
			servers.num = size/sizeof(NETADDR4);

			info_request_begin = 0;
			info_request_end = 0;

			int i;
			for(i = 0; i < servers.num; i++)
			{
				servers.infos[i].num_players = 0;
				servers.infos[i].max_players = 0;
				servers.infos[i].latency = 999;
#if defined(CONF_ARCH_ENDIAN_BIG)
				const char *tmp = (const char *)&servers.addresses[i].port;
				servers.addresses[i].port = (tmp[1]<<8) | tmp[0];
#endif
				sprintf(servers.infos[i].address, "%d.%d.%d.%d:%d",
					servers.addresses[i].ip[0], servers.addresses[i].ip[1], servers.addresses[i].ip[2],
					servers.addresses[i].ip[3], servers.addresses[i].port);
				sprintf(servers.infos[i].name, "%d.%d.%d.%d:%d",
					servers.addresses[i].ip[0], servers.addresses[i].ip[1], servers.addresses[i].ip[2],
					servers.addresses[i].ip[3], servers.addresses[i].port);
			}
		}

		if(packet->data_size >= (int)sizeof(SERVERBROWSE_INFO) &&
			memcmp(packet->data, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
		{
			// we got ze info
			UNPACKER up;
			unpacker_reset(&up, (unsigned char*)packet->data+sizeof(SERVERBROWSE_INFO), packet->data_size-sizeof(SERVERBROWSE_INFO));
			
			if(serverlist_lan)
			{
				if(servers.num != MAX_SERVERS)
				{
					int i = servers.num;
					strncpy(servers.infos[i].name, unpacker_get_string(&up), 128);
					strncpy(servers.infos[i].map, unpacker_get_string(&up), 128);
					servers.infos[i].max_players = unpacker_get_int(&up);
					servers.infos[i].num_players = unpacker_get_int(&up);
					servers.infos[i].latency = 0;
					
					sprintf(servers.infos[i].address, "%d.%d.%d.%d:%d",
						packet->address.ip[0], packet->address.ip[1], packet->address.ip[2],
						packet->address.ip[3], packet->address.port);

					if(config.debug)
						dbg_msg("client", "got server info");
					servers.num++;
					
				}
			}
			else
			{
				int i;
				for(i = 0; i < servers.num; i++)
				{
					if(net_addr4_cmp(&servers.addresses[i], &packet->address) == 0)
					{
						strncpy(servers.infos[i].name, unpacker_get_string(&up), 128);
						strncpy(servers.infos[i].map, unpacker_get_string(&up), 128);
						servers.infos[i].max_players = unpacker_get_int(&up);
						servers.infos[i].num_players = unpacker_get_int(&up);
						servers.infos[i].latency = ((time_get() - servers.request_times[i])*1000)/time_freq();
						if(config.debug)
							dbg_msg("client", "got server info");
						break;
					}
				}
			}
		}
	}
	else
	{
		
		int sys;
		int msg = msg_unpack_start(packet->data, packet->data_size, &sys);
		if(sys)
		{
			// system message
			if(msg == NETMSG_MAP)
			{
				const char *map = msg_unpack_string();
				dbg_msg("client/network", "connection accepted, map=%s", map);
				client_set_state(CLIENTSTATE_LOADING);
				
				if(map_load(map))
				{
					modc_entergame();
					client_send_entergame();
					dbg_msg("client/network", "loading done");
					// now we will wait for two snapshots
					// to finish the connection
				}
				else
				{
					client_error("failure to load map");
				}
			}
			else if(msg == NETMSG_SNAP || msg == NETMSG_SNAPEMPTY) //|| msg == NETMSG_SNAPSMALL || msg == NETMSG_SNAPEMPTY)
			{
				//dbg_msg("client/network", "got snapshot");
				int game_tick = msg_unpack_int();
				int delta_tick = game_tick-msg_unpack_int();
				int input_predtick = msg_unpack_int();
				int time_left = msg_unpack_int();
				int num_parts = 1;
				int part = 0;
				int part_size = 0;
				int crc = 0;
				
				if(msg != NETMSG_SNAPEMPTY)
				{
					crc = msg_unpack_int();
					part_size = msg_unpack_int();
				}
				
				/* TODO: adjust our prediction time */
				if(time_left)
				{
					int k;
					for(k = 0; k < 200; k++) /* TODO: do this better */
					{
						if(inputs[k].tick == input_predtick)
						{
							float wanted_latency = inputs[k].latency - time_left/1000.0f + 0.01f;
							if(wanted_latency > prediction_latency)
								prediction_latency = prediction_latency*0.90f + wanted_latency*0.10f;
							else
								prediction_latency = prediction_latency*0.95f + wanted_latency*0.05f;
							//dbg_msg("DEBUG", "predlatency=%f", prediction_latency);
							break;
						}
					}
				}
				
				//dbg_msg("DEBUG", "new predlatency=%f", prediction_latency);
				
				if(snapshot_part == part && game_tick > current_recv_tick)
				{
					// TODO: clean this up abit
					const char *d = (const char *)msg_unpack_raw(part_size);
					mem_copy((char*)snapshot_incomming_data + part*MAX_SNAPSHOT_PACKSIZE, d, part_size);
					snapshot_part++;
				
					if(snapshot_part == num_parts)
					{
						snapshot_part = 0;
						
						// find snapshot that we should use as delta 
						static SNAPSHOT emptysnap;
						emptysnap.data_size = 0;
						emptysnap.num_items = 0;
						
						SNAPSHOT *deltashot = &emptysnap;
						
						// find delta
						if(delta_tick >= 0)
						{
							int deltashot_size = snapstorage_get(&snapshot_storage, delta_tick, 0, &deltashot);
							
							if(deltashot_size < 0)
							{
								// couldn't find the delta snapshots that the server used
								// to compress this snapshot. force the server to resync
								if(config.debug)
									dbg_msg("client", "error, couldn't find the delta snapshot");
								
								// ack snapshot
								// TODO: combine this with the input message
								msg_pack_start_system(NETMSG_SNAPACK, 0);
								msg_pack_int(-1);
								msg_pack_end();
								client_send_msg();
								return;
							}
						}

						// decompress snapshot
						void *deltadata = snapshot_empty_delta();
						int deltasize = sizeof(int)*3;

						unsigned char tmpbuffer[MAX_SNAPSHOT_SIZE];
						unsigned char tmpbuffer2[MAX_SNAPSHOT_SIZE];
						if(part_size)
						{
							int compsize = zerobit_decompress(snapshot_incomming_data, part_size, tmpbuffer);
							int intsize = intpack_decompress(tmpbuffer, compsize, tmpbuffer2);
							deltadata = tmpbuffer2;
							deltasize = intsize;
						}

						//dbg_msg("UNPACK", "%d unpacked with %d", game_tick, delta_tick);
						
						unsigned char tmpbuffer3[MAX_SNAPSHOT_SIZE];
						int snapsize = snapshot_unpack_delta(deltashot, (SNAPSHOT*)tmpbuffer3, deltadata, deltasize);
						if(snapshot_crc((SNAPSHOT*)tmpbuffer3) != crc)
						{
							if(config.debug)
								dbg_msg("client", "snapshot crc error\n");
							snapcrcerrors++;
							if(snapcrcerrors > 25)
							{
								// to many errors, send reset
								msg_pack_start_system(NETMSG_SNAPACK, 0);
								msg_pack_int(-1);
								msg_pack_end();
								client_send_msg();
								snapcrcerrors = 0;
							}
							return;
						}
						else
						{
							if(snapcrcerrors)
								snapcrcerrors--;
						}

						// purge old snapshots				
						int purgetick = delta_tick;
						if(snapshots[SNAP_PREV] && snapshots[SNAP_PREV]->tick < purgetick)
							purgetick = snapshots[SNAP_PREV]->tick;
						if(snapshots[SNAP_CURRENT] && snapshots[SNAP_CURRENT]->tick < purgetick)
							purgetick = snapshots[SNAP_PREV]->tick;
						snapstorage_purge_until(&snapshot_storage, purgetick);
						//client_snapshot_purge_until(game_tick-50);
						
						// add new
						snapstorage_add(&snapshot_storage, game_tick, time_get(), snapsize, (SNAPSHOT*)tmpbuffer3);
						//SNAPSTORAGE_HOLDER *snap = client_snapshot_add(game_tick, time_get(), tmpbuffer3, snapsize);
						
						//int ncrc = snapshot_crc((snapshot*)tmpbuffer3);
						//if(crc != ncrc)
						//	dbg_msg("client", "client snapshot crc failure %d %d", crc, ncrc);
						
						// apply snapshot, cycle pointers
						recived_snapshots++;
						

						if(current_recv_tick > 0)
							snaploss += game_tick-current_recv_tick-1;
						current_recv_tick = game_tick;
						
						// we got two snapshots until we see us self as connected
						if(recived_snapshots == 2)
						{
							snapshots[SNAP_PREV] = snapshot_storage.first;
							snapshots[SNAP_CURRENT] = snapshot_storage.last;
							local_start_time = time_get();
							client_set_state(CLIENTSTATE_ONLINE);
						}

						int64 now = time_get();
						int64 t = now - game_tick*time_freq()/50;
						if(game_start_time == -1 || t < game_start_time)
						{
							if(config.debug)
								dbg_msg("client", "adjusted time");
							game_start_time = t;
						}
						
						int64 wanted = game_start_time+(game_tick*time_freq())/50;
						float current_latency = (now-wanted)/(float)time_freq();
						snapshot_latency = snapshot_latency*0.95f+current_latency*0.05f;
						
						// ack snapshot
						msg_pack_start_system(NETMSG_SNAPACK, 0);
						msg_pack_int(game_tick);
						msg_pack_end();
						client_send_msg();
					}
				}
				else
				{
					dbg_msg("client", "snapshot reset!");
					snapshot_part = 0;
				}
			}
		}
		else
		{
			// game message
			modc_message(msg);
		}
	}
}


static void client_pump_network()
{
	netclient_update(net);

	// check for errors		
	if(client_state() != CLIENTSTATE_OFFLINE && netclient_state(net) == NETSTATE_OFFLINE)
	{
		client_set_state(CLIENTSTATE_OFFLINE);
		dbg_msg("client", "offline error='%s'", netclient_error_string(net));
	}

	//
	if(client_state() == CLIENTSTATE_CONNECTING && netclient_state(net) == NETSTATE_ONLINE)
	{
		// we switched to online
		dbg_msg("client", "connected, sending info");
		client_set_state(CLIENTSTATE_LOADING);
		client_send_info();
	}
	
	// process packets
	NETPACKET packet;
	while(netclient_recv(net, &packet))
		client_process_packet(&packet);
}

static void client_run(const char *direct_connect_server)
{
	local_start_time = time_get();
	snapshot_part = 0;
	info_request_begin = 0;
	info_request_end = 0;
	
	client_serverbrowse_init();
	
	// init graphics and sound
	if(!gfx_init())
		return;

	snd_init(); // sound is allowed to fail
	
	// load data
	if(!client_load_data())
		return;

	// init menu
	modmenu_init(); // TODO: remove
	
	// init snapshotting
	snap_init();
	
	// init the mod
	modc_init();
	dbg_msg("client", "version %s", modc_net_version());
	
	// open socket
	NETADDR4 bindaddr;
	mem_zero(&bindaddr, sizeof(bindaddr));
	net = netclient_open(bindaddr, 0);
	
	//
	net_host_lookup(config.masterserver, MASTERSERVER_PORT, &master_server);

	// connect to the server if wanted
	if(direct_connect_server)
		client_connect(direct_connect_server);
		
	int64 reporttime = time_get();
	int64 reportinterval = time_freq()*1;
	int frames = 0;
	
	inp_mouse_mode_relative();
	
	while (1)
	{	
		frames++;
		int64 frame_start_time = time_get();

		// switch snapshot
		if(recived_snapshots >= 3)
		{
			int repredict = 0;
			int64 now = time_get();
			while(1)
			{
				SNAPSTORAGE_HOLDER *cur = snapshots[SNAP_CURRENT];
				int64 tickstart = game_start_time + (cur->tick+1)*time_freq()/50;
				int64 t = tickstart;
				if(snapshot_latency > 0)
					t += (int64)(time_freq()*(snapshot_latency*1.1f));

				if(t < now)
				{
					SNAPSTORAGE_HOLDER *next = snapshots[SNAP_CURRENT]->next;
					if(next)
					{
						snapshots[SNAP_PREV] = snapshots[SNAP_CURRENT];
						snapshots[SNAP_CURRENT] = next;
						
						// set tick
						current_tick = snapshots[SNAP_CURRENT]->tick;
						
						if(snapshots[SNAP_CURRENT] && snapshots[SNAP_PREV])
						{
							modc_newsnapshot();
							repredict = 1;
						}
					}
					else
					{
						extra_polating = 1;
						break;
					}
				}
				else
				{
					extra_polating = 0;
					break;
				}
			}
			
			if(snapshots[SNAP_CURRENT] && snapshots[SNAP_PREV])
			{
				int64 curtick_start = game_start_time + (snapshots[SNAP_CURRENT]->tick+1)*time_freq()/50;
				if(snapshot_latency > 0)
					curtick_start += (int64)(time_freq()*(snapshot_latency*1.1f));
					
				int64 prevtick_start = game_start_time + (snapshots[SNAP_PREV]->tick+1)*time_freq()/50;
				if(snapshot_latency > 0)
					prevtick_start += (int64)(time_freq()*(snapshot_latency*1.1f));
					
				intratick = (now - prevtick_start) / (float)(curtick_start-prevtick_start);

				// 25 frames ahead
				int64 last_pred_game_time = 0;
				int64 predicted_game_time = (now - game_start_time) + (int64)(time_freq()*prediction_latency);
				if(predicted_game_time < last_pred_game_time)
					predicted_game_time = last_pred_game_time;
				last_pred_game_time = predicted_game_time;
				
				//int64 predictiontime = game_start_time + time_freq()+ prediction_latency*SERVER_TICK_SPEED
				int new_predtick = (predicted_game_time*SERVER_TICK_SPEED) / time_freq();
				
				int64 predtick_start_time = (new_predtick*time_freq())/SERVER_TICK_SPEED;
				intrapredtick = (predicted_game_time - predtick_start_time)*SERVER_TICK_SPEED/(float)time_freq();
				
				if(new_predtick > current_predtick)
				{
					//dbg_msg("")
					current_predtick = new_predtick;
					repredict = 1;
					
					// send input
					client_send_input();
				}
			}
			
			//intrapredtick = current_predtick
			
			if(repredict)
				modc_predict();
		}

		// STRESS TEST: join the server again
		if(client_state() == CLIENTSTATE_OFFLINE && config.stress && (frames%100) == 0)
			client_connect(config.cl_stress_server);
		
		// update input
		inp_update();
		
		// refocus
		if(!gfx_window_active())
		{
			if(window_must_refocus == 0)
				inp_mouse_mode_absolute();
			window_must_refocus = 1;
		}

		if(window_must_refocus && gfx_window_active())
		{
			if(window_must_refocus < 3)
			{
				inp_mouse_mode_absolute();
				window_must_refocus++;
			}

			if(inp_key_pressed(KEY_MOUSE_1))
			{
				inp_mouse_mode_relative();
				window_must_refocus = 0;
			}
		}

		// screenshot button
		if(inp_key_down(config.key_screenshot))
			gfx_screenshot();

		// panic button
		if(config.debug)
		{
			if(inp_key_pressed(KEY_F1))
				inp_mouse_mode_absolute();
			if(inp_key_pressed(KEY_F2))
				inp_mouse_mode_relative();

			if(inp_key_pressed(KEY_LCTRL) && inp_key_pressed('Q'))
				break;
		
			if(inp_key_pressed(KEY_F5))
			{
				// ack snapshot
				msg_pack_start_system(NETMSG_SNAPACK, 0);
				msg_pack_int(-1);
				msg_pack_end();
				client_send_msg();
			}
		}
			
		// pump the network
		client_pump_network();
		
		// update the server browser
		client_serverbrowse_update();
		
		// render
		if(config.stress)
		{
			if((frames%10) == 0)
			{
				client_render();
				gfx_swap();
			}
		}
		else
		{
			client_render();
			gfx_swap();
		}
		
		// check conditions
		if(client_state() == CLIENTSTATE_QUITING)
			break;

		// be nice
		if(config.cpu_throttle || !gfx_window_active())
			thread_sleep(1);
		
		if(reporttime < time_get())
		{
			if(config.debug)
			{
				dbg_msg("client/report", "fps=%.02f netstate=%d",
					frames/(float)(reportinterval/time_freq()), netclient_state(net));
			}
			frames = 0;
			reporttime += reportinterval;
		}
		
		// update frametime
		frametime = (time_get()-frame_start_time)/(float)time_freq();
	}
	
	modc_shutdown();
	client_disconnect();

	modmenu_shutdown(); // TODO: remove this
	
	gfx_shutdown();
	snd_shutdown();
}


int editor_main(int argc, char **argv);

//client main_client;

int main(int argc, char **argv)
{
	dbg_msg("client", "starting...");
	
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

	const char *direct_connect_server = 0x0;
	snd_set_master_volume(config.volume / 255.0f);
	int editor = 0;

	// init network, need to be done first so we can do lookups
	net_init();

	// parse arguments
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'c' && argv[i][2] == 0 && argc - i > 1)
		{
			// -c SERVER:PORT
			i++;
			direct_connect_server = argv[i];
		}
		else if(argv[i][0] == '-' && argv[i][1] == 'e' && argv[i][2] == 0)
		{
			editor = 1;
		}
		else
			config_set(argv[i]);
	}
	
	if(editor)
		editor_main(argc, argv);
	else
	{
		// start the client
		client_run(direct_connect_server);
	}
	return 0;
}
