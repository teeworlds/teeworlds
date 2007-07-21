#include <baselib/system.h>
#include <baselib/input.h>
#include <baselib/audio.h>
#include <baselib/stream/file.h>

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <engine/interface.h>

#include <engine/packet.h>
#include <engine/snapshot.h>
#include "ui.h"

#include <engine/compression.h>

#include <engine/versions.h>
#include <engine/config.h>
#include <engine/network.h>

#include <mastersrv/mastersrv.h>

using namespace baselib;

// --- input wrappers ---
static int keyboard_state[2][input::last];
static int keyboard_current = 0;
static int keyboard_first = 1;

void inp_mouse_relative(int *x, int *y) { input::mouse_position(x, y); }
int inp_key_pressed(int key) { return keyboard_state[keyboard_current][key]; }
int inp_key_was_pressed(int key) { return keyboard_state[keyboard_current^1][key]; }
int inp_key_down(int key) { return inp_key_pressed(key)&&!inp_key_was_pressed(key); }
int inp_button_pressed(int button) { return input::pressed(button); }

void inp_update()
{
	if(keyboard_first)
	{
		// make sure to reset
		keyboard_first = 0;
		inp_update();
	}
	
	keyboard_current = keyboard_current^1;
	for(int i = 0; i < input::last; i++)
		keyboard_state[keyboard_current][i] = input::pressed(i);
}

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
	SNAP_INCOMMING=2,
	NUM_SNAPSHOT_TYPES=3,
};

static snapshot_storage snapshots_new;
static int current_tick;
static snapshot *snapshots[NUM_SNAPSHOT_TYPES];
static char snapshot_data[NUM_SNAPSHOT_TYPES][MAX_SNAPSHOT_SIZE];
static int recived_snapshots;
static int64 snapshot_start_time;
static int64 local_start_time;

float client_localtime()
{
	return (time_get()-local_start_time)/(float)(time_freq());
}

void *snap_get_item(int snapid, int index, snap_item *item)
{
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	snapshot::item *i = snapshots[snapid]->get_item(index);
	item->type = i->type();
	item->id = i->id();
	return (void *)i->data();
}

int snap_num_items(int snapid)
{
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	return snapshots[snapid]->num_items;
}

static void snap_init()
{
	snapshots[SNAP_INCOMMING] = (snapshot*)snapshot_data[0];
	snapshots[SNAP_CURRENT] = (snapshot*)snapshot_data[1];
	snapshots[SNAP_PREV] = (snapshot*)snapshot_data[2];
	mem_zero(snapshot_data, NUM_SNAPSHOT_TYPES*MAX_SNAPSHOT_SIZE);
	recived_snapshots = 0;
}

float client_intratick()
{
	return (time_get() - snapshot_start_time)/(float)(time_freq()/SERVER_TICK_SPEED);
}

int client_tick()
{
	return current_tick;
}

int client_tickspeed()
{
	return SERVER_TICK_SPEED;
}

void *snap_find_item(int snapid, int type, int id)
{
	// TODO: linear search. should be fixed.
	for(int i = 0; i < snapshots[snapid]->num_items; i++)
	{
		snapshot::item *itm = snapshots[snapid]->get_item(i);
		if(itm->type() == type && itm->id() == id)
			return (void *)itm->data();
	}
	return 0x0;
}


int menu_loop(); // TODO: what is this?
static float frametime = 0.0001f;

float client_frametime()
{
	return frametime;
}

static net_client net;

int client_send_msg()
{
	const msg_info *info = msg_get_info();
	NETPACKET packet;
	packet.client_id = 0;
	packet.data = info->data;
	packet.data_size = info->size;

	if(info->flags&MSGFLAG_VITAL)	
		packet.flags = PACKETFLAG_VITAL;
	
	net.send(&packet);
	return 0;
}

static struct 
{
	server_info infos[MAX_SERVERS];
	int64 request_times[MAX_SERVERS];
	netaddr4 addresses[MAX_SERVERS];
	int num;
} servers;

static netaddr4 master_server;

int client_serverbrowse_getlist(server_info **serverlist)
{
	*serverlist = servers.infos;
	return servers.num;
}

void client_serverbrowse_refresh()
{
	dbg_msg("client", "requesting server list");
	NETPACKET packet;
	packet.client_id = -1;
	packet.address = master_server;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_GETLIST);
	packet.data = SERVERBROWSE_GETLIST;
	net.send(&packet);	
	
	// reset the list
	servers.num = 0;
}

enum
{
	STATE_OFFLINE,
	STATE_CONNECTING,
	STATE_LOADING,
	STATE_ONLINE,
	STATE_BROKEN,
	STATE_QUIT,
};

static netaddr4 server_address;

static int state;
static int get_state() { return state; }
static void set_state(int s)
{
	dbg_msg("game", "state change. last=%d current=%d", state, s);
	state = s;
}


void client_connect(const char *server_address_str)
{
	char buf[512];
	strncpy(buf, server_address_str, 512);
	
	const char *port_str = 0;
	for(int k = 0; buf[k]; k++)
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
	
	net.connect(&server_address);
	set_state(STATE_CONNECTING);	
}

// --- client ---
// TODO: remove this class
class client
{
public:
	int info_request_begin;
	int info_request_end;
	
	int snapshot_part;

	int debug_font; // TODO: rfemove this line

	// data to hold three snapshots
	// previous, 

	void send_info()
	{
		recived_snapshots = 0;

		msg_pack_start_system(NETMSG_INFO, MSGFLAG_VITAL);
		msg_pack_string(config.player_name, 128);
		msg_pack_string(config.clan_name, 128);
		msg_pack_string(config.password, 128);
		msg_pack_string("myskin", 128);
		msg_pack_end();
		client_send_msg();
	}

	void send_entergame()
	{
		msg_pack_start_system(NETMSG_ENTERGAME, MSGFLAG_VITAL);
		msg_pack_end();
		client_send_msg();
	}

	void send_error(const char *error)
	{
		/*
			pack(NETMSG_CLIENT_ERROR, "s", error);
		*/
		/*
		packet p(NETMSG_CLIENT_ERROR);
		p.write_str(error);
		send_packet(&p);
		//send_packet(&p);
		//send_packet(&p);
		*/
	}	

	void send_input()
	{
		msg_pack_start_system(NETMSG_INPUT, 0);
		msg_pack_int(input_data_size);
		for(int i = 0; i < input_data_size/4; i++)
			msg_pack_int(input_data[i]);
		msg_pack_end();
		client_send_msg();
	}
	
	void disconnect()
	{
		send_error("disconnected");
		set_state(STATE_OFFLINE);
		map_unload();
	}
	
	bool load_data()
	{
		debug_font = gfx_load_texture("data/debug_font.png");
		return true;
	}
	
	void debug_render()
	{
		gfx_blend_normal();
		gfx_texture_set(debug_font);
		gfx_mapscreen(0,0,gfx_screenwidth(),gfx_screenheight());
		
		static NETSTATS prev, current;
		static int64 last_snap = 0;
		if(time_get()-last_snap > time_freq()/10)
		{
			last_snap = time_get();
			prev = current;
			net.stats(&current);
		}
		
		char buffer[512];
		sprintf(buffer, "send: %8d recv: %8d",
			(current.send_bytes-prev.send_bytes)*10,
			(current.recv_bytes-prev.recv_bytes)*10);
		gfx_quads_text(10, 10, 16, buffer);
		
	}
	
	void render()
	{
		gfx_clear(0.0f,0.0f,0.0f);
		
		// this should be moved around abit
		// TODO: clean this shit up!
		if(get_state() == STATE_ONLINE)
		{
			modc_render();
			
			// debug render stuff
			debug_render();
			
		}
		else if (get_state() != STATE_CONNECTING && get_state() != STATE_LOADING)
		{
			//netaddr4 server_address;
			int status = modmenu_render();

			if (status == -1)
				set_state(STATE_QUIT);
		}
		else if (get_state() == STATE_CONNECTING || get_state() == STATE_LOADING)
		{
			static int64 start = time_get();
			static int tee_texture;
			static int connecting_texture;
			static bool inited = false;
			
			if (!inited)
			{
				tee_texture = gfx_load_texture("data/gui_tee.png");
				connecting_texture = gfx_load_texture("data/gui/connecting.png");
					
				inited = true;
			}

			gfx_mapscreen(0,0,400.0f,300.0f);

			float t = (time_get() - start) / (double)time_freq();

			float speed = 2*sin(t);

			speed = 1.0f;

			float x = 208 + sin(t*speed) * 32;
			float w = sin(t*speed + 3.149) * 64;

			ui_do_image(tee_texture, x, 95, w, 64);
			ui_do_image(connecting_texture, 88, 150, 256, 64);
		}
	}
	
	void run(const char *direct_connect_server)
	{
		local_start_time = time_get();
		snapshot_part = 0;
		info_request_begin = 0;
		info_request_end = 0;
		
		// init graphics and sound
		if(!gfx_init())
			return;

		snd_init(); // sound is allowed to fail
		
		// load data
		if(!load_data())
			return;

		// init snapshotting
		snap_init();
		
		// init the mod
		modc_init();

		// init menu
		modmenu_init();
		
		// open socket
		net.open(0, 0);
		
		//
		net_host_lookup(MASTERSERVER_ADDRESS, MASTERSERVER_PORT, &master_server);

		// connect to the server if wanted
		if(direct_connect_server)
			client_connect(direct_connect_server);
			
		//int64 inputs_per_second = 50;
		//int64 time_per_input = time_freq()/inputs_per_second;
		int64 game_starttime = time_get();
		int64 last_input = game_starttime;
		
		int64 reporttime = time_get();
		int64 reportinterval = time_freq()*1;
		int frames = 0;
		
		input::set_mouse_mode(input::mode_relative);
		
		while (1)
		{	
			frames++;
			int64 frame_start_time = time_get();

			// send input
			if(get_state() == STATE_ONLINE)
			{
				if(input_is_changed || time_get() > last_input+time_freq())
				{
					send_input();
					input_is_changed = 0;
					last_input = time_get();
				}
			}
			
			// update input
			inp_update();
			
			//
			if(input::pressed(input::f1))
				input::set_mouse_mode(input::mode_absolute);
			if(input::pressed(input::f2))
				input::set_mouse_mode(input::mode_relative);

			// panic button
			if(input::pressed(input::lctrl) && input::pressed('Q'))
				break;
				
			// pump the network
			pump_network();
			
			// update the server browser
			serverbrowse_update();
			
			// render
			render();
			
			// swap the buffers
			gfx_swap();
			
			// check conditions
			if(get_state() == STATE_BROKEN || get_state() == STATE_QUIT)
				break;

			// be nice
			//thread_sleep(1);
			
			if(reporttime < time_get())
			{
				dbg_msg("client/report", "fps=%.02f netstate=%d",
					frames/(float)(reportinterval/time_freq()), net.state());
				frames = 0;
				reporttime += reportinterval;
			}
			
			if (input::pressed(input::esc))
				if (get_state() == STATE_CONNECTING || get_state() == STATE_ONLINE)
					disconnect();

			// update frametime
			frametime = (time_get()-frame_start_time)/(float)time_freq();
		}
		
		modc_shutdown();
		disconnect();

		modmenu_shutdown();
		
		gfx_shutdown();
		snd_shutdown();
	}
	
	void error(const char *msg)
	{
		dbg_msg("game", "error: %s", msg);
		send_error(msg);
		set_state(STATE_BROKEN);
	}
	
	void serverbrowse_request(int id)
	{
		dbg_msg("client", "requesting server info from %d.%d.%d.%d:%d",
			servers.addresses[id].ip[0], servers.addresses[id].ip[1], servers.addresses[id].ip[2],
			servers.addresses[id].ip[3], servers.addresses[id].port);
		NETPACKET packet;
		packet.client_id = -1;
		packet.address = servers.addresses[id];
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = sizeof(SERVERBROWSE_GETINFO);
		packet.data = SERVERBROWSE_GETINFO;
		net.send(&packet);
		servers.request_times[id] = time_get();
	}
	
	void serverbrowse_update()
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
			serverbrowse_request(info_request_end);
			info_request_end++;
		}
	}

	void process_packet(NETPACKET *packet)
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

				for(int i = 0; i < servers.num; i++)
				{
					servers.infos[i].num_players = 0;
					servers.infos[i].max_players = 0;
					servers.infos[i].latency = 999;
					sprintf(servers.infos[i].address, "%d.%d.%d.%d:%d",
						servers.addresses[i].ip[0], servers.addresses[i].ip[1], servers.addresses[i].ip[2],
						servers.addresses[i].ip[3], servers.addresses[i].port);
				}
			}

			if(packet->data_size >= (int)sizeof(SERVERBROWSE_INFO) &&
				memcmp(packet->data, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
			{
				// we got ze info
				data_unpacker unpacker;
				unpacker.reset((unsigned char*)packet->data+sizeof(SERVERBROWSE_INFO), packet->data_size-sizeof(SERVERBROWSE_INFO));
				
				for(int i = 0; i < servers.num; i++)
				{
					if(net_addr4_cmp(&servers.addresses[i], &packet->address) == 0)
					{
						strncpy(servers.infos[i].name, unpacker.get_string(), 128);
						strncpy(servers.infos[i].map, unpacker.get_string(), 128);
						servers.infos[i].max_players = unpacker.get_int();
						servers.infos[i].num_players = unpacker.get_int();
						servers.infos[i].latency = ((time_get() - servers.request_times[i])*1000)/time_freq();
						dbg_msg("client", "got server info");
						break;
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
					set_state(STATE_LOADING);
					
					if(map_load(map))
					{
						modc_entergame();
						send_entergame();
						dbg_msg("client/network", "loading done");
						// now we will wait for two snapshots
						// to finish the connection
					}
					else
					{
						error("failure to load map");
					}
				}
				else if(msg == NETMSG_SNAP || msg == NETMSG_SNAPSMALL || msg == NETMSG_SNAPEMPTY)
				{
					//dbg_msg("client/network", "got snapshot");
					int game_tick = msg_unpack_int();
					int delta_tick = game_tick-msg_unpack_int();
					int num_parts = 1;
					int part = 0;
					int part_size = 0;
					
					if(msg == NETMSG_SNAP)
					{
						num_parts = msg_unpack_int();
						part = msg_unpack_int();
					}
					
					if(msg != NETMSG_SNAPEMPTY)
						part_size = msg_unpack_int();
					
					if(snapshot_part == part)
					{
						// TODO: clean this up abit
						const char *d = (const char *)msg_unpack_raw(part_size);
						mem_copy((char*)snapshots[SNAP_INCOMMING] + part*MAX_SNAPSHOT_PACKSIZE, d, part_size);
						snapshot_part++;
					
						if(snapshot_part == num_parts)
						{
							snapshot *tmp = snapshots[SNAP_PREV];
							snapshots[SNAP_PREV] = snapshots[SNAP_CURRENT];
							snapshots[SNAP_CURRENT] = tmp;
							current_tick = game_tick;

							// decompress snapshot
							void *deltadata = snapshot_empty_delta();
							int deltasize = sizeof(int)*3;

							unsigned char tmpbuffer[MAX_SNAPSHOT_SIZE];
							unsigned char tmpbuffer2[MAX_SNAPSHOT_SIZE];
							if(part_size)
							{
								int compsize = zerobit_decompress(snapshots[SNAP_INCOMMING], part_size, tmpbuffer);
								int intsize = intpack_decompress(tmpbuffer, compsize, tmpbuffer2);
								deltadata = tmpbuffer2;
								deltasize = intsize;
							}

							// find snapshot that we should use as delta 
							static snapshot emptysnap;
							emptysnap.data_size = 0;
							emptysnap.num_items = 0;
							
							snapshot *deltashot = &emptysnap;
							int deltashot_size;

							if(delta_tick >= 0)
							{
								void *delta_data;
								deltashot_size = snapshots_new.get(delta_tick, &delta_data);
								if(deltashot_size >= 0)
								{
									deltashot = (snapshot *)delta_data;
								}
								else
								{
									// TODO: handle this
									dbg_msg("client", "error, couldn't find the delta snapshot");
								}
							}

							int snapsize = snapshot_unpack_delta(deltashot, (snapshot*)snapshots[SNAP_CURRENT], deltadata, deltasize);
							//snapshot *shot = (snapshot *)snapshots[SNAP_CURRENT];

							// purge old snapshots					
							snapshots_new.purge_until(delta_tick);
							snapshots_new.purge_until(game_tick-50); // TODO: change this to server tickrate
							
							// add new
							snapshots_new.add(game_tick, snapsize, snapshots[SNAP_CURRENT]);
							
							// apply snapshot, cycle pointers
							recived_snapshots++;
							snapshot_start_time = time_get();
							
							// we got two snapshots until we see us self as connected
							if(recived_snapshots == 2)
							{
								local_start_time = time_get();
								set_state(STATE_ONLINE);
							}
							
							if(recived_snapshots > 2)
								modc_newsnapshot();
							
							snapshot_part = 0;
							
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
	
	void pump_network()
	{
		net.update();

		// check for errors		
		if(get_state() != STATE_OFFLINE && net.state() == NETSTATE_OFFLINE)
		{
			// TODO: add message to the user there
			set_state(STATE_OFFLINE);
		}

		//
		if(get_state() == STATE_CONNECTING && net.state() == NETSTATE_ONLINE)
		{
			// we switched to online
			dbg_msg("client", "connected, sending info");
			set_state(STATE_LOADING);
			send_info();
		}
		
		// process packets
		NETPACKET packet;
		while(net.recv(&packet))
			process_packet(&packet);
	}	
};

int editor_main(int argc, char **argv);

int main(int argc, char **argv)
{
	dbg_msg("client", "starting...");
	
	config_reset();
	config_load("teewars.cfg");

	const char *direct_connect_server = 0x0;
	snd_set_master_volume(config.volume / 255.0f);
	bool editor = false;

	// init network, need to be done first so we can do lookups
	net_init();

	// parse arguments
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'c' && argv[i][2] == 0 && argc - i > 1)
		{
			// -c SERVER:PORT
			i++;
			direct_connect_server = argv[i];
		}
		else if(argv[i][0] == '-' && argv[i][1] == 'n' && argv[i][2] == 0 && argc - i > 1)
		{
			// -n NAME
			i++;
			config_set_player_name(&config, argv[i]);
		}
		else if(argv[i][0] == '-' && argv[i][1] == 'w' && argv[i][2] == 0)
		{
			// -w
			config.fullscreen = 0;
		}
		else if(argv[i][0] == '-' && argv[i][1] == 'e' && argv[i][2] == 0)
		{
			editor = true;
		}
	}
	
	if(editor)
		editor_main(argc, argv);
	else
	{
		// start the client
		client c;
		c.run(direct_connect_server);
	}
	return 0;
}
