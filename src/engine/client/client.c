
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <engine/system.h>
#include <engine/engine.h>
#include <engine/interface.h>
#include "ui.h"

#include <engine/protocol.h>
#include <engine/snapshot.h>
#include <engine/compression.h>
#include <engine/network.h>
#include <engine/config.h>
#include <engine/packer.h>
#include <engine/memheap.h>

#include <mastersrv/mastersrv.h>

const int prediction_margin = 10; /* magic network prediction value */

/*
	Server Time
	Client Mirror Time
	Client Predicted Time
	
	Snapshot Latency
		Downstream latency
	
	Prediction Latency
		Upstream latency
*/

/* network client, must be accessible from other parts like the server browser */
NETCLIENT *net;

/* TODO: ugly, fix me */
extern void client_serverbrowse_set(NETADDR4 *addr, int request, SERVER_INFO *info);


static int snapshot_part;
static int64 local_start_time;

static int debug_font;
static float frametime = 0.0001f;
static NETADDR4 server_address;
static int window_must_refocus = 0;
static int snaploss = 0;
static int snapcrcerrors = 0;

static int ack_game_tick = -1;
static int current_recv_tick = 0;

/* current time */
static int current_tick = 0;
static float intratick = 0;

/* predicted time */
static int current_predtick = 0;
static float intrapredtick = 0;

static struct /* TODO: handle input better */
{
	int data[MAX_INPUT_SIZE]; /* the input data */
	int tick; /* the tick that the input is for */
	int64 game_time; /* prediction latency when we sent this input */
	int64 time;
} inputs[200];
static int current_input = 0;

enum
{
	GRAPH_MAX=128
};

typedef struct
{
	float min, max;
	float values[GRAPH_MAX];
	int index;
} GRAPH;

static void graph_init(GRAPH *g, float min, float max)
{
	g->min = min;
	g->max = max;
	g->index = 0;
}

static void graph_add(GRAPH *g, float v)
{
	g->index = (g->index+1)&(GRAPH_MAX-1);
	g->values[g->index] = v;
}

static void graph_render(GRAPH *g, float x, float y, float w, float h)
{
	int i;
	gfx_texture_set(-1);
	
	gfx_quads_begin();
	gfx_setcolor(0, 0, 0, 1);
	gfx_quads_drawTL(x, y, w, h);
	gfx_quads_end();
		
	gfx_lines_begin();
	gfx_setcolor(0.95f, 0.95f, 0.95f, 1);
	gfx_lines_draw(x, y+h/2, x+w, y+h/2);
	gfx_setcolor(0.5f, 0.5f, 0.5f, 1);
	gfx_lines_draw(x, y+(h*3)/4, x+w, y+(h*3)/4);
	gfx_lines_draw(x, y+h/4, x+w, y+h/4);
	for(i = 1; i < GRAPH_MAX; i++)
	{
		float a0 = (i-1)/(float)GRAPH_MAX;
		float a1 = i/(float)GRAPH_MAX;
		int i0 = (g->index+i-1)&(GRAPH_MAX-1);
		int i1 = (g->index+i)&(GRAPH_MAX-1);
		
		float v0 = g->values[i0];
		float v1 = g->values[i1];
		
		gfx_setcolor(0, 1, 0, 1);
		gfx_lines_draw(x+a0*w, y+h-v0*h, x+a1*w, y+h-v1*h);
	}
	gfx_lines_end();
}

typedef struct
{
	int64 snap;
	int64 current;
	int64 target;
	
	int64 rlast;
	int64 tlast;
	GRAPH graph;
} SMOOTHTIME;

static void st_init(SMOOTHTIME *st, int64 target)
{
	st->snap = time_get();
	st->current = target;
	st->target = target;
	graph_init(&st->graph, 0.0f, 1.0f);
}

static int64 st_get(SMOOTHTIME *st, int64 now)
{
	float adjust_speed, a;
	int64 c = st->current + (now - st->snap);
	int64 t = st->target + (now - st->snap);
	int64 r;
	
	/* it's faster to adjust upward instead of downward */
	/* we might need to adjust these abit */
	adjust_speed = 0.25f; /*0.99f;*/
	if(t > c)
		adjust_speed = 500.0f;
	
	a = ((now-st->snap)/(float)time_freq()) * adjust_speed;
	if(a > 1.0f)
		a = 1.0f;
		
	r = c + (int64)((t-c)*a);
	
	graph_add(&st->graph, a+0.5f);
	
	return r;
}

static void st_update(SMOOTHTIME *st, int64 target)
{
	int64 now = time_get();
	st->current = st_get(st, now);
	st->snap = now;
	st->target = target;
}

SMOOTHTIME game_time;
SMOOTHTIME predicted_time;

GRAPH intra_graph;
GRAPH predict_graph;

/* --- input snapping --- */
static int input_data[MAX_INPUT_SIZE] = {0};
static int input_data_size;
static int input_is_changed = 1;
static GRAPH input_late_graph;
void snap_input(void *data, int size)
{
	if(input_data_size != size || memcmp(input_data, data, size))
		input_is_changed = 1;
	mem_copy(input_data, data, size);
	input_data_size = size;
}

/* -- snapshot handling --- */
enum
{
	NUM_SNAPSHOT_TYPES=2
};

SNAPSTORAGE snapshot_storage;
static SNAPSTORAGE_HOLDER *snapshots[NUM_SNAPSHOT_TYPES];
static int recived_snapshots;
static char snapshot_incomming_data[MAX_SNAPSHOT_SIZE];

/* --- */

const void *snap_get_item(int snapid, int index, SNAP_ITEM *item)
{
	SNAPSHOT_ITEM *i;
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	i = snapshot_get_item(snapshots[snapid]->snap, index);
	item->type = snapitem_type(i);
	item->id = snapitem_id(i);
	return (void *)snapitem_data(i);
}

const void *snap_find_item(int snapid, int type, int id)
{
	/* TODO: linear search. should be fixed. */
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

/* ------ time functions ------ */
float client_intratick() { return intratick; }
float client_intrapredtick() { return intrapredtick; }
int client_tick() { return current_tick; }
int client_predtick() { return current_predtick; }
int client_tickspeed() { return SERVER_TICK_SPEED; }
float client_frametime() { return frametime; }
float client_localtime() { return (time_get()-local_start_time)/(float)(time_freq()); }

/* ----- send functions ----- */
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
	msg_pack_start_system(NETMSG_INFO, MSGFLAG_VITAL);
	msg_pack_string(modc_net_version(), 128);
	msg_pack_string(config.player_name, 128);
	msg_pack_string(config.clan_name, 128);
	msg_pack_string(config.password, 128);
	msg_pack_end();
	client_send_msg();
}


static void client_send_entergame()
{
	msg_pack_start_system(NETMSG_ENTERGAME, MSGFLAG_VITAL);
	msg_pack_end();
	client_send_msg();
}

static void client_send_ready()
{
	msg_pack_start_system(NETMSG_READY, MSGFLAG_VITAL);
	msg_pack_end();
	client_send_msg();
}

static void client_send_error(const char *error)
{
	/*
	packet p(NETMSG_CLIENT_ERROR);
	p.write_str(error);
	send_packet(&p);
	*/
}

void client_rcon(const char *cmd)
{
	msg_pack_start_system(NETMSG_CMD, MSGFLAG_VITAL);
	msg_pack_string(config.rcon_password, 32);
	msg_pack_string(cmd, 256);
	msg_pack_end();
	client_send_msg();
}

static void client_send_input()
{
	int64 now = time_get();	
	int i;

	if(current_predtick <= 0)
		return;
		
	msg_pack_start_system(NETMSG_INPUT, 0);
	msg_pack_int(ack_game_tick);
	msg_pack_int(current_predtick);
	msg_pack_int(input_data_size);

	inputs[current_input].tick = current_predtick;
	inputs[current_input].game_time = st_get(&predicted_time, now);
	inputs[current_input].time = now;
	
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

/* ------ state handling ----- */
static int state = CLIENTSTATE_OFFLINE;
int client_state() { return state; }
static void client_set_state(int s)
{
	int old = state;
	if(config.debug)
		dbg_msg("client", "state change. last=%d current=%d", state, s);
	state = s;
	if(old != s)
		modc_statechange(state, old);
}

/* called when the map is loaded and we should init for a new round */
static void client_on_enter_game()
{
	/* reset input */
	int i;
	for(i = 0; i < 200; i++)
		inputs[i].tick = -1;
	current_input = 0;

	/* reset snapshots */
	snapshots[SNAP_CURRENT] = 0;
	snapshots[SNAP_PREV] = 0;
	snapstorage_purge_all(&snapshot_storage);
	recived_snapshots = 0;
	current_predtick = 0;
	current_recv_tick = 0;
}

void client_entergame()
{
	/* now we will wait for two snapshots */
	/* to finish the connection */
	client_send_entergame();
	client_on_enter_game();
}

void client_connect(const char *server_address_str)
{
	char buf[512];
	const char *port_str = 0;
	int k;
	int port = 8303;

	dbg_msg("client", "connecting to '%s'", server_address_str);

	strncpy(buf, server_address_str, 512);

	for(k = 0; buf[k]; k++)
	{
		if(buf[k] == ':')
		{
			port_str = &(buf[k+1]);
			buf[k] = 0;
			break;
		}
	}
	
	if(port_str)
		port = atoi(port_str);
		
	if(net_host_lookup(buf, port, &server_address) != 0)
		dbg_msg("client", "could not find the address of %s, connecting to localhost", buf);
	
	netclient_connect(net, &server_address);
	client_set_state(CLIENTSTATE_CONNECTING);
	
	graph_init(&intra_graph, 0.0f, 1.0f);
	graph_init(&input_late_graph, 0.0f, 1.0f);
	graph_init(&predict_graph, 0.0f, 200.0f);
	
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
	static NETSTATS prev, current;
	static int64 last_snap = 0;
	static float frametime_avg = 0;
	char buffer[512];
	
	if(!config.debug)
		return;
	
	gfx_blend_normal();
	gfx_texture_set(debug_font);
	gfx_mapscreen(0,0,gfx_screenwidth(),gfx_screenheight());
	
	if(time_get()-last_snap > time_freq()/10)
	{
		last_snap = time_get();
		prev = current;
		netclient_stats(net, &current);
	}
	
	frametime_avg = frametime_avg*0.9f + frametime*0.1f;
	sprintf(buffer, "ticks: %8d %8d send: %6d recv: %6d snaploss: %d  mem %dk   gfxmem: %dk  fps: %3d",
		current_tick, current_predtick,
		(current.send_bytes-prev.send_bytes)*10,
		(current.recv_bytes-prev.recv_bytes)*10,
		snaploss,
		mem_allocated()/1024,
		gfx_memory_usage()/1024,
		(int)(1.0f/frametime_avg));
	gfx_quads_text(2, 2, 16, buffer);
	
	/* render graphs */
	gfx_mapscreen(0,0,400.0f,300.0f);
	graph_render(&predict_graph, 300, 10, 90, 50);
	graph_render(&predicted_time.graph, 300, 10+50+10, 90, 50);
	
	graph_render(&intra_graph, 300, 10+50+10+50+10, 90, 50);
	graph_render(&input_late_graph, 300, 10+50+10+50+10+50+10, 90, 50);
	
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
	client_set_state(CLIENTSTATE_OFFLINE);
}

static void client_process_packet(NETPACKET *packet)
{
	if(packet->client_id == -1)
	{
		/* connectionlesss */
		if(packet->data_size >= (int)sizeof(SERVERBROWSE_LIST) &&
			memcmp(packet->data, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST)) == 0)
		{
			int size = packet->data_size-sizeof(SERVERBROWSE_LIST);
			int num = size/sizeof(NETADDR4);
			NETADDR4 *addrs = (NETADDR4 *)((char*)packet->data+sizeof(SERVERBROWSE_LIST));
			int i;
			
			for(i = 0; i < num; i++)
			{
				NETADDR4 addr = addrs[i];
				SERVER_INFO info = {0};

#if defined(CONF_ARCH_ENDIAN_BIG)
				const char *tmp = (const char *)&addr.port;
				addr.port = (tmp[1]<<8) | tmp[0];
#endif
				
				info.latency = 999;
				sprintf(info.address, "%d.%d.%d.%d:%d",
					addr.ip[0], addr.ip[1], addr.ip[2],
					addr.ip[3], addr.port);
				sprintf(info.name, "\255%d.%d.%d.%d:%d", /* the \255 is to make sure that it's sorted last */
					addr.ip[0], addr.ip[1], addr.ip[2],
					addr.ip[3], addr.port);
				
				client_serverbrowse_set(addrs+i, 1, &info);
			}
		}

		if(packet->data_size >= (int)sizeof(SERVERBROWSE_INFO) &&
			memcmp(packet->data, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
		{
			/* we got ze info */
			UNPACKER up;
			SERVER_INFO info = {0};
			int i;

			unpacker_reset(&up, (unsigned char*)packet->data+sizeof(SERVERBROWSE_INFO), packet->data_size-sizeof(SERVERBROWSE_INFO));

			strncpy(info.version, unpacker_get_string(&up), 32);
			strncpy(info.name, unpacker_get_string(&up), 64);
			strncpy(info.map, unpacker_get_string(&up), 32);
			info.game_type = atol(unpacker_get_string(&up));
			info.flags = atol(unpacker_get_string(&up));
			info.progression = atol(unpacker_get_string(&up));
			info.num_players = atol(unpacker_get_string(&up));
			info.max_players = atol(unpacker_get_string(&up));
			sprintf(info.address, "%d.%d.%d.%d:%d",
				packet->address.ip[0], packet->address.ip[1], packet->address.ip[2],
				packet->address.ip[3], packet->address.port);
			
			for(i = 0; i < info.num_players; i++)
			{
				strncpy(info.player_names[i], unpacker_get_string(&up), 48);
				info.player_scores[i] = atol(unpacker_get_string(&up));
			}
			
			/* TODO: unpack players aswell */
			client_serverbrowse_set(&packet->address, 0, &info);
		}
	}
	else
	{
		int sys;
		int msg = msg_unpack_start(packet->data, packet->data_size, &sys);

		if(sys)
		{
			/* system message */
			if(msg == NETMSG_MAP)
			{
				const char *map = msg_unpack_string();
				dbg_msg("client/network", "connection accepted, map=%s", map);
				client_set_state(CLIENTSTATE_LOADING);
				
				if(map_load(map))
				{
					dbg_msg("client/network", "loading done");
					client_send_ready();
					modc_connected();
				}
				else
				{
					client_error("failure to load map");
				}
			}
			else if(msg == NETMSG_SNAP || msg == NETMSG_SNAPEMPTY)
			{
				/*dbg_msg("client/network", "got snapshot"); */
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
					
					graph_add(&input_late_graph, time_left/100.0f+0.5f);
					
					if(time_left < 0)
						dbg_msg("client", "input was late with %d ms", time_left);
					
					for(k = 0; k < 200; k++) /* TODO: do this better */
					{
						if(inputs[k].tick == input_predtick)
						{
							/*-1000/50 prediction_margin */
							int64 target = inputs[k].game_time + (time_get() - inputs[k].time);
							st_update(&predicted_time, target - (int64)(((time_left-prediction_margin)/1000.0f)*time_freq()));
							break;
						}
					}
				}
				
				if(snapshot_part == part && game_tick > current_recv_tick)
				{
					/* TODO: clean this up abit */
					const char *d = (const char *)msg_unpack_raw(part_size);
					mem_copy((char*)snapshot_incomming_data + part*MAX_SNAPSHOT_PACKSIZE, d, part_size);
					snapshot_part++;
				
					if(snapshot_part == num_parts)
					{
						static SNAPSHOT emptysnap;
						SNAPSHOT *deltashot = &emptysnap;
						int purgetick;
						void *deltadata;
						int deltasize;
						unsigned char tmpbuffer[MAX_SNAPSHOT_SIZE];
						unsigned char tmpbuffer2[MAX_SNAPSHOT_SIZE];
						unsigned char tmpbuffer3[MAX_SNAPSHOT_SIZE];
						int snapsize;

						snapshot_part = 0;
						
						/* find snapshot that we should use as delta */
						emptysnap.data_size = 0;
						emptysnap.num_items = 0;
						
						/* find delta */
						if(delta_tick >= 0)
						{
							int deltashot_size = snapstorage_get(&snapshot_storage, delta_tick, 0, &deltashot);
							
							if(deltashot_size < 0)
							{
								/* couldn't find the delta snapshots that the server used */
								/* to compress this snapshot. force the server to resync */
								if(config.debug)
									dbg_msg("client", "error, couldn't find the delta snapshot");
								
								/* ack snapshot */
								/* TODO: combine this with the input message */
								ack_game_tick = -1;
								return;
							}
						}

						/* decompress snapshot */
						deltadata = snapshot_empty_delta();
						deltasize = sizeof(int)*3;

						if(part_size)
						{
							int compsize = zerobit_decompress(snapshot_incomming_data, part_size, tmpbuffer);
							int intsize = intpack_decompress(tmpbuffer, compsize, tmpbuffer2);
							deltadata = tmpbuffer2;
							deltasize = intsize;
						}

						/*dbg_msg("UNPACK", "%d unpacked with %d", game_tick, delta_tick); */
						
						purgetick = delta_tick;
						snapsize = snapshot_unpack_delta(deltashot, (SNAPSHOT*)tmpbuffer3, deltadata, deltasize);
						if(msg != NETMSG_SNAPEMPTY && snapshot_crc((SNAPSHOT*)tmpbuffer3) != crc)
						{
							if(config.debug)
								dbg_msg("client", "snapshot crc error %d", snapcrcerrors);
							snapcrcerrors++;
							if(snapcrcerrors > 10)
							{
								/* to many errors, send reset */
								ack_game_tick = -1;
								client_send_input();
								snapcrcerrors = 0;
							}
							return;
						}
						else
						{
							if(snapcrcerrors)
								snapcrcerrors--;
						}

						/* purge old snapshots */
						purgetick = delta_tick;
						if(snapshots[SNAP_PREV] && snapshots[SNAP_PREV]->tick < purgetick)
							purgetick = snapshots[SNAP_PREV]->tick;
						if(snapshots[SNAP_CURRENT] && snapshots[SNAP_CURRENT]->tick < purgetick)
							purgetick = snapshots[SNAP_PREV]->tick;
						snapstorage_purge_until(&snapshot_storage, purgetick);
						/*client_snapshot_purge_until(game_tick-50); */
						
						/* add new */
						snapstorage_add(&snapshot_storage, game_tick, time_get(), snapsize, (SNAPSHOT*)tmpbuffer3);
						
						/* apply snapshot, cycle pointers */
						recived_snapshots++;
						

						if(current_recv_tick > 0)
							snaploss += game_tick-current_recv_tick-1;
						current_recv_tick = game_tick;
						
						/* we got two snapshots until we see us self as connected */
						if(recived_snapshots == 2)
						{
							/* start at 200ms and work from there */
							st_init(&predicted_time, game_tick*time_freq()/50);
							st_init(&game_time, (game_tick-1)*time_freq()/50);
							snapshots[SNAP_PREV] = snapshot_storage.first;
							snapshots[SNAP_CURRENT] = snapshot_storage.last;
							local_start_time = time_get();
							client_set_state(CLIENTSTATE_ONLINE);
						}
						
						{
							int64 now = time_get();
							graph_add(&predict_graph, (st_get(&predicted_time, now)-st_get(&game_time, now))/(float)time_freq());
						}
						
						st_update(&game_time, (game_tick-1)*time_freq()/50);
						
						/* ack snapshot */
						ack_game_tick = game_tick;
					}
				}
				else
				{
					dbg_msg("client", "snapsht reset!");
					snapshot_part = 0;
				}
			}
		}
		else
		{
			/* game message */
			modc_message(msg);
		}
	}
}

static void client_pump_network()
{
	NETPACKET packet;

	netclient_update(net);

	/* check for errors */
	if(client_state() != CLIENTSTATE_OFFLINE && netclient_state(net) == NETSTATE_OFFLINE)
	{
		client_set_state(CLIENTSTATE_OFFLINE);
		dbg_msg("client", "offline error='%s'", netclient_error_string(net));
	}

	/* */
	if(client_state() == CLIENTSTATE_CONNECTING && netclient_state(net) == NETSTATE_ONLINE)
	{
		/* we switched to online */
		dbg_msg("client", "connected, sending info");
		client_set_state(CLIENTSTATE_LOADING);
		client_send_info();
	}
	
	/* process packets */
	while(netclient_recv(net, &packet))
		client_process_packet(&packet);
}

static void client_run()
{
	NETADDR4 bindaddr;
	int64 reporttime = time_get();
	int64 reportinterval = time_freq()*1;
	int frames = 0;

	local_start_time = time_get();
	snapshot_part = 0;
	
	/* init graphics and sound */
	if(!gfx_init())
		return;

	/* sound is allowed to fail */
	snd_init();
	
	/* load data */
	if(!client_load_data())
		return;

	/* init the mod */
	modc_init();
	dbg_msg("client", "version %s", modc_net_version());
	
	/* open socket */
	mem_zero(&bindaddr, sizeof(bindaddr));
	net = netclient_open(bindaddr, 0);
	
	/* connect to the server if wanted */
	if(config.cl_connect[0] != 0)
		client_connect(config.cl_connect);
		
	inp_mouse_mode_relative();
	
	while (1)
	{	
		int64 frame_start_time = time_get();
		frames++;

		/* switch snapshot */
		if(recived_snapshots >= 3)
		{
			int repredict = 0;
			int64 now = st_get(&game_time, time_get());

			while(1)
			{
				SNAPSTORAGE_HOLDER *cur = snapshots[SNAP_CURRENT];
				int64 tickstart = (cur->tick)*time_freq()/50;

				if(tickstart < now)
				{
					SNAPSTORAGE_HOLDER *next = snapshots[SNAP_CURRENT]->next;
					if(next)
					{
						snapshots[SNAP_PREV] = snapshots[SNAP_CURRENT];
						snapshots[SNAP_CURRENT] = next;
						
						/* set tick */
						current_tick = snapshots[SNAP_CURRENT]->tick;
						
						if(snapshots[SNAP_CURRENT] && snapshots[SNAP_PREV])
						{
							modc_newsnapshot();
							repredict = 1;
						}
					}
					else
						break;
				}
				else
					break;
			}

			if(snapshots[SNAP_CURRENT] && snapshots[SNAP_PREV])
			{
				int64 curtick_start = (snapshots[SNAP_CURRENT]->tick)*time_freq()/50;
				int64 prevtick_start = (snapshots[SNAP_PREV]->tick)*time_freq()/50;
				int64 pred_now = st_get(&predicted_time, time_get());
				/*tg_add(&predicted_time_graph, pred_now, 0); */
				int prev_pred_tick = (int)(pred_now*50/time_freq());
				int new_pred_tick = prev_pred_tick+1;

				intratick = (now - prevtick_start) / (float)(curtick_start-prevtick_start);

				graph_add(&intra_graph, intratick*0.25f);

				curtick_start = new_pred_tick*time_freq()/50;
				prevtick_start = prev_pred_tick*time_freq()/50;
				intrapredtick = (pred_now - prevtick_start) / (float)(curtick_start-prevtick_start);
				
				if(new_pred_tick > current_predtick)
				{
					current_predtick = new_pred_tick;
					repredict = 1;
					
					/* send input */
					client_send_input();
				}
			}

			/* only do sane predictions */
			if(repredict)
			{
				if(current_predtick > current_tick && current_predtick < current_tick+50)
					modc_predict();
			}
		}

		/* STRESS TEST: join the server again */
		if(client_state() == CLIENTSTATE_OFFLINE && config.stress && (frames%100) == 0)
			client_connect(config.cl_stress_server);
		
		/* update input */
		inp_update();
		
		/* refocus */
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

		/* screenshot button */
		if(inp_key_down(config.key_screenshot))
			gfx_screenshot();

		/* some debug keys */
		if(config.debug)
		{
			if(inp_key_pressed(KEY_F1))
				inp_mouse_mode_absolute();
			if(inp_key_pressed(KEY_F2))
				inp_mouse_mode_relative();

			if(inp_key_pressed(KEY_F5))
			{
				ack_game_tick = -1;
				client_send_input();
			}
		}

		/* panic quit button */
		if(inp_key_pressed(KEY_LCTRL) && inp_key_pressed(KEY_LSHIFT) && inp_key_pressed('Q'))
			break;
			
		/* pump the network */
		client_pump_network();
		
		/* update the server browser */
		client_serverbrowse_update();
		
		/* render */
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
		
		/* check conditions */
		if(client_state() == CLIENTSTATE_QUITING)
			break;

		/* be nice */
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
		
		/* update frametime */
		frametime = (time_get()-frame_start_time)/(float)time_freq();
	}
	
	modc_shutdown();
	client_disconnect();

	gfx_shutdown();
	snd_shutdown();
}


int editor_main(int argc, char **argv);

int main(int argc, char **argv)
{
	/* init the engine */
	dbg_msg("client", "starting...");
	engine_init("Teewars", argc, argv);
	
	if(config.cl_editor)
		editor_main(argc, argv);
	else
		client_run();
		
	engine_writeconfig();
	return 0;
}
