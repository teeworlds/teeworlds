/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>
#include <engine/e_network.h>
#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include <engine/e_memheap.h>
#include <engine/e_engine.h>

#include <mastersrv/mastersrv.h>

#include <string.h>
#include <stdlib.h>

extern NETCLIENT *net;


/* ------ server browse ---- */
/* TODO: move all this to a separate file */

typedef struct SERVERENTRY_t SERVERENTRY;
struct SERVERENTRY_t
{
	NETADDR addr;
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

enum
{
	MAX_FAVORITES=256
};

static NETADDR favorite_servers[MAX_FAVORITES];
static int num_favorite_servers = 0;

static SERVERENTRY *serverlist_ip[256] = {0}; /* ip hash list */

static SERVERENTRY *first_req_server = 0; /* request list */
static SERVERENTRY *last_req_server = 0;
static int num_requests = 0;

static int need_refresh = 0;

static int num_sorted_servers = 0;
static int num_sorted_servers_capacity = 0;
static int num_servers = 0;
static int num_server_capacity = 0;

static int sorthash = 0;
static char filterstring[64] = {0};
static char filtergametypestring[128] = {0};

/* the token is to keep server refresh separated from each other */
static int current_token = 1;

static int serverlist_type = 0;
static int64 broadcast_time = 0;

int client_serverbrowse_lan() { return serverlist_type == BROWSETYPE_LAN; }
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
	return strcmp(a->info.gametype, b->info.gametype);
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
	int i = 0, p = 0;
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
		else if(config.b_filter_compatversion && strncmp(serverlist[i]->info.version, modc_net_version(), 3) != 0)
			filtered = 1;
		else if(config.b_filter_string[0] != 0)
		{
			int matchfound = 0;
			
			serverlist[i]->info.quicksearch_hit = 0;

			/* match against server name */
			if(str_find_nocase(serverlist[i]->info.name, config.b_filter_string))
			{
				matchfound = 1;
				serverlist[i]->info.quicksearch_hit |= BROWSEQUICK_SERVERNAME;
			}

			/* match against players */				
			for(p = 0; p < serverlist[i]->info.num_players; p++)
			{
				if(str_find_nocase(serverlist[i]->info.players[p].name, config.b_filter_string))
				{
					matchfound = 1;
					serverlist[i]->info.quicksearch_hit |= BROWSEQUICK_PLAYERNAME;
					break;
				}
			}
			
			/* match against map */
			if(str_find_nocase(serverlist[i]->info.map, config.b_filter_string))
			{
				matchfound = 1;
				serverlist[i]->info.quicksearch_hit |= BROWSEQUICK_MAPNAME;
			}
			
			if(!matchfound)
				filtered = 1;
		}
		else if(config.b_filter_gametype[0] != 0)
		{
			/* match against game type */
			if(!str_find_nocase(serverlist[i]->info.gametype, config.b_filter_gametype))
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
	i |= config.b_filter_compatversion<<8;
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
	
	str_copy(filtergametypestring, config.b_filter_gametype, sizeof(filtergametypestring)); 
	str_copy(filterstring, config.b_filter_string, sizeof(filterstring)); 
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

static SERVERENTRY *client_serverbrowse_find(NETADDR *addr)
{
	SERVERENTRY *entry = serverlist_ip[addr->ip[0]];
	
	for(; entry; entry = entry->next_ip)
	{
		if(net_addr_comp(&entry->addr, addr) == 0)
			return entry;
	}
	return (SERVERENTRY*)0;
}

void client_serverbrowse_queuerequest(SERVERENTRY *entry)
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

void client_serverbrowse_setinfo(SERVERENTRY *entry, SERVER_INFO *info)
{
	int fav = entry->info.favorite;
	entry->info = *info;
	entry->info.favorite = fav;
	entry->info.netaddr = entry->addr;

	/*if(!request)
	{
		entry->info.latency = (time_get()-entry->request_time)*1000/time_freq();
		client_serverbrowse_remove_request(entry);
	}*/
	
	entry->got_info = 1;
	client_serverbrowse_sort();	
}

SERVERENTRY *client_serverbrowse_add(NETADDR *addr)
{
	int hash = addr->ip[0];
	SERVERENTRY *entry = 0;
	int i;

	/* create new entry */
	entry = (SERVERENTRY *)memheap_allocate(serverlist_heap, sizeof(SERVERENTRY));
	mem_zero(entry, sizeof(SERVERENTRY));
	
	/* set the info */
	entry->addr = *addr;
	entry->info.netaddr = *addr;
	
	entry->info.latency = 999;
	str_format(entry->info.address, sizeof(entry->info.address), "%d.%d.%d.%d:%d",
		addr->ip[0], addr->ip[1], addr->ip[2],
		addr->ip[3], addr->port);
	str_format(entry->info.name, sizeof(entry->info.name), "\255%d.%d.%d.%d:%d", /* the \255 is to make sure that it's sorted last */
		addr->ip[0], addr->ip[1], addr->ip[2],
		addr->ip[3], addr->port);	
	
	/*if(serverlist_type == BROWSETYPE_LAN)
		entry->info.latency = (time_get()-broadcast_time)*1000/time_freq();*/

	/* check if it's a favorite */
	for(i = 0; i < num_favorite_servers; i++)
	{
		if(net_addr_comp(addr, &favorite_servers[i]) == 0)
			entry->info.favorite = 1;
	}
	
	/* add to the hash list */	
	entry->next_ip = serverlist_ip[hash];
	serverlist_ip[hash] = entry;
	
	if(num_servers == num_server_capacity)
	{
		SERVERENTRY **newlist;
		num_server_capacity += 100;
		newlist = mem_alloc(num_server_capacity*sizeof(SERVERENTRY*), 1);
		mem_copy(newlist, serverlist, num_servers*sizeof(SERVERENTRY*));
		mem_free(serverlist);
		serverlist = newlist;
	}
	
	/* add to list */
	serverlist[num_servers] = entry;
	entry->info.server_index = num_servers;
	num_servers++;
	
	return entry;
}

void client_serverbrowse_set(NETADDR *addr, int type, int token, SERVER_INFO *info)
{
	SERVERENTRY *entry = 0;
	if(type == BROWSESET_MASTER_ADD)
	{
		if(serverlist_type != BROWSETYPE_INTERNET)
			return;
			
		if(!client_serverbrowse_find(addr))
		{
			entry = client_serverbrowse_add(addr);
			client_serverbrowse_queuerequest(entry);
		}
	}
	else if(type == BROWSESET_FAV_ADD)
	{
		if(serverlist_type != BROWSETYPE_FAVORITES)
			return;
			
		if(!client_serverbrowse_find(addr))
		{
			entry = client_serverbrowse_add(addr);
			client_serverbrowse_queuerequest(entry);
		}
	}
	else if(type == BROWSESET_TOKEN)
	{
		if(token != current_token)
			return;
			
		entry = client_serverbrowse_find(addr);
		if(!entry)
			entry = client_serverbrowse_add(addr);
		if(entry)
		{
			client_serverbrowse_setinfo(entry, info);
			if(serverlist_type == BROWSETYPE_LAN)
				entry->info.latency = (time_get()-broadcast_time)*1000/time_freq();
			else
				entry->info.latency = (time_get()-entry->request_time)*1000/time_freq();
			client_serverbrowse_remove_request(entry);				
		}
	}
	else if(type == BROWSESET_OLD_INTERNET)
	{
		entry = client_serverbrowse_find(addr);
		if(entry)
		{
			client_serverbrowse_setinfo(entry, info);
			
			if(serverlist_type == BROWSETYPE_LAN)
				entry->info.latency = (time_get()-broadcast_time)*1000/time_freq();
			else
				entry->info.latency = (time_get()-entry->request_time)*1000/time_freq();
			client_serverbrowse_remove_request(entry);				
		}
	}
	else if(type == BROWSESET_OLD_LAN)
	{
		entry = client_serverbrowse_find(addr);
		if(entry)
		if(!entry)
			entry = client_serverbrowse_add(addr);
		if(entry)
			client_serverbrowse_setinfo(entry, info);
	}
	
	client_serverbrowse_sort();
}

void client_serverbrowse_refresh(int type)
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
	
	/* next token */
	current_token = (current_token+1)&0xff;
	
	/* */
	serverlist_type = type;

	if(type == BROWSETYPE_LAN)
	{
		unsigned char buffer[sizeof(SERVERBROWSE_GETINFO)+1];
		NETCHUNK packet;
		
		mem_copy(buffer, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
		buffer[sizeof(SERVERBROWSE_GETINFO)] = current_token;
			
		packet.client_id = -1;
		mem_zero(&packet, sizeof(packet));
		packet.address.ip[0] = 255;
		packet.address.ip[1] = 255;
		packet.address.ip[2] = 255;
		packet.address.ip[3] = 255;
		packet.address.port = 8303;
		packet.flags = NETSENDFLAG_CONNLESS;
		packet.data_size = sizeof(buffer);
		packet.data = buffer;
		broadcast_time = time_get();
		netclient_send(net, &packet);

		if(config.debug)
			dbg_msg("client", "broadcasting for servers");
	}
	else if(type == BROWSETYPE_INTERNET)
	{
		need_refresh = 1;
		mastersrv_refresh_addresses();
	}
	else if(type == BROWSETYPE_FAVORITES)
	{
		int i;
		for(i = 0; i < num_favorite_servers; i++)
			client_serverbrowse_set(&favorite_servers[i], BROWSESET_FAV_ADD, -1, 0);
	}
}

static void client_serverbrowse_request(NETADDR *addr, SERVERENTRY *entry)
{
	/*unsigned char buffer[sizeof(SERVERBROWSE_GETINFO)+1];*/
	NETCHUNK p;

	if(config.debug)
	{
		dbg_msg("client", "requesting server info from %d.%d.%d.%d:%d",
			addr->ip[0], addr->ip[1], addr->ip[2],
			addr->ip[3], addr->port);
	}
	
	/*mem_copy(buffer, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
	buffer[sizeof(SERVERBROWSE_GETINFO)] = current_token;*/
	
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	/*p.data_size = sizeof(buffer);
	p.data = buffer;
	netclient_send(net, &p);*/

	/* send old requtest style aswell */	
	p.data_size = sizeof(SERVERBROWSE_OLD_GETINFO);
	p.data = SERVERBROWSE_OLD_GETINFO;
	netclient_send(net, &p);
	
	if(entry)
		entry->request_time = time_get();
}

void client_serverbrowse_update()
{
	int64 timeout = time_freq();
	int64 now = time_get();
	int count;
	SERVERENTRY *entry, *next;
	
	/* do server list requests */
	if(need_refresh && !mastersrv_refreshing())
	{
		NETADDR addr;
		NETCHUNK p;
		int i;
		
		need_refresh = 0;
		
		mem_zero(&p, sizeof(p));
		p.client_id = -1;
		p.flags = NETSENDFLAG_CONNLESS;
		p.data_size = sizeof(SERVERBROWSE_GETLIST);
		p.data = SERVERBROWSE_GETLIST;
		
		for(i = 0; i < MAX_MASTERSERVERS; i++)
		{
			addr = mastersrv_get(i);
			if(!addr.ip[0] && !addr.ip[1] && !addr.ip[2] && !addr.ip[3])
				continue;
			
			p.address = addr;
			netclient_send(net, &p);
		}

		if(config.debug)
			dbg_msg("client", "requesting server list");
	}
	
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
			client_serverbrowse_request(&entry->addr, entry);
		
		count++;
		entry = entry->next_req;
	}
	
	/* check if we need to resort */
	/* TODO: remove the strcmp */
	if(sorthash != client_serverbrowse_sorthash() || strcmp(filterstring, config.b_filter_string) != 0 || strcmp(filtergametypestring, config.b_filter_gametype) != 0)
		client_serverbrowse_sort();
}


void client_serverbrowse_addfavorite(NETADDR addr)
{
	int i;
	SERVERENTRY *entry;
	
	if(num_favorite_servers == MAX_FAVORITES)
		return;

	/* make sure that we don't already have the server in our list */
	for(i = 0; i < num_favorite_servers; i++)
	{
		if(net_addr_comp(&addr, &favorite_servers[i]) == 0)
			return;
	}
	
	/* add the server to the list */
	favorite_servers[num_favorite_servers++] = addr;
	entry = client_serverbrowse_find(&addr);
	if(entry)
		entry->info.favorite = 1;
	dbg_msg("", "added fav, %p", entry);
}

void client_serverbrowse_removefavorite(NETADDR addr)
{
	int i;
	SERVERENTRY *entry;
	
	for(i = 0; i < num_favorite_servers; i++)
	{
		if(net_addr_comp(&addr, &favorite_servers[i]) == 0)
		{
			mem_move(&favorite_servers[i], &favorite_servers[i+1], num_favorite_servers-(i+1));
			num_favorite_servers--;

			entry = client_serverbrowse_find(&addr);
			if(entry)
				entry->info.favorite = 0;

			return;
		}
	}
}

void client_serverbrowse_save()
{
	int i;
	char addrstr[128];
	char buffer[256];
	for(i = 0; i < num_favorite_servers; i++)
	{
		net_addr_str(&favorite_servers[i], addrstr, sizeof(addrstr));
		str_format(buffer, sizeof(buffer), "add_favorite %s", addrstr);
		engine_config_write_line(buffer);
	}
}


int client_serverbrowse_refreshingmasters()
{
	return mastersrv_refreshing();
}
