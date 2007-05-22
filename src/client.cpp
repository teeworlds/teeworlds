#include <baselib/system.h>
#include <baselib/keys.h>
#include <baselib/mouse.h>
#include <baselib/audio.h>
#include <baselib/stream/file.h>

#include <string.h>
#include <math.h>
#include "interface.h"


#include "packet.h"
#include "snapshot.h"
#include "ui.h"

#include "lzw.h"

#include "versions.h"

using namespace baselib;

// --- string handling (MOVE THESE!!) ---
void snap_encode_string(const char *src, int *dst, int length, int max_length)
{
	const unsigned char *p = (const unsigned char *)src;
	
	// handle whole int
	for(int i = 0; i < length/4; i++)
	{
		*dst = (p[0]<<24|p[1]<<16|p[2]<<8|p[3]);
		p += 4;
		dst++;
	}
	
	// take care of the left overs
	int left = length%4;
	if(left)
	{
		unsigned last = 0;
		switch(left)
		{
			case 3: last |= p[2]<<8;
			case 2: last |= p[1]<<16;
			case 1: last |= p[0]<<24;
		}
		*dst = last;
	}
}

void snap_decode_string(const int *src, char *dst, int max_length)
{
	dbg_assert((max_length%4) == 0, "length must be power of 4");
	for(int i = 0; i < max_length; i++)
		dst[0] = 0;
	
	for(int i = 0; i < max_length/4; i++)
	{
		dst[0] = (*src>>24)&0xff;
		dst[1] = (*src>>16)&0xff;
		dst[2] = (*src>>8)&0xff;
		dst[3] = (*src)&0xff;
		src++;
		dst+=4;
	}
	dst[-1] = 0; // make sure to zero terminate
}

// --- input wrappers ---
static int keyboard_state[2][keys::last];
static int keyboard_current = 0;
static int keyboard_first = 1;

void inp_mouse_relative(int *x, int *y) { mouse::position(x, y); }
int inp_key_pressed(int key) { return keyboard_state[keyboard_current][key]; }
int inp_key_was_pressed(int key) { return keyboard_state[keyboard_current^1][key]; }
int inp_key_down(int key) { return inp_key_pressed(key)&&!inp_key_was_pressed(key); }
int inp_mouse_button_pressed(int button) { return mouse::pressed(button); }

void inp_update()
{
	if(keyboard_first)
	{
		// make sure to reset
		keyboard_first = 0;
		inp_update();
	}
	
	keyboard_current = keyboard_current^1;
	for(int i = 0; i < keys::last; i++)
		keyboard_state[keyboard_current][i] = keys::pressed(i);
}

// --- input snapping ---
static int input_data[MAX_INPUT_SIZE];
static int input_data_size;
void snap_input(void *data, int size)
{
	mem_copy(input_data, data, size);
	input_data_size = size;
}

// -- snapshot handling ---
enum
{
	SNAP_INCOMMING=2,
	NUM_SNAPSHOT_TYPES=3,
};

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
	return (void *)i->data;
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

float snap_intratick()
{
	return (time_get() - snapshot_start_time)/(float)(time_freq()/SERVER_TICK_SPEED);
}

void *snap_find_item(int snapid, int type, int id)
{
	// TODO: linear search. should be fixed.
	for(int i = 0; i < snapshots[snapid]->num_items; i++)
	{
		snapshot::item *itm = snapshots[snapid]->get_item(i);
		if(itm->type() == type && itm->id() == id)
			return (void *)itm->data;
	}
	return 0x0;
}


int menu_loop();
float frametime = 0.0001f;

float client_frametime()
{
	return frametime;
}

// --- client ---
class client
{
public:
	socket_udp4 socket;
	connection conn;
	int64 reconnect_timer;
	
	int snapshot_part;

	int debug_font; // TODO: rfemove this line

	// data to hold three snapshots
	// previous, 
	char name[MAX_NAME_LENGTH];

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
	
	void set_name(const char *new_name)
	{
		mem_zero(name, MAX_NAME_LENGTH);
		strncpy(name, new_name, MAX_NAME_LENGTH);
		name[MAX_NAME_LENGTH-1] = 0;
	}

	void set_fullscreen(bool flag) { fullscreen = flag; }
	
	void send_packet(packet *p)
	{
		conn.send(p);
	}
	
	void send_connect()
	{
		recived_snapshots = 0;
		
		packet p(NETMSG_CLIENT_CONNECT);
		p.write_str(TEEWARS_NETVERSION, 32); // payload
		p.write_str(name,MAX_NAME_LENGTH);
		p.write_str("no clan", MAX_CLANNAME_LENGTH);
		p.write_str("password", 32);
		p.write_str("myskin", 32);
		send_packet(&p);
	}

	void send_done()
	{
		packet p(NETMSG_CLIENT_DONE);
		send_packet(&p);
	}

	void send_error(const char *error)
	{
		packet p(NETMSG_CLIENT_ERROR);
		p.write_str(error, 128);
		send_packet(&p);
		//send_packet(&p);
		//send_packet(&p);
	}	

	void send_input()
	{
		packet p(NETMSG_CLIENT_INPUT);
		
		p.write_int(input_data_size);
		for(int i = 0; i < input_data_size/4; i++)
			p.write_int(input_data[i]);
		send_packet(&p);
	}
	
	void disconnect()
	{
		send_error("disconnected");
		set_state(STATE_OFFLINE);
		map_unload();
	}
	
	void connect(netaddr4 *server_address)
	{
		conn.init(&socket, server_address);
		
		// start by sending connect
		send_connect();
		set_state(STATE_CONNECTING);
		reconnect_timer = time_get()+time_freq();
	}
	
	bool load_data()
	{
		debug_font = gfx_load_texture_tga("data/debug_font.tga");
		return true;
	}
	
	void render()
	{
		gfx_clear(0.0f,0.0f,0.0f);
		
		// this should be moved around abit
		if(get_state() == STATE_ONLINE)
		{
			modc_render();
		}
		else if (get_state() != STATE_CONNECTING && get_state() != STATE_LOADING)
		{
			netaddr4 server_address;
			int status = modmenu_render(&server_address, name, MAX_NAME_LENGTH);

			if (status == -1)
				set_state(STATE_QUIT);
			else if (status)
				connect(&server_address);
		}
		else if (get_state() == STATE_CONNECTING)
		{
			static int64 start = time_get();
			static int tee_texture;
			static int connecting_texture;
			static bool inited = false;
			
			if (!inited)
			{
				tee_texture = gfx_load_texture_tga("data/gui_tee.tga");
				connecting_texture = gfx_load_texture_tga("data/gui/connecting.tga");
					
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
		
		// open socket
		if(!socket.open(0))
		{
			dbg_msg("network/client", "failed to open socket");
			return;
		}

		// connect to the server if wanted
		if (server_address)
			connect(server_address);
		
		int64 inputs_per_second = 50;
		int64 time_per_input = time_freq()/inputs_per_second;
		int64 game_starttime = time_get();
		int64 lastinput = game_starttime;
		
		int64 reporttime = time_get();
		int64 reportinterval = time_freq()*1;
		int frames = 0;
		
		mouse::set_mode(mouse::mode_relative);
		
		while (1)
		{	
			frames++;
			int64 frame_start_time = time_get();
			
			if(time_get()-lastinput > time_per_input)
			{
				if(get_state() == STATE_ONLINE)
					send_input();
				lastinput += time_per_input;
			}
			
			// update input
			inp_update();
			
			//
			if(keys::pressed(keys::f1))
				mouse::set_mode(mouse::mode_absolute);
			if(keys::pressed(keys::f2))
				mouse::set_mode(mouse::mode_relative);
				
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
				unsigned sent, recved;
				conn.counter_get(&sent, &recved);
				dbg_msg("client/report", "fps=%.02f",
					frames/(float)(reportinterval/time_freq()));
				frames = 0;
				reporttime += reportinterval;
				conn.counter_reset();
			}
			
			if (keys::pressed(keys::esc))
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

	void process_packet(packet *p)
	{
		if(p->msg() == NETMSG_SERVER_ACCEPT)
		{
			char map[32];
			p->read_str(map, 32);
			
			if(p->is_good())
			{
				dbg_msg("client/network", "connection accepted, map=%s", map);
				set_state(STATE_LOADING);
				
				if(map_load(map))
				{
					modc_entergame();
					send_done();
					dbg_msg("client/network", "loading done");
					// now we will wait for two snapshots
					// to finish the connection
				}
				else
				{
					error("failure to load map");
				}
			}
		}
		else if(p->msg() == NETMSG_SERVER_SNAP)
		{
			//dbg_msg("client/network", "got snapshot");
			int num_parts = p->read_int();
			int part = p->read_int();
			int part_size = p->read_int();
			
			if(p->is_good())
			{
				if(snapshot_part == part)
				{
					p->read_raw((char*)snapshots[SNAP_INCOMMING] + part*MAX_SNAPSHOT_PACKSIZE, part_size);
					snapshot_part++;
				
					if(snapshot_part == num_parts)
					{
						snapshot *tmp = snapshots[SNAP_PREV];
						snapshots[SNAP_PREV] = snapshots[SNAP_CURRENT];
						snapshots[SNAP_CURRENT] = tmp;

						// decompress snapshot
						lzw_decompress(snapshots[SNAP_INCOMMING], snapshots[SNAP_CURRENT]);
						
						// apply snapshot, cycle pointers
						/*
						snapshot *tmp = snapshots[SNAP_PREV];
						snapshots[SNAP_PREV] = snapshots[SNAP_CURRENT];
						snapshots[SNAP_CURRENT] = snapshots[SNAP_INCOMMING];
						snapshots[SNAP_INCOMMING] = tmp;
						*/
						
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
			dbg_msg("server/client", "unknown packet %x", p->msg());
		}
	}
	
	void pump_network()
	{
		while(1)
		{
			packet p;
			netaddr4 from;
			int bytes = socket.recv(&from, p.data(), p.max_size());
			
			if(bytes <= 0)
				break;
			
			process_packet(&p);
		}
		
		//
		if(get_state() == STATE_CONNECTING && time_get() > reconnect_timer)
		{
			send_connect();
			reconnect_timer = time_get() + time_freq();
		}
	}	
};

int client_main(int argc, char **argv)
{
	dbg_msg("client", "starting...");
	netaddr4 server_address(127, 0, 0, 1, 8303);
	const char *name = "nameless jerk";
	bool connect_at_once = false;
	bool fullscreen = true;

	// init network, need to be done first so we can do lookups
	net_init();

	// parse arguments
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'c' && argv[i][2] == 0 && argc - i > 1)
		{
			// -c SERVER
			i++;
			if(net_host_lookup(argv[i], 8303, &server_address) != 0)
				dbg_msg("main", "could not find the address of %s, connecting to localhost", argv[i]);
			else
				connect_at_once = true;
		}
		else if(argv[i][0] == '-' && argv[i][1] == 'n' && argv[i][2] == 0 && argc - i > 1)
		{
			// -n NAME
			i++;
			name = argv[i];
		}
		else if(argv[i][0] == '-' && argv[i][1] == 'w' && argv[i][2] == 0)
		{
			// -w
			i++;
			fullscreen = false;
		}
	}
	
	// start the server
	client c;
	c.set_name(name);
	c.set_fullscreen(fullscreen);
	c.run(connect_at_once ? &server_address : 0x0);
	return 0;
}
