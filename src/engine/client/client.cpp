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

// --- client ---
// TODO: remove this class
class client
{
public:
	
	//socket_udp4 socket;
	//connection conn;
	int64 reconnect_timer;
	
	int snapshot_part;

	int debug_font; // TODO: rfemove this line

	// data to hold three snapshots
	// previous, 

	bool fullscreen;

	enum
	{
		STATE_OFFLINE,
		STATE_CONNECTING,
		STATE_LOADING,
		STATE_ONLINE,
		STATE_BROKEN,
		STATE_QUIT,
	};
	
	int state;
	int get_state() { return state; }
	void set_state(int s)
	{
		dbg_msg("game", "state change. last=%d current=%d", state, s);
		state = s;
	}

	void set_fullscreen(bool flag) { fullscreen = flag; }
	
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
	
	void connect(netaddr4 *server_address)
	{
		net.connect(server_address);
		set_state(STATE_CONNECTING);
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
			netaddr4 server_address;
			int status = modmenu_render(&server_address);

			if (status == -1)
				set_state(STATE_QUIT);
			else if (status)
				connect(&server_address);
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
	
	void run(netaddr4 *server_address)
	{
		snapshot_part = 0;
		
		// init graphics and sound
		if(!gfx_init(fullscreen))
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
		
		net.open(0);
		
		// open socket
		/*
		if(!socket.open(0))
		{
			dbg_msg("network/client", "failed to open socket");
			return;
		}*/

		// connect to the server if wanted
		if(server_address)
			connect(server_address);
		
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

			if(input::pressed(input::lctrl) && input::pressed('Q'))
				break;
				
			// pump the network
			pump_network();
			
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
				//unsigned sent, recved;
				//conn.counter_get(&sent, &recved);
				dbg_msg("client/report", "fps=%.02f netstate=%d",
					frames/(float)(reportinterval/time_freq()), net.state());
				frames = 0;
				reporttime += reportinterval;
				//conn.counter_reset();
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

	void process_packet(NETPACKET *packet)
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
							//int snapsize = lzw_decompress(snapshots[SNAP_INCOMMING], snapshots[SNAP_CURRENT]);
							int compsize = zerobit_decompress(snapshots[SNAP_INCOMMING], part_size, tmpbuffer);
							//int compsize = lzw_decompress(snapshots[SNAP_INCOMMING],tmpbuffer);
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

int main(int argc, char **argv)
{
	dbg_msg("client", "starting...");
	
	config_reset();
	config_load("teewars.cfg");

	netaddr4 server_address(127, 0, 0, 1, 8303);
	//const char *name = "nameless jerk";
	bool connect_at_once = false;
	bool fullscreen = true;

	// init network, need to be done first so we can do lookups
	net_init();

	// parse arguments
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'c' && argv[i][2] == 0 && argc - i > 1)
		{
			// -c SERVER:PORT
			i++;
			const char *port_str = 0;
			for(int k = 0; argv[i][k]; k++)
			{
				if(argv[i][k] == ':')
				{
					port_str = &(argv[i][k+1]);
					argv[i][k] = 0;
					break;
				}
			}
			int port = 8303;
			if(port_str)
				port = atoi(port_str);
				
			if(net_host_lookup(argv[i], port, &server_address) != 0)
				dbg_msg("main", "could not find the address of %s, connecting to localhost", argv[i]);
			else
				connect_at_once = true;
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
			fullscreen = false;
		}
	}
	
	// start the server
	client c;
	c.set_fullscreen(fullscreen);
	c.run(connect_at_once ? &server_address : 0x0);
	return 0;
}
