/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <engine/e_system.h>
#include <engine/e_engine.h>
#include <engine/e_client_interface.h>

#include <engine/e_protocol.h>
#include <engine/e_snapshot.h>
#include <engine/e_compression.h>
#include <engine/e_network.h>
#include <engine/e_config.h>
#include <engine/e_packer.h>
#include <engine/e_memheap.h>
#include <engine/e_datafile.h>
#include <engine/e_console.h>
#include <engine/e_ringbuffer.h>

#include <mastersrv/mastersrv.h>

const int prediction_margin = 7; /* magic network prediction value */

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
static float frametime_low = 1.0f;
static float frametime_high = 0.0f;
static int frames = 0;
static NETADDR4 server_address;
static int window_must_refocus = 0;
static int snaploss = 0;
static int snapcrcerrors = 0;

static int ack_game_tick = -1;
static int current_recv_tick = 0;
static int rcon_authed = 0;

/* pinging */
static int64 ping_start_time = 0;

/* map download */
static char mapdownload_filename[256] = {0};
static IOHANDLE mapdownload_file = 0;
static int mapdownload_chunk = 0;
static int mapdownload_crc = 0;
static int mapdownload_amount = -1;
static int mapdownload_totalsize = -1;

/* current time */
static int current_tick = 0;
static float intratick = 0;
static float ticktime = 0;

/* predicted time */
static int current_predtick = 0;
static float predintratick = 0;

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
	adjust_speed = 0.2f; /*0.99f;*/
	if(t > c)
		adjust_speed = 350.0f;
	
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
static GRAPH input_late_graph;

/* -- snapshot handling --- */
enum
{
	NUM_SNAPSHOT_TYPES=2
};

/* the game snapshots are modifiable by the game */
SNAPSTORAGE snapshot_storage;
static SNAPSTORAGE_HOLDER *snapshots[NUM_SNAPSHOT_TYPES];

static int recived_snapshots;
static char snapshot_incomming_data[MAX_SNAPSHOT_SIZE];

/* --- */

void *snap_get_item(int snapid, int index, SNAP_ITEM *item)
{
	SNAPSHOT_ITEM *i;
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	i = snapshot_get_item(snapshots[snapid]->alt_snap, index);
	item->datasize = snapshot_get_item_datasize(snapshots[snapid]->alt_snap, index);
	item->type = snapitem_type(i);
	item->id = snapitem_id(i);
	return (void *)snapitem_data(i);
}

void snap_invalidate_item(int snapid, int index)
{
	SNAPSHOT_ITEM *i;
	dbg_assert(snapid >= 0 && snapid < NUM_SNAPSHOT_TYPES, "invalid snapid");
	i = snapshot_get_item(snapshots[snapid]->alt_snap, index);
	if(i)
	{
		if((char *)i < (char *)snapshots[snapid]->alt_snap || (char *)i > (char *)snapshots[snapid]->alt_snap + snapshots[snapid]->snap_size)
			dbg_msg("ASDFASDFASdf", "ASDFASDFASDF");
		if((char *)i >= (char *)snapshots[snapid]->snap && (char *)i < (char *)snapshots[snapid]->snap + snapshots[snapid]->snap_size)
			dbg_msg("ASDFASDFASdf", "ASDFASDFASDF");
		i->type_and_id = -1;
	}
}

void *snap_find_item(int snapid, int type, int id)
{
	/* TODO: linear search. should be fixed. */
	int i;
	for(i = 0; i < snapshots[snapid]->snap->num_items; i++)
	{
		SNAPSHOT_ITEM *itm = snapshot_get_item(snapshots[snapid]->alt_snap, i);
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
float client_predintratick() { return predintratick; }
float client_ticktime() { return ticktime; }
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
	
	if(!info)
		return -1;
		
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

int client_rcon_authed()
{
	return rcon_authed;
}

void client_rcon_auth(const char *name, const char *password)
{
	msg_pack_start_system(NETMSG_RCON_AUTH, MSGFLAG_VITAL);
	msg_pack_string(name, 32);
	msg_pack_string(password, 32);
	msg_pack_end();
	client_send_msg();
}

void client_rcon(const char *cmd)
{
	msg_pack_start_system(NETMSG_RCON_CMD, MSGFLAG_VITAL);
	msg_pack_string(cmd, 256);
	msg_pack_end();
	client_send_msg();
}

int client_connection_problems()
{
	return netclient_gotproblems(net);
}

void client_direct_input(int *input, int size)
{
	int i;
	msg_pack_start_system(NETMSG_INPUT, 0);
	msg_pack_int(ack_game_tick);
	msg_pack_int(current_predtick);
	msg_pack_int(size);
	
	for(i = 0; i < size/4; i++)
		msg_pack_int(input[i]);	
		
	msg_pack_end();
	client_send_msg();
	
	dbg_msg("client", "sent direct input");
}


static void client_send_input()
{
	int64 now = time_get();	
	int i, size;

	if(current_predtick <= 0)
		return;
	
	/* fetch input */
	size = modc_snap_input(inputs[current_input].data);
	
	msg_pack_start_system(NETMSG_INPUT, 0);
	msg_pack_int(ack_game_tick);
	msg_pack_int(current_predtick);
	msg_pack_int(size);

	inputs[current_input].tick = current_predtick;
	inputs[current_input].game_time = st_get(&predicted_time, now);
	inputs[current_input].time = now;

	/* pack it */	
	for(i = 0; i < size/4; i++)
		msg_pack_int(inputs[current_input].data[i]);
	
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
	
	client_disconnect();

	dbg_msg("client", "connecting to '%s'", server_address_str);

	str_copy(buf, server_address_str, sizeof(buf));

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
	
	rcon_authed = 0;
	netclient_connect(net, &server_address);
	client_set_state(CLIENTSTATE_CONNECTING);
	
	graph_init(&intra_graph, 0.0f, 1.0f);
	graph_init(&input_late_graph, 0.0f, 1.0f);
	graph_init(&predict_graph, 0.0f, 200.0f);
}

void client_disconnect_with_reason(const char *reason)
{
	rcon_authed = 0;
	netclient_disconnect(net, reason);
	client_set_state(CLIENTSTATE_OFFLINE);
	map_unload();
	
	/* disable all downloads */
	mapdownload_chunk = 0;
	if(mapdownload_file)
		io_close(mapdownload_file);
	mapdownload_file = 0;
	mapdownload_crc = 0;
	mapdownload_totalsize = -1;
	mapdownload_amount = 0;
	
}

void client_disconnect()
{
	client_disconnect_with_reason(0);
}

static int client_load_data()
{
	debug_font = gfx_load_texture("data/debug_font.png", IMG_AUTO, TEXLOAD_NORESAMPLE);
	return 1;
}

extern int snapshot_data_rate[0xffff];
extern int snapshot_data_updates[0xffff];

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
	str_format(buffer, sizeof(buffer), "ticks: %8d %8d send: %6d recv: %6d snaploss: %d  mem %dk   gfxmem: %dk  fps: %3d",
		current_tick, current_predtick,
		(current.send_bytes-prev.send_bytes)*10,
		(current.recv_bytes-prev.recv_bytes)*10,
		snaploss,
		mem_allocated()/1024,
		gfx_memory_usage()/1024,
		(int)(1.0f/frametime_avg));
	gfx_quads_text(2, 2, 16, buffer);

	/* render rates */
	{
		int i;
		for(i = 0; i < 256; i++)
		{
			if(snapshot_data_rate[i])
			{
				str_format(buffer, sizeof(buffer), "%4d : %8d %8d %8d", i, snapshot_data_rate[i]/8, snapshot_data_updates[i],
					(snapshot_data_rate[i]/snapshot_data_updates[i])/8);
				gfx_quads_text(2, 100+i*8, 16, buffer);
			}
		}
	}
	
	/* render graphs */
	if(config.dbg_graphs)
	{
		gfx_mapscreen(0,0,400.0f,300.0f);
		graph_render(&predict_graph, 300, 10, 90, 50);
		graph_render(&predicted_time.graph, 300, 10+50+10, 90, 50);
		
		graph_render(&intra_graph, 300, 10+50+10+50+10, 90, 50);
		graph_render(&input_late_graph, 300, 10+50+10+50+10+50+10, 90, 50);
	}
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
	if(config.gfx_clear)	
		gfx_clear(1,1,0);

	modc_render();
	client_debug_render();
}

static const char *client_load_map(const char *filename, int wanted_crc)
{
	static char errormsg[128];
	DATAFILE *df;
	int crc;
	
	client_set_state(CLIENTSTATE_LOADING);
	
	df = datafile_load(filename);
	if(!df)
	{
		str_format(errormsg, sizeof(errormsg), "map '%s' not found", filename);
		return errormsg;
	}
	
	/* get the crc of the map */
	crc = datafile_crc(filename);
	if(crc != wanted_crc)
	{
		datafile_unload(df);
		str_format(errormsg, sizeof(errormsg), "map differs from the server. %08x != %08x", crc, wanted_crc);
		return errormsg;
	}
	
	dbg_msg("client", "loaded map '%s'", filename);
	recived_snapshots = 0;
	map_set(df);
	return NULL;
}

static const char *client_load_map_search(const char *mapname, int wanted_crc)
{
	const char *error = 0;
	char buf[512];
	char buf2[512];
	dbg_msg("client", "loading map, map=%s wanted crc=%08x", mapname, wanted_crc);
	client_set_state(CLIENTSTATE_LOADING);
	
	/* try the normal maps folder */
	str_format(buf, sizeof(buf), "data/maps/%s.map", mapname);
	error = client_load_map(buf, wanted_crc);
	if(!error)
		return error;

	/* try the downloaded maps */
	str_format(buf2, sizeof(buf2), "maps/%s_%8x.map", mapname, wanted_crc);
	engine_savepath(buf2, buf, sizeof(buf));
	error = client_load_map(buf, wanted_crc);
	return error;
}


static int player_score_comp(const void *a, const void *b)
{
	SERVER_INFO_PLAYER *p0 = (SERVER_INFO_PLAYER *)a;
	SERVER_INFO_PLAYER *p1 = (SERVER_INFO_PLAYER *)b;
	if(p0->score == p1->score)
		return 0;
	if(p0->score < p1->score)
		return 1;
	return -1;
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
				str_format(info.address, sizeof(info.address), "%d.%d.%d.%d:%d",
					addr.ip[0], addr.ip[1], addr.ip[2],
					addr.ip[3], addr.port);
				str_format(info.name, sizeof(info.name), "\255%d.%d.%d.%d:%d", /* the \255 is to make sure that it's sorted last */
					addr.ip[0], addr.ip[1], addr.ip[2],
					addr.ip[3], addr.port);
				
				client_serverbrowse_set(&addr, 1, &info);
			}
		}

		{
			int got_info_packet = 0;
			
			if(client_serverbrowse_lan())
			{
				if(packet->data_size >= (int)sizeof(SERVERBROWSE_INFO_LAN) &&
					memcmp(packet->data, SERVERBROWSE_INFO_LAN, sizeof(SERVERBROWSE_INFO_LAN)) == 0)
				{
					got_info_packet = 1;
				}
			}
			else
			{
				if(packet->data_size >= (int)sizeof(SERVERBROWSE_INFO) &&
					memcmp(packet->data, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
				{
					got_info_packet = 1;
				}
			}

			if(got_info_packet)
			{
				/* we got ze info */
				UNPACKER up;
				SERVER_INFO info = {0};
				int i;

				unpacker_reset(&up, (unsigned char*)packet->data+sizeof(SERVERBROWSE_INFO), packet->data_size-sizeof(SERVERBROWSE_INFO));

				str_copy(info.version, unpacker_get_string(&up), sizeof(info.version));
				str_copy(info.name, unpacker_get_string(&up), sizeof(info.name));
				str_copy(info.map, unpacker_get_string(&up), sizeof(info.map));
				info.game_type = atol(unpacker_get_string(&up));
				info.flags = atol(unpacker_get_string(&up));
				info.progression = atol(unpacker_get_string(&up));
				info.num_players = atol(unpacker_get_string(&up));
				info.max_players = atol(unpacker_get_string(&up));
				str_format(info.address, sizeof(info.address), "%d.%d.%d.%d:%d",
					packet->address.ip[0], packet->address.ip[1], packet->address.ip[2],
					packet->address.ip[3], packet->address.port);
				
				for(i = 0; i < info.num_players; i++)
				{
					str_copy(info.players[i].name, unpacker_get_string(&up), sizeof(info.players[i].name));
					info.players[i].score = atol(unpacker_get_string(&up));
				}
				
				if(!up.error)
				{
					/* sort players */
					qsort(info.players, info.num_players, sizeof(*info.players), player_score_comp);
					client_serverbrowse_set(&packet->address, 0, &info);
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
			/* system message */
			if(msg == NETMSG_MAP_CHANGE)
			{
				const char *map = msg_unpack_string();
				int map_crc = msg_unpack_int();
				const char *error = 0;
				int i;

				if(msg_unpack_error())
					return;
				
				for(i = 0; map[i]; i++) /* protect the player from nasty map names */
				{
					if(map[i] == '/' || map[i] == '\\')
						error = "strange character in map name";
				}
				
				if(error)
					client_disconnect_with_reason(error);
				else
				{
					error = client_load_map_search(map, map_crc);

					if(!error)
					{
						dbg_msg("client/network", "loading done");
						client_send_ready();
						modc_connected();
					}
					else
					{
						char buf[512];
						str_format(buf, sizeof(buf), "maps/%s_%8x.map", map, map_crc);
						engine_savepath(buf, mapdownload_filename, sizeof(mapdownload_filename));

						dbg_msg("client/network", "starting to download map to '%s'", mapdownload_filename);
						
						mapdownload_chunk = 0;
						mapdownload_file = io_open(mapdownload_filename, IOFLAG_WRITE);
						mapdownload_crc = map_crc;
						mapdownload_totalsize = -1;
						mapdownload_amount = 0;
						
						msg_pack_start_system(NETMSG_REQUEST_MAP_DATA, 0);
						msg_pack_int(mapdownload_chunk);
						msg_pack_end();
						client_send_msg();
										
						if(config.debug)
							dbg_msg("client/network", "requested chunk %d", mapdownload_chunk);
					}
				}
			}
			else if(msg == NETMSG_MAP_DATA)
			{
				int last = msg_unpack_int();
				int total_size = msg_unpack_int();
				int size = msg_unpack_int();
				const unsigned char *data = msg_unpack_raw(size);
				
				/* check fior errors */
				if(msg_unpack_error() || size <= 0 || total_size <= 0 || !mapdownload_file)
					return;
				
				io_write(mapdownload_file, data, size);
				
				mapdownload_totalsize = total_size;
				mapdownload_amount += size;
				
				if(last)
				{
					const char *error;
					dbg_msg("client/network", "download complete, loading map");
					
					io_close(mapdownload_file);
					mapdownload_file = 0;
					mapdownload_amount = 0;
					mapdownload_totalsize = -1;
					
					/* load map */
					error = client_load_map(mapdownload_filename, mapdownload_crc);
					if(!error)
					{
						dbg_msg("client/network", "loading done");
						client_send_ready();
						modc_connected();
					}
					else
						client_disconnect_with_reason(error);
				}
				else
				{
					/* request new chunk */
					mapdownload_chunk++;
					msg_pack_start_system(NETMSG_REQUEST_MAP_DATA, 0);
					msg_pack_int(mapdownload_chunk);
					msg_pack_end();
					client_send_msg();

					if(config.debug)
						dbg_msg("client/network", "requested chunk %d", mapdownload_chunk);
				}
			}
			else if(msg == NETMSG_PING)
			{
				msg_pack_start_system(NETMSG_PING_REPLY, 0);
				msg_pack_end();
				client_send_msg();
			}
			else if(msg == NETMSG_RCON_AUTH_STATUS)
			{
				int result = msg_unpack_int();
				if(msg_unpack_error() == 0)
					rcon_authed = result;
			}
			else if(msg == NETMSG_RCON_LINE)
			{
				const char *line = msg_unpack_string();
				if(msg_unpack_error() == 0)
				{
					/*dbg_msg("remote", "%s", line);*/
					modc_rcon_line(line);
				}
			}
			else if(msg == NETMSG_PING_REPLY)
				dbg_msg("client/network", "latency %.2f", (time_get() - ping_start_time)*1000 / (float)time_freq());
			else if(msg == NETMSG_SNAP || msg == NETMSG_SNAPSINGLE || msg == NETMSG_SNAPEMPTY)
			{
				/*dbg_msg("client/network", "got snapshot"); */
				int num_parts = 1;
				int part = 0;
				int game_tick = msg_unpack_int();
				int delta_tick = game_tick-msg_unpack_int();
				int input_predtick = msg_unpack_int();
				int time_left = msg_unpack_int();
				int part_size = 0;
				int crc = 0;
				int complete_size = 0;
				const char *data = 0;
				
				if(msg == NETMSG_SNAP)
				{
					num_parts = msg_unpack_int();
					part = msg_unpack_int();
				}
				
				if(msg != NETMSG_SNAPEMPTY)
				{
					crc = msg_unpack_int();
					part_size = msg_unpack_int();
				}
				
				data = (const char *)msg_unpack_raw(part_size);
				
				if(msg_unpack_error())
					return;
				
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
					mem_copy((char*)snapshot_incomming_data + part*MAX_SNAPSHOT_PACKSIZE, data, part_size);
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
						
						complete_size = (num_parts-1) * MAX_SNAPSHOT_PACKSIZE + part_size;

						/* reset snapshoting */
						snapshot_part = 0;
						
						/* find snapshot that we should use as delta */
						emptysnap.data_size = 0;
						emptysnap.num_items = 0;
						
						/* find delta */
						if(delta_tick >= 0)
						{
							int deltashot_size = snapstorage_get(&snapshot_storage, delta_tick, 0, &deltashot, 0);
							
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

						if(complete_size)
						{	
							int intsize;
							int compsize = zerobit_decompress(snapshot_incomming_data, complete_size, tmpbuffer);
							
							if(compsize < 0) /* failure during decompression, bail */
								return;
								
							intsize = intpack_decompress(tmpbuffer, compsize, tmpbuffer2);

							if(intsize < 0) /* failure during decompression, bail */
								return;

							deltadata = tmpbuffer2;
							deltasize = intsize;
						}
						
						/* unpack delta */
						purgetick = delta_tick;
						snapsize = snapshot_unpack_delta(deltashot, (SNAPSHOT*)tmpbuffer3, deltadata, deltasize);
						if(snapsize < 0)
							return;
							
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
						
						/* add new */
						snapstorage_add(&snapshot_storage, game_tick, time_get(), snapsize, (SNAPSHOT*)tmpbuffer3, 1);
						
						/* apply snapshot, cycle pointers */
						recived_snapshots++;

						if(current_recv_tick > 0)
							snaploss += game_tick-current_recv_tick-2;
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

int client_mapdownload_amount() { return mapdownload_amount; }
int client_mapdownload_totalsize() { return mapdownload_totalsize; }

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

static void client_update()
{
	/* switch snapshot */
	if(client_state() != CLIENTSTATE_OFFLINE && recived_snapshots >= 3)
	{
		int repredict = 0;
		int64 freq = time_freq();
		int64 now = st_get(&game_time, time_get());
		int64 pred_now = st_get(&predicted_time, time_get());

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
			/*tg_add(&predicted_time_graph, pred_now, 0); */
			int prev_pred_tick = (int)(pred_now*50/time_freq());
			int new_pred_tick = prev_pred_tick+1;
			static float last_predintra = 0;

			intratick = (now - curtick_start) / (float)(curtick_start-prevtick_start);
			ticktime = (now - curtick_start) / (freq/(float)SERVER_TICK_SPEED);

			graph_add(&intra_graph, intratick*0.25f);

			curtick_start = new_pred_tick*time_freq()/50;
			prevtick_start = prev_pred_tick*time_freq()/50;
			predintratick = (pred_now - prevtick_start) / (float)(curtick_start-prevtick_start);
			
			if(new_pred_tick < snapshots[SNAP_PREV]->tick-SERVER_TICK_SPEED/10 || new_pred_tick > snapshots[SNAP_PREV]->tick+SERVER_TICK_SPEED)
			{
				dbg_msg("client", "prediction time reset!");
				st_init(&predicted_time, snapshots[SNAP_CURRENT]->tick*time_freq()/50);
			}
			
			if(new_pred_tick > current_predtick)
			{
				last_predintra = predintratick;
				current_predtick = new_pred_tick;
				repredict = 1;
				
				/* send input */
				client_send_input();
			}
			
			last_predintra = predintratick;
		}

		/* only do sane predictions */
		if(repredict)
		{
			if(current_predtick > current_tick && current_predtick < current_tick+50)
				modc_predict();
		}
	}

	/* STRESS TEST: join the server again */
	if(config.dbg_stress)
	{
		static int64 action_taken = 0;
		int64 now = time_get();
		if(client_state() == CLIENTSTATE_OFFLINE)
		{
			if(now > action_taken+time_freq()*2)
			{
				dbg_msg("stress", "reconnecting!");
				client_connect(config.dbg_stress_server);
				action_taken = now;
			}
		}
		else
		{
			if(now > action_taken+time_freq()*(10+config.dbg_stress))
			{
				dbg_msg("stress", "disconnecting!");
				client_disconnect();
				action_taken = now;
			}
		}
	}
	
	/* pump the network */
	client_pump_network();
	
	/* update the maser server registry */
	mastersrv_update();
	
	/* update the server browser */
	client_serverbrowse_update();
}

extern int editor_update_and_render();
extern void editor_init();

static void client_run()
{
	NETADDR4 bindaddr;
	int64 reporttime = time_get();
	int64 reportinterval = time_freq()*1;

	static PERFORMACE_INFO rootscope = {"root", 0};
	perf_start(&rootscope);

	local_start_time = time_get();
	snapshot_part = 0;
	
	/* init graphics and sound */
	if(!gfx_init())
		return;

	/* start refreshing addresses while we load */
	mastersrv_refresh_addresses();
	
	/* init the editor */
	editor_init();

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
	/*
	if(config.cl_connect[0] != 0)
		client_connect(config.cl_connect);
	config.cl_connect[0] = 0;
	*/
	
	/* never start with the editor */
	config.cl_editor = 0;
		
	inp_mouse_mode_relative();
	
	while (1)
	{	
		static PERFORMACE_INFO rootscope = {"root", 0};
		int64 frame_start_time = time_get();
		frames++;
		
		perf_start(&rootscope);
		
		/* update input */
		{
			static PERFORMACE_INFO scope = {"inp_update", 0};
			perf_start(&scope);
			inp_update();
			perf_end();
		}

		/* update sound */		
		{
			static PERFORMACE_INFO scope = {"snd_update", 0};
			perf_start(&scope);
			snd_update();
			perf_end();
		}
		
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

		/* panic quit button */
		if(inp_key_pressed(KEY_LCTRL) && inp_key_pressed(KEY_LSHIFT) && inp_key_pressed('Q'))
			break;

		if(inp_key_pressed(KEY_LCTRL) && inp_key_pressed(KEY_LSHIFT) && inp_key_down('E'))
			config.cl_editor = config.cl_editor^1;
		
		if(!gfx_window_open())
			break;
			
		/* render */
		if(config.cl_editor)
		{
			client_update();
			editor_update_and_render();
			gfx_swap();
		}
		else
		{
			{
				static PERFORMACE_INFO scope = {"client_update", 0};
				perf_start(&scope);
				client_update();
				perf_end();
			}
			
			if(config.dbg_stress)
			{
				if((frames%10) == 0)
				{
					client_render();
					gfx_swap();
				}
			}
			else
			{
				{
					static PERFORMACE_INFO scope = {"client_render", 0};
					perf_start(&scope);
					client_render();
					perf_end();
				}

				{
					static PERFORMACE_INFO scope = {"gfx_swap", 0};
					perf_start(&scope);
					gfx_swap();
					perf_end();
				}
			}
		}

		perf_end();

		
		/* check conditions */
		if(client_state() == CLIENTSTATE_QUITING)
			break;

		/* be nice */
		if(config.cl_cpu_throttle || !gfx_window_active())
			thread_sleep(1);
			
		if(config.dbg_hitch)
		{
			thread_sleep(config.dbg_hitch);
			config.dbg_hitch = 0;
		}
		
		if(reporttime < time_get())
		{
			if(config.debug)
			{
				dbg_msg("client/report", "fps=%.02f (%.02f %.02f) netstate=%d",
					frames/(float)(reportinterval/time_freq()),
					1.0f/frametime_high,
					1.0f/frametime_low,
					netclient_state(net));
			}
			frametime_low = 1;
			frametime_high = 0;
			frames = 0;
			reporttime += reportinterval;
			perf_next();
			
			if(config.dbg_pref)
				perf_dump(&rootscope);
		}
		
		/* update frametime */
		frametime = (time_get()-frame_start_time)/(float)time_freq();
		if(frametime < frametime_low)
			frametime_low = frametime;
		if(frametime > frametime_high)
			frametime_high = frametime;
	}
	
	modc_shutdown();
	client_disconnect();

	gfx_shutdown();
	snd_shutdown();
}

static void con_connect(void *result, void *user_data)
{
	client_connect(console_arg_string(result, 0));
}

static void con_disconnect(void *result, void *user_data)
{
	client_disconnect();
}

static void con_quit(void *result, void *user_data)
{
	client_quit();
}

static void con_ping(void *result, void *user_data)
{
	msg_pack_start_system(NETMSG_PING, 0);
	msg_pack_end();
	client_send_msg();
	ping_start_time = time_get();
}

static void con_screenshot(void *result, void *user_data)
{
	gfx_screenshot();
}

static void con_rcon(void *result, void *user_data)
{
	client_rcon(console_arg_string(result, 0));
}

static void client_register_commands()
{
	MACRO_REGISTER_COMMAND("quit", "", con_quit, 0x0);
	MACRO_REGISTER_COMMAND("connect", "s", con_connect, 0x0);
	MACRO_REGISTER_COMMAND("disconnect", "", con_disconnect, 0x0);
	MACRO_REGISTER_COMMAND("ping", "", con_ping, 0x0);
	MACRO_REGISTER_COMMAND("screenshot", "", con_screenshot, 0x0);
	MACRO_REGISTER_COMMAND("rcon", "r", con_rcon, 0x0);
}

void client_save_line(const char *line)
{
	engine_config_write_line(line);	
}

/*int editor_main(int argc, char **argv);*/

/*extern void test_parser();*/

int main(int argc, char **argv)
{
#if defined(CONF_PLATFORM_MACOSX)
	char buffer[512];
	unsigned pos = strrchr(argv[0], '/') - argv[0];

	if(pos >= 512)
		return -1;

	strncpy(buffer, argv[0], 511);
	buffer[pos] = 0;
	chdir(buffer);
#endif

	/* init the engine */
	dbg_msg("client", "starting...");
	engine_init("Teeworlds");

/*	test_parser();
	return 0;*/
	
	/* register all console commands */
	client_register_commands();
	modc_console_init();
	
	/* parse the command line arguments */
	engine_parse_arguments(argc, argv);
	
	/* run the client*/
	client_run();
	
	/* write down the config and quit */
	if(engine_config_write_start() == 0)
	{
		config_save();
		modc_save_config();
		engine_config_write_stop();
	}
	
	return 0;
}
