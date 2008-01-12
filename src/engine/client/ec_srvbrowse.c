/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_system.h>
#include <engine/e_network.h>
#include <engine/e_interface.h>
#include <engine/e_config.h>
#include <engine/e_memheap.h>

#include <mastersrv/mastersrv.h>

#include <string.h>
#include <stdlib.h>

extern NETCLIENT *net;


/* ------ server browse ---- */
/* TODO: move all this to a separate file */

typedef struct SERVERENTRY_t SERVERENTRY;
struct SERVERENTRY_t
{
	NETADDR4 addr;
	int64 request_time;
	int got_info;
	SERVER_INFO info;
	
	SERVERENTRY *next_ip; /* ip hashed list */
	
	SERVERENTRY *prev_req; /* request list */
	SERVERENTRY *next_req;
};

static HEAP *serverlist_heap = 0;
static SERVERENTRY **serverlist = 0;
static int *sorted_serverlist = 0;

static SERVERENTRY *serverlist_ip[256] = {0}; /* ip hash list */

static SERVERENTRY *first_req_server = 0; /* request list */
static SERVERENTRY *last_req_server = 0;
static int num_requests = 0;

static int num_sorted_servers = 0;
static int num_sorted_servers_capacity = 0;
static int num_servers = 0;
static int num_server_capacity = 0;

static int sorthash = 0;
static char filterstring[64] = {0};

static int serverlist_lan = 1;

int client_serverbrowse_num() { return num_servers; }

SERVER_INFO *client_serverbrowse_get(int index)
{
	if(index < 0 || index >= num_servers)
		return 0;
	return &serverlist[index]->info;
}

int client_serverbrowse_sorted_num() { return num_sorted_servers; }

SERVER_INFO *client_serverbrowse_sorted_get(int index)
{
	if(index < 0 || index >= num_sorted_servers)
		return 0;
	return &serverlist[sorted_serverlist[index]]->info;
}


int client_serverbrowse_num_requests()
{
	return num_requests;
}

static int client_serverbrowse_sort_compare_name(const void *ai, const void *bi)
{
	SERVERENTRY *a = serverlist[*(const int*)ai];
	SERVERENTRY *b = serverlist[*(const int*)bi];
	return strcmp(a->info.name, b->info.name);
}

static int client_serverbrowse_sort_compare_map(const void *ai, const void *bi)
{
	SERVERENTRY *a = serverlist[*(const int*)ai];
	SERVERENTRY *b = serverlist[*(const int*)bi];
	return strcmp(a->info.map, b->info.map);
}

static int client_serverbrowse_sort_compare_ping(const void *ai, const void *bi)
{
	SERVERENTRY *a = serverlist[*(const int*)ai];
	SERVERENTRY *b = serverlist[*(const int*)bi];
	if(a->info.latency > b->info.latency) return 1;
	if(a->info.latency < b->info.latency) return -1;
	return 0;
}

static int client_serverbrowse_sort_compare_gametype(const void *ai, const void *bi)
{
	SERVERENTRY *a = serverlist[*(const int*)ai];
	SERVERENTRY *b = serverlist[*(const int*)bi];
	if(a->info.game_type > b->info.game_type) return 1;
	if(a->info.game_type < b->info.game_type) return -1;
	return 0;
}

static int client_serverbrowse_sort_compare_progression(const void *ai, const void *bi)
{
	SERVERENTRY *a = serverlist[*(const int*)ai];
	SERVERENTRY *b = serverlist[*(const int*)bi];
	if(a->info.progression > b->info.progression) return 1;
	if(a->info.progression < b->info.progression) return -1;
	return 0;
}

static int client_serverbrowse_sort_compare_numplayers(const void *ai, const void *bi)
{
	SERVERENTRY *a = serverlist[*(const int*)ai];
	SERVERENTRY *b = serverlist[*(const int*)bi];
	if(a->info.num_players > b->info.num_players) return 1;
	if(a->info.num_players < b->info.num_players) return -1;
	return 0;
}

static void client_serverbrowse_filter()
{
	int i = 0;
	num_sorted_servers = 0;

	/* allocate the sorted list */	
	if(num_sorted_servers_capacity < num_servers)
	{
		if(sorted_serverlist)
			mem_free(sorted_serverlist);
		num_sorted_servers_capacity = num_servers;
		sorted_serverlist = mem_alloc(num_sorted_servers_capacity*sizeof(int), 1);
	}
	
	/* filter the servers */
	for(i = 0; i < num_servers; i++)
	{
		int filtered = 0;

		if(config.b_filter_empty && serverlist[i]->info.num_players == 0)
			filtered = 1;
		else if(config.b_filter_full && serverlist[i]->info.num_players == serverlist[i]->info.max_players)
			filtered = 1;
		else if(config.b_filter_pw && serverlist[i]->info.flags&1)
			filtered = 1;
		else if(config.b_filter_ping < serverlist[i]->info.latency)
			filtered = 1;
		else if(!(config.b_filter_gametype&(1<<serverlist[i]->info.game_type)))
			filtered = 1;
		else if(config.b_filter_string[0] != 0)
		{
			if(strstr(serverlist[i]->info.name, config.b_filter_string) == 0)
				filtered = 1;
		}
			
		if(filtered == 0)
			sorted_serverlist[num_sorted_servers++] = i;
	}
}

static int client_serverbrowse_sorthash()
{
	int i = config.b_sort&0xf;
	i |= config.b_filter_empty<<4;
	i |= config.b_filter_full<<5;
	i |= config.b_filter_pw<<6;
	i |= config.b_sort_order<<7;
	i |= config.b_filter_gametype<<8;
	i |= config.b_filter_ping<<16;
	return i;
}

static void client_serverbrowse_sort()
{
	int i;
	
	/* create filtered list */
	client_serverbrowse_filter();
	
	/* sort */
	if(config.b_sort == BROWSESORT_NAME)
		qsort(sorted_serverlist, num_sorted_servers, sizeof(int), client_serverbrowse_sort_compare_name);
	else if(config.b_sort == BROWSESORT_PING)
		qsort(sorted_serverlist, num_sorted_servers, sizeof(int), client_serverbrowse_sort_compare_ping);
	else if(config.b_sort == BROWSESORT_MAP)
		qsort(sorted_serverlist, num_sorted_servers, sizeof(int), client_serverbrowse_sort_compare_map);
	else if(config.b_sort == BROWSESORT_NUMPLAYERS)
		qsort(sorted_serverlist, num_sorted_servers, sizeof(int), client_serverbrowse_sort_compare_numplayers);
	else if(config.b_sort == BROWSESORT_GAMETYPE)
		qsort(sorted_serverlist, num_sorted_servers, sizeof(int), client_serverbrowse_sort_compare_gametype);
	else if(config.b_sort == BROWSESORT_PROGRESSION)
		qsort(sorted_serverlist, num_sorted_servers, sizeof(int), client_serverbrowse_sort_compare_progression);
	
	/* invert the list if requested */
	if(config.b_sort_order)
	{
		for(i = 0; i < num_sorted_servers/2; i++)
		{
			int temp = sorted_serverlist[i];
			sorted_serverlist[i] = sorted_serverlist[num_sorted_servers-i-1];
			sorted_serverlist[num_sorted_servers-i-1] = temp;
		}
	}
	
	/* set indexes */
	for(i = 0; i < num_sorted_servers; i++)
		serverlist[sorted_serverlist[i]]->info.sorted_index = i;
	
	strncpy(filterstring, config.b_filter_string, sizeof(filterstring)-1); 
	sorthash = client_serverbrowse_sorthash();
}

static void client_serverbrowse_remove_request(SERVERENTRY *entry)
{
	if(entry->prev_req || entry->next_req || first_req_server == entry)
	{
		if(entry->prev_req)
			entry->prev_req->next_req = entry->next_req;
		else
			first_req_server = entry->next_req;
			
		if(entry->next_req)
			entry->next_req->prev_req = entry->prev_req;
		else
			last_req_server = entry->prev_req;
			
		entry->prev_req = 0;
		entry->next_req = 0;
		num_requests--;
	}
}

void client_serverbrowse_set(NETADDR4 *addr, int request, SERVER_INFO *info)
{
	int hash = addr->ip[0];
	SERVERENTRY *entry = serverlist_ip[hash];
	while(entry)
	{
		if(net_addr4_cmp(&entry->addr, addr) == 0)
		{
			/* update the server that we already have */
			entry->info = *info;
			if(!request)
			{
				entry->info.latency = (time_get()-entry->request_time)*1000/time_freq();
				client_serverbrowse_remove_request(entry);
			}
			
			entry->got_info = 1;
			client_serverbrowse_sort();
			return;
		}
		entry = entry->next_ip;
	}

	/* create new entry */	
	entry = (SERVERENTRY *)memheap_allocate(serverlist_heap, sizeof(SERVERENTRY));
	mem_zero(entry, sizeof(SERVERENTRY));
	
	/* set the info */
	entry->addr = *addr;
	entry->info = *info;

	/* add to the hash list */	
	entry->next_ip = serverlist_ip[hash];
	serverlist_ip[hash] = entry;
	
	if(num_servers == num_server_capacity)
	{
		SERVERENTRY **newlist;
		num_server_capacity += 100;
		newlist = mem_alloc(num_server_capacity*sizeof(SERVERENTRY*), 1);
		memcpy(newlist, serverlist, num_servers*sizeof(SERVERENTRY*));
		mem_free(serverlist);
		serverlist = newlist;
	}
	
	/* add to list */
	serverlist[num_servers] = entry;
	entry->info.server_index = num_servers;
	num_servers++;
	
	/* */
	if(request)
	{
		/* add it to the list of servers that we should request info from */
		entry->prev_req = last_req_server;
		if(last_req_server)
			last_req_server->next_req = entry;
		else
			first_req_server = entry;
		last_req_server = entry;
		
		num_requests++;
	}
	
	client_serverbrowse_sort();
}

void client_serverbrowse_refresh(int lan)
{
	/* clear out everything */
	if(serverlist_heap)
		memheap_destroy(serverlist_heap);
	serverlist_heap = memheap_create();
	num_servers = 0;
	num_sorted_servers = 0;
	mem_zero(serverlist_ip, sizeof(serverlist_ip));
	first_req_server = 0;
	last_req_server = 0;
	num_requests = 0;
	
	
	/* */
	serverlist_lan = lan;
	
	if(serverlist_lan)
	{
		NETPACKET packet;
		packet.client_id = -1;
		mem_zero(&packet, sizeof(packet));
		packet.address.ip[0] = 255;
		packet.address.ip[1] = 255;
		packet.address.ip[2] = 255;
		packet.address.ip[3] = 255;
		packet.address.port = 8303;
		packet.flags = PACKETFLAG_CONNLESS;
		packet.data_size = sizeof(SERVERBROWSE_GETINFO);
		packet.data = SERVERBROWSE_GETINFO;
		netclient_send(net, &packet);	

		if(config.debug)
			dbg_msg("client", "broadcasting for servers");
	}
	else
	{
		NETADDR4 master_server;
		NETPACKET p;

		net_host_lookup(config.masterserver, MASTERSERVER_PORT, &master_server);

		mem_zero(&p, sizeof(p));
		p.client_id = -1;
		p.address = master_server;
		p.flags = PACKETFLAG_CONNLESS;
		p.data_size = sizeof(SERVERBROWSE_GETLIST);
		p.data = SERVERBROWSE_GETLIST;
		netclient_send(net, &p);	

		if(config.debug)
			dbg_msg("client", "requesting server list");
	}
}

static void client_serverbrowse_request(SERVERENTRY *entry)
{
	NETPACKET p;

	if(config.debug)
	{
		dbg_msg("client", "requesting server info from %d.%d.%d.%d:%d",
			entry->addr.ip[0], entry->addr.ip[1], entry->addr.ip[2],
			entry->addr.ip[3], entry->addr.port);
	}
	
	p.client_id = -1;
	p.address = entry->addr;
	p.flags = PACKETFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_GETINFO);
	p.data = SERVERBROWSE_GETINFO;
	netclient_send(net, &p);
	entry->request_time = time_get();
}

void client_serverbrowse_update()
{
	int64 timeout = time_freq();
	int64 now = time_get();
	int count;
	SERVERENTRY *entry, *next;
	
	/* do timeouts */
	entry = first_req_server;
	while(1)
	{
		if(!entry) /* no more entries */
			break;
			
		next = entry->next_req;
		
		if(entry->request_time && entry->request_time+timeout < now)
		{
			/* timeout */
			client_serverbrowse_remove_request(entry);
			num_requests--;
		}
			
		entry = next;
	}

	/* do timeouts */
	entry = first_req_server;
	count = 0;
	while(1)
	{
		if(!entry) /* no more entries */
			break;
		
		/* no more then 10 concurrent requests */
		if(count == config.b_max_requests)
			break;
			
		if(entry->request_time == 0)
			client_serverbrowse_request(entry);
		
		count++;
		entry = entry->next_req;
	}
	
	/* check if we need to resort */
	/* TODO: remove the strcmp */
	if(sorthash != client_serverbrowse_sorthash() || strcmp(filterstring, config.b_filter_string) != 0)
		client_serverbrowse_sort();
}
