#include <base/system.h>
#include "e_network.h"
#include "e_network_internal.h"

typedef struct
{
	NETCONNECTION conn;
} NETSLOT;

typedef struct NETBAN
{
	NETBANINFO info;
	
	struct NETBAN *hashnext;
	struct NETBAN *hashprev;
	
	struct NETBAN *next;
	struct NETBAN *prev;
} NETBAN;

struct NETSERVER
{
	NETSOCKET socket;
	NETSLOT slots[NET_MAX_CLIENTS];
	int max_clients;

	NETBAN *bans[256];
	NETBAN banpool[NET_SERVER_MAXBANS];
	NETBAN *banpool_firstfree;
	NETBAN *banpool_firstused;

	NETFUNC_NEWCLIENT new_client;
	NETFUNC_NEWCLIENT del_client;
	void *user_ptr;
	
	NETRECVINFO recv;
};

NETSERVER *netserver_open(NETADDR bindaddr, int max_clients, int flags)
{
	int i;
	NETSERVER *server;
	NETSOCKET socket = net_udp_create(bindaddr);
	if(socket == NETSOCKET_INVALID)
		return 0;
	
	server = (NETSERVER *)mem_alloc(sizeof(NETSERVER), 1);
	mem_zero(server, sizeof(NETSERVER));
	server->socket = socket;
	server->max_clients = max_clients;
	if(server->max_clients > NET_MAX_CLIENTS)
		server->max_clients = NET_MAX_CLIENTS;
	if(server->max_clients < 1)
		server->max_clients = 1;
	
	for(i = 0; i < NET_MAX_CLIENTS; i++)
		conn_init(&server->slots[i].conn, server->socket);
	
	/* setup all pointers for bans */
	for(i = 1; i < NET_SERVER_MAXBANS-1; i++)
	{
		server->banpool[i].next = &server->banpool[i+1];
		server->banpool[i].prev = &server->banpool[i-1];
	}
	
	server->banpool[0].next = &server->banpool[1];
	server->banpool[NET_SERVER_MAXBANS-1].prev = &server->banpool[NET_SERVER_MAXBANS-2];
	server->banpool_firstfree = &server->banpool[0];

	return server;
}

int netserver_set_callbacks(NETSERVER *s, NETFUNC_NEWCLIENT new_client, NETFUNC_DELCLIENT del_client, void *user)
{
	s->new_client = new_client;
	s->del_client = del_client;
	s->user_ptr = user;
	return 0;
}

int netserver_max_clients(NETSERVER *s)
{
	return s->max_clients;
}

int netserver_close(NETSERVER *s)
{
	/* TODO: implement me */
	return 0;
}

int netserver_drop(NETSERVER *s, int client_id, const char *reason)
{
	/* TODO: insert lots of checks here */
	NETADDR addr;
	netserver_client_addr(s, client_id, &addr);
	
	dbg_msg("net_server", "client dropped. cid=%d ip=%d.%d.%d.%d reason=\"%s\"",
		client_id,
		addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3],
		reason
		);
	conn_disconnect(&s->slots[client_id].conn, reason);

	if(s->del_client)
		s->del_client(client_id, s->user_ptr);
		
	return 0;
}

int netserver_ban_get(NETSERVER *s, int index, NETBANINFO *info)
{
	NETBAN *ban;
	for(ban = s->banpool_firstused; ban && index; ban = ban->next, index--)
		{}
		
	if(!ban)
		return 0;
	*info = ban->info;
	return 1;
}

int netserver_ban_num(NETSERVER *s)
{
	int count = 0;
	NETBAN *ban;
	for(ban = s->banpool_firstused; ban; ban = ban->next)
		count++;
	return count;
}

int netserver_ban_remove(NETSERVER *s, NETADDR addr)
{
	return 0;
}

int netserver_ban_add(NETSERVER *s, NETADDR addr, int type, int seconds)
{
	int iphash = (addr.ip[0]+addr.ip[1]+addr.ip[2]+addr.ip[3])&0xff;
	unsigned stamp = time_timestamp() + seconds;
	NETBAN *ban;
	NETBAN *insert_after;
	
	/* search to see if it already exists */
	for(ban = s->bans[iphash]; ban; ban = ban->hashnext)
	{
		if(net_addr_comp(&ban->info.addr, &addr) == 0)
		{
			if(ban->info.expires < stamp)
			{
				/* decide what to do here */
			}
			return -1;
		}
	}
	
	if(!s->banpool_firstfree)
		return -1;

	/* fetch and clear the new ban */	
	ban = s->banpool_firstfree;
	s->banpool_firstfree->prev = 0;
	ban->next = 0;
	ban->prev = 0;
	ban->info.expires = stamp;
	ban->info.type = type;
	
	/* add it to the ban hash */
	if(s->bans[iphash])
		s->bans[iphash]->hashprev = ban;
	ban->hashnext = s->bans[iphash];
	ban->hashprev = 0;
	s->bans[iphash] = ban;
	
	/* insert it into the used list */
	insert_after = s->banpool_firstused;
	while(1)
	{
		if(!insert_after->next)
			break;
		if(insert_after->next->info.expires < stamp)
			break;
		insert_after = insert_after->next;
	}
	
	if(!insert_after || insert_after->info.expires > stamp)
	{
		/* insert first */
		insert_after->prev = ban;
		s->banpool_firstused = ban;
		ban->next = insert_after;
		ban->prev = 0;
	}
	else
	{
		/* insert after */
		ban->next = insert_after->next;
		ban->prev = insert_after;
		if(ban->next)
			ban->next->prev = ban;
		insert_after->next = ban;
	}
	
	return 0;
}

int netserver_update(NETSERVER *s)
{
	int i;
	for(i = 0; i < s->max_clients; i++)
	{
		conn_update(&s->slots[i].conn);
		if(s->slots[i].conn.state == NET_CONNSTATE_ERROR)
			netserver_drop(s, i, conn_error(&s->slots[i].conn));
	}
	return 0;
}

/*
	TODO: chopp up this function into smaller working parts
*/
int netserver_recv(NETSERVER *s, NETCHUNK *chunk)
{
	unsigned now = time_timestamp();
	
	while(1)
	{
		NETADDR addr;
		int i, bytes, found;
			
		/* check for a chunk */
		if(recvinfo_fetch_chunk(&s->recv, chunk))
			return 1;
		
		/* TODO: empty the recvinfo */
		bytes = net_udp_recv(s->socket, &addr, s->recv.buffer, NET_MAX_PACKETSIZE);

		/* no more packets for now */
		if(bytes <= 0)
			break;
		
		if(unpack_packet(s->recv.buffer, bytes, &s->recv.data) == 0)
		{
			NETBAN *ban = 0;
			int iphash = (addr.ip[0]+addr.ip[1]+addr.ip[2]+addr.ip[3])&0xff;
			found = 0;
			
			/* search a ban */
			for(ban = s->bans[iphash]; ban; ban = ban->hashnext)
			{
				if(net_addr_comp(&ban->info.addr, &addr) == 0)
					break;
			}
			
			/* check if we just should drop the packet */
			if(ban && ban->info.type == NETBANTYPE_DROP && ban->info.expires > now)
				continue;
			
			if(s->recv.data.flags&NET_PACKETFLAG_CONNLESS)
			{
				chunk->flags = NETSENDFLAG_CONNLESS;
				chunk->client_id = -1;
				chunk->address = addr;
				chunk->data_size = s->recv.data.data_size;
				chunk->data = s->recv.data.chunk_data;
				return 1;
			}
			else
			{			
				/* TODO: check size here */
				if(s->recv.data.flags&NET_PACKETFLAG_CONTROL && s->recv.data.chunk_data[0] == NET_CTRLMSG_CONNECT)
				{
					found = 0;
					
					if(ban && ban->info.expires > now)
					{
						/* TODO: soft ban, reply with a message */
					}
					else
					{
						/* check if we already got this client */
						for(i = 0; i < s->max_clients; i++)
						{
							if(s->slots[i].conn.state != NET_CONNSTATE_OFFLINE &&
								net_addr_comp(&s->slots[i].conn.peeraddr, &addr) == 0)
							{
								found = 1; /* silent ignore.. we got this client already */
								break;
							}
						}
						
						/* client that wants to connect */
						if(!found)
						{
							for(i = 0; i < s->max_clients; i++)
							{
								if(s->slots[i].conn.state == NET_CONNSTATE_OFFLINE)
								{
									found = 1;
									conn_feed(&s->slots[i].conn, &s->recv.data, &addr);
									if(s->new_client)
										s->new_client(i, s->user_ptr);
									break;
								}
							}
							
							if(!found)
							{
								/* TODO: send server full message */
							}
						}
					}
				}
				else
				{
					/* normal packet, find matching slot */
					for(i = 0; i < s->max_clients; i++)
					{
						if(net_addr_comp(&s->slots[i].conn.peeraddr, &addr) == 0)
						{
							if(conn_feed(&s->slots[i].conn, &s->recv.data, &addr))
							{
								if(s->recv.data.data_size)
									recvinfo_start(&s->recv, &addr, &s->slots[i].conn, i);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int netserver_send(NETSERVER *s, NETCHUNK *chunk)
{
	if(chunk->data_size >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netserver", "packet payload too big. %d. dropping packet", chunk->data_size);
		return -1;
	}
	
	if(chunk->flags&NETSENDFLAG_CONNLESS)
	{
		/* send connectionless packet */
		send_packet_connless(s->socket, &chunk->address, chunk->data, chunk->data_size);
	}
	else
	{
		int f = 0;
		dbg_assert(chunk->client_id >= 0, "errornous client id");
		dbg_assert(chunk->client_id < s->max_clients, "errornous client id");
		
		if(chunk->flags&NETSENDFLAG_VITAL)
			f = NET_CHUNKFLAG_VITAL;
		
		conn_queue_chunk(&s->slots[chunk->client_id].conn, f, chunk->data_size, chunk->data);

		if(chunk->flags&NETSENDFLAG_FLUSH)
			conn_flush(&s->slots[chunk->client_id].conn);
	}
	return 0;
}

void netserver_stats(NETSERVER *s, NETSTATS *stats)
{
	int num_stats = sizeof(NETSTATS)/sizeof(int);
	int *istats = (int *)stats;
	int c, i;

	mem_zero(stats, sizeof(NETSTATS));
	
	for(c = 0; c < s->max_clients; c++)
	{
		if(s->slots[c].conn.state != NET_CONNSTATE_OFFLINE)
		{
			int *sstats = (int *)(&(s->slots[c].conn.stats));
			for(i = 0; i < num_stats; i++)
				istats[i] += sstats[i];
		}
	}
}

NETSOCKET netserver_socket(NETSERVER *s)
{
	return s->socket;
}


int netserver_client_addr(NETSERVER *s, int client_id, NETADDR *addr)
{
	*addr = s->slots[client_id].conn.peeraddr;
	return 1;
}
