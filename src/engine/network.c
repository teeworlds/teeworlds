/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <stdio.h>

#include "system.h"
#include "config.h"
#include "network.h"

/*
	header (6 bytes)
		unsigned char flags;		1
		unsigned char seq_ack[3];	4
		unsigned char token[2];		6
*/

enum
{
	NETWORK_VERSION = 1,
	
	NETWORK_HEADER_SIZE = 6,
	NETWORK_MAX_PAYLOAD = 1024,
	NETWORK_MAX_PACKET_SIZE = NETWORK_HEADER_SIZE+NETWORK_MAX_PAYLOAD,
	NETWORK_MAX_CLIENTS = 16,
	
	NETWORK_CONNSTATE_OFFLINE=0,
	NETWORK_CONNSTATE_CONNECT=1,
	NETWORK_CONNSTATE_CONNECTACCEPTED=2,
	NETWORK_CONNSTATE_ONLINE=3,
	NETWORK_CONNSTATE_ERROR=4,
	
	NETWORK_PACKETFLAG_CONNECT=0x01,
	NETWORK_PACKETFLAG_ACCEPT=0x02,
	NETWORK_PACKETFLAG_CLOSE=0x04,
	NETWORK_PACKETFLAG_VITAL=0x08,
	NETWORK_PACKETFLAG_RESEND=0x10,
	NETWORK_PACKETFLAG_CONNLESS=0x20,
	
	NETWORK_MAX_SEQACK=0x1000
};

static int current_token = 1;

typedef struct
{
	unsigned char ID[2];
	unsigned char version;
	unsigned char flags;
	unsigned short seq;
	unsigned short ack;
	unsigned crc;
	int token;
	unsigned data_size;
	int64 first_send_time;
	unsigned char *data;
} NETPACKETDATA;


static void send_packet(NETSOCKET socket, NETADDR4 *addr, NETPACKETDATA *packet)
{
	unsigned char buffer[NETWORK_MAX_PACKET_SIZE];
	int send_size = NETWORK_HEADER_SIZE+packet->data_size;

	buffer[0] = packet->flags;
	buffer[1] = ((packet->seq>>4)&0xf0) | ((packet->ack>>8)&0x0f);
	buffer[2] = packet->seq;
	buffer[3] = packet->ack;
	buffer[4] = packet->token>>8;
	buffer[5] = packet->token&0xff;
	mem_copy(buffer+NETWORK_HEADER_SIZE, packet->data, packet->data_size);
	net_udp4_send(socket, addr, buffer, send_size);
}

typedef struct RINGBUFFER_ITEM_t
{
	struct RINGBUFFER_ITEM_t *next;
	struct RINGBUFFER_ITEM_t *prev;
	int size;
} RINGBUFFER_ITEM;

typedef struct
{
	RINGBUFFER_ITEM *first;
	RINGBUFFER_ITEM *last;
	unsigned buffer_size;
} RINGBUFFER;

static void rb_init(RINGBUFFER *rb)
{
	rb->first = 0;
	rb->last = 0;
	rb->buffer_size = 0;
}

static void *rb_item_data(RINGBUFFER_ITEM *item)
{
	return (void*)(item+1);
}

static void *rb_alloc(RINGBUFFER *rb, int size)
{
	RINGBUFFER_ITEM *item = (RINGBUFFER_ITEM*)mem_alloc(sizeof(RINGBUFFER_ITEM)+size, 1);
	item->size = size;
	
	item->prev = rb->last;
	item->next = 0;
	if(rb->last)
		rb->last->next = item;
	else
		rb->first = item;
	rb->last = item;
	
	rb->buffer_size += size;
	return rb_item_data(item);
}

static void rb_pop_first(RINGBUFFER *rb)
{
	if(rb->first)
	{
		RINGBUFFER_ITEM *next = rb->first->next;
		rb->buffer_size -= rb->first->size;
		mem_free(rb->first);
		rb->first = next;
		if(rb->first)
			rb->first->prev = NULL;
		else
			rb->last = NULL;
	}
}

static void rb_clear(RINGBUFFER *rb)
{
	while(rb->first)
		rb_pop_first(rb);
}


typedef struct
{
	unsigned short seq;
	unsigned short ack;
	unsigned state;
	
	int token;
	
	int remote_closed;
	
	int connected;
	int disconnected;
	
	RINGBUFFER buffer;
	
	int64 last_update_time;
	int64 last_recv_time;
	int64 last_send_time;
	
	char error_string[256];
	
	NETADDR4 peeraddr;
	NETSOCKET socket;
	NETSTATS stats;
} NETCONNECTION;

typedef struct
{
	NETCONNECTION conn;
} NETSLOT;

struct NETSERVER_t
{
	NETSOCKET socket;
	NETSLOT slots[NETWORK_MAX_CLIENTS];
	int max_clients;
	NETFUNC_NEWCLIENT new_client;
	NETFUNC_NEWCLIENT del_client;
	void *user_ptr;
	unsigned char recv_buffer[NETWORK_MAX_PACKET_SIZE];
} ;

struct NETCLIENT_t
{
	NETADDR4 server_addr;
	NETSOCKET socket;
	unsigned char recv_buffer[NETWORK_MAX_PACKET_SIZE];
	
	NETCONNECTION conn;
};

static void conn_reset_stats(NETCONNECTION *conn)
{
	mem_zero(&conn->stats, sizeof(conn->stats));
}

static void conn_reset(NETCONNECTION *conn)
{
	conn->seq = 0;
	conn->ack = 0;
	conn->remote_closed = 0;
	
	if(conn->state == NETWORK_CONNSTATE_ONLINE ||
		conn->state == NETWORK_CONNSTATE_ERROR)
	{
		conn->disconnected++;
	}
		
	conn->state = NETWORK_CONNSTATE_OFFLINE;
	conn->last_send_time = 0;
	conn->last_recv_time = 0;
	conn->last_update_time = 0;
	conn->token = -1;
	
	rb_clear(&conn->buffer);
}


static const char *conn_error(NETCONNECTION *conn)
{
	return conn->error_string;
}

static void conn_set_error(NETCONNECTION *conn, const char *str)
{
	strcpy(conn->error_string, str);
}

/*
static int conn_state(NETCONNECTION *conn)
{
	return conn->state;
}*/

static void conn_init(NETCONNECTION *conn, NETSOCKET socket)
{
	conn_reset(conn);
	conn_reset_stats(conn);
	conn->socket = socket;
	conn->connected = 0;
	conn->disconnected = 0;
	rb_init(&conn->buffer);
	mem_zero(conn->error_string, sizeof(conn->error_string));
}

static void conn_ack(NETCONNECTION *conn, int ack)
{
	while(1)
	{
		RINGBUFFER_ITEM *item = conn->buffer.first;
		NETPACKETDATA *resend;
		if(!item)
			break;
			
		resend = (NETPACKETDATA *)rb_item_data(item);
		if(resend->seq <= ack || (ack < NETWORK_MAX_SEQACK/3 && resend->seq > NETWORK_MAX_SEQACK/2))
			rb_pop_first(&conn->buffer);
		else
			break;
	}
}

static void conn_send_raw(NETCONNECTION *conn, NETPACKETDATA *data)
{
	conn->last_send_time = time_get();
	conn->stats.send_packets++;
	conn->stats.send_bytes += data->data_size + NETWORK_HEADER_SIZE;
	send_packet(conn->socket, &conn->peeraddr, data);
}

static void conn_resend(NETCONNECTION *conn)
{
	RINGBUFFER_ITEM *item = conn->buffer.first;
	while(item)
	{
		NETPACKETDATA *resend = (NETPACKETDATA *)rb_item_data(item);
		conn->stats.resend_packets++;
		conn->stats.resend_bytes += resend->data_size + NETWORK_HEADER_SIZE;
		conn_send_raw(conn, resend);
		item = item->next;
	}
}

static void conn_send(NETCONNECTION *conn, int flags, int data_size, const void *data)
{
	NETPACKETDATA p;

	if(flags&NETWORK_PACKETFLAG_VITAL)
		conn->seq = (conn->seq+1)%NETWORK_MAX_SEQACK;

	p.ID[0] = 'T';
	p.ID[1] = 'W';
	p.version = NETWORK_VERSION;
	p.flags = flags;
	p.seq = conn->seq;
	p.ack = conn->ack;
	p.crc = 0;
	p.token = conn->token;
	p.data_size = data_size;
	p.data = (unsigned char *)data;
	p.first_send_time = time_get();

	if(flags&NETWORK_PACKETFLAG_VITAL)
	{
		/* save packet if we need to resend */
		NETPACKETDATA *resend = (NETPACKETDATA *)rb_alloc(&conn->buffer, sizeof(NETPACKETDATA)+data_size);
		*resend = p;
		resend->data = (unsigned char *)(resend+1);
		mem_copy(resend->data, p.data, p.data_size);
	}
	
	/* TODO: calc crc */
	conn_send_raw(conn, &p);
}

static int conn_connect(NETCONNECTION *conn, NETADDR4 *addr)
{
	if(conn->state != NETWORK_CONNSTATE_OFFLINE)
		return -1;
	
	/* init connection */
	conn_reset(conn);
	conn->peeraddr = *addr;
	conn->token = current_token++;
	mem_zero(conn->error_string, sizeof(conn->error_string));
	conn->state = NETWORK_CONNSTATE_CONNECT;
	conn_send(conn, NETWORK_PACKETFLAG_CONNECT, 0, 0);
	return 0;
}

static void conn_disconnect(NETCONNECTION *conn, const char *reason)
{
	if(conn->remote_closed == 0)
	{
		if(reason)
			conn_send(conn, NETWORK_PACKETFLAG_CLOSE, strlen(reason)+1, reason);
		else
			conn_send(conn, NETWORK_PACKETFLAG_CLOSE, 0, 0);

		conn->error_string[0] = 0;
		if(reason)
			strcpy(conn->error_string, reason);
	}
	
	conn_reset(conn);
}

static int conn_feed(NETCONNECTION *conn, NETPACKETDATA *p, NETADDR4 *addr)
{
	conn->last_recv_time = time_get();
	conn->stats.recv_packets++;
	conn->stats.recv_bytes += p->data_size + NETWORK_HEADER_SIZE;
	
	if(p->flags&NETWORK_PACKETFLAG_CLOSE)
	{
		conn->state = NETWORK_CONNSTATE_ERROR;
		conn->remote_closed = 1;
		
		if(p->data_size)
			conn_set_error(conn, (char *)p->data);
		else
			conn_set_error(conn, "no reason given");
		if(config.debug)
			dbg_msg("conn", "closed reason='%s'", conn_error(conn));
		return 0;
	}
	
	if(conn->state == NETWORK_CONNSTATE_OFFLINE)
	{
		if(p->flags == NETWORK_PACKETFLAG_CONNECT)
		{
			/* send response and init connection */
			conn->state = NETWORK_CONNSTATE_ONLINE;
			conn->connected++;
			conn->peeraddr = *addr;
			conn->token = p->token;
			conn_send(conn, NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT, 0, 0);
			if(config.debug)
				dbg_msg("connection", "got connection, sending connect+accept");
		}
	}
	else if(net_addr4_cmp(&conn->peeraddr, addr) == 0)
	{
		if(p->token != conn->token)
			return 0;

		if(conn->state == NETWORK_CONNSTATE_ONLINE)
		{
			/* remove packages that are acked */
			conn_ack(conn, p->ack);
			
			/* check if resend is requested */
			if(p->flags&NETWORK_PACKETFLAG_RESEND)
				conn_resend(conn);
				
			if(p->flags&NETWORK_PACKETFLAG_VITAL)
			{
				if(p->seq == (conn->ack+1)%NETWORK_MAX_SEQACK)
				{
					/* in sequence */
					conn->ack = (conn->ack+1)%NETWORK_MAX_SEQACK;
				}
				else
				{
					/* out of sequence, request resend */
					dbg_msg("conn", "asking for resend %d %d", p->seq, (conn->ack+1)%NETWORK_MAX_SEQACK);
					conn_send(conn, NETWORK_PACKETFLAG_RESEND, 0, 0);
					return 0;
				}
			}
			else
			{
				if(p->seq > conn->ack)
					conn_send(conn, NETWORK_PACKETFLAG_RESEND, 0, 0);
			}
			
			if(p->data_size == 0)
				return 0;
				
			return 1;
		}
		else if(conn->state == NETWORK_CONNSTATE_CONNECT)
		{
			/* connection made */
			if(p->flags == (NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT))
			{
				conn_send(conn, NETWORK_PACKETFLAG_ACCEPT, 0, 0);
				conn->state = NETWORK_CONNSTATE_ONLINE;
				conn->connected++;
				dbg_msg("connection", "got connect+accept, sending accept. connection online");
			}
		}
		/*
		else if(conn->state == NETWORK_CONNSTATE_CONNECTACCEPTED)
		{
			// connection made
			if(p->flags == NETWORK_PACKETFLAG_ACCEPT)
			{
				conn->state = NETWORK_CONNSTATE_ONLINE;
				dbg_msg("connection", "got accept. connection online");
			}
		}*/
		else
		{
			/* strange packet, wrong state */
			conn->state = NETWORK_CONNSTATE_ERROR;
			conn_set_error(conn, "strange state and packet");
		}
	}
	else
	{
		/* strange packet, not ment for me */
	}
	
	return 0;
}



static int conn_update(NETCONNECTION *conn)
{
	int64 now = time_get();

	if(conn->state == NETWORK_CONNSTATE_OFFLINE || conn->state == NETWORK_CONNSTATE_ERROR)
		return 0;

	/* watch out for major hitches */
	{
		int64 delta = now-conn->last_update_time;
		if(conn->last_update_time && delta > time_freq()/2)
		{
			RINGBUFFER_ITEM *item = conn->buffer.first;
	
			dbg_msg("conn", "hitch %d", (int)((delta*1000)/time_freq()));
			conn->last_recv_time += delta;
	
			while(item)
			{
				NETPACKETDATA *resend = (NETPACKETDATA *)rb_item_data(item);
				resend->first_send_time += delta;
				item = item->next;
			}
		}

		conn->last_update_time = now;
	}
		
	
	/* check for timeout */
	if(conn->state != NETWORK_CONNSTATE_OFFLINE &&
		conn->state != NETWORK_CONNSTATE_CONNECT &&
		(now-conn->last_recv_time) > time_freq()*10)
	{
		conn->state = NETWORK_CONNSTATE_ERROR;
		conn_set_error(conn, "timeout");
	}
	
	/* check for large buffer errors */
	if(conn->buffer.buffer_size > 1024*64)
	{
		conn->state = NETWORK_CONNSTATE_ERROR;
		conn_set_error(conn, "too weak connection (out of buffer)");
	}
	
	if(conn->buffer.first)
	{
		NETPACKETDATA *resend = (NETPACKETDATA *)(conn->buffer.first+1);
		if(now-resend->first_send_time > time_freq()*10)
		{
			conn->state = NETWORK_CONNSTATE_ERROR;
			conn_set_error(conn, "too weak connection (not acked for 10 seconds)");
		}
	}
	
	/* send keep alives if nothing has happend for 250ms */
	if(conn->state == NETWORK_CONNSTATE_ONLINE)
	{
		if(time_get()-conn->last_send_time> time_freq()/4)
			conn_send(conn, NETWORK_PACKETFLAG_VITAL, 0, 0);
	}
	else if(conn->state == NETWORK_CONNSTATE_CONNECT)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) /* send a new connect every 500ms */
			conn_send(conn, NETWORK_PACKETFLAG_CONNECT, 0, 0);
	}
	else if(conn->state == NETWORK_CONNSTATE_CONNECTACCEPTED)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) /* send a new connect/accept every 500ms */
			conn_send(conn, NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT, 0, 0);
	}
	
	return 0;
}


static int check_packet(unsigned char *buffer, int size, NETPACKETDATA *packet)
{
	/* check the size */
	if(size < NETWORK_HEADER_SIZE || size > NETWORK_MAX_PACKET_SIZE)
		return -1;
	
	/* read the packet */
	packet->ID[0] = 'T';
	packet->ID[1] = 'W';
	packet->version = NETWORK_VERSION;
	packet->flags = buffer[0];
	packet->seq = ((buffer[1]&0xf0)<<4)|buffer[2];
	packet->ack = ((buffer[1]&0x0f)<<8)|buffer[3];
	packet->crc = 0;
	packet->token = (buffer[4]<<8)|buffer[5];
	packet->data_size = size - NETWORK_HEADER_SIZE;
	packet->data = buffer+NETWORK_HEADER_SIZE;
	
	/* check the packet */
	if(packet->ID[0] != 'T' || packet->ID[1] != 'W')
		return 1;
	
	if(packet->version != NETWORK_VERSION)
		return 1;

	/* TODO: perform crc check */
	
	/* return success */
	return 0;
}

NETSERVER *netserver_open(NETADDR4 bindaddr, int max_clients, int flags)
{
	int i;
	NETSERVER *server;
	NETSOCKET socket = net_udp4_create(bindaddr);
	if(socket == NETSOCKET_INVALID)
		return 0;
	
	server = (NETSERVER *)mem_alloc(sizeof(NETSERVER), 1);
	mem_zero(server, sizeof(NETSERVER));
	server->socket = socket;
	server->max_clients = max_clients;
	if(server->max_clients > NETWORK_MAX_CLIENTS)
		server->max_clients = NETWORK_MAX_CLIENTS;
	if(server->max_clients < 1)
		server->max_clients = 1;
	
	for(i = 0; i < NETWORK_MAX_CLIENTS; i++)
		conn_init(&server->slots[i].conn, server->socket);

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
	dbg_msg("net_server", "client dropped. cid=%d reason=\"%s\"", client_id, reason);
	conn_disconnect(&s->slots[client_id].conn, reason);

	if(s->del_client)
		s->del_client(client_id, s->user_ptr);
		
	return 0;
}

int netserver_update(NETSERVER *s)
{
	int i;
	for(i = 0; i < s->max_clients; i++)
	{
		conn_update(&s->slots[i].conn);
		if(s->slots[i].conn.state == NETWORK_CONNSTATE_ERROR)
			netserver_drop(s, i, conn_error(&s->slots[i].conn));
	}
	return 0;
}

int netserver_recv(NETSERVER *s, NETPACKET *packet)
{
	NETPACKETDATA data;
	int i, r, bytes, found;
	NETADDR4 addr;
	
	while(1)
	{
		bytes = net_udp4_recv(s->socket, &addr, s->recv_buffer, NETWORK_MAX_PACKET_SIZE);

		/* no more packets for now */
		if(bytes <= 0)
			break;
		
		r = check_packet(s->recv_buffer, bytes, &data);
		if(r == 0)
		{
			if(data.flags&NETWORK_PACKETFLAG_CONNLESS)
			{
				/* connection less packets */
				packet->client_id = -1;
				packet->address = addr;
				packet->flags = PACKETFLAG_CONNLESS;
				packet->data_size = data.data_size;
				packet->data = data.data;
				return 1;		
			}
			else
			{
				/* ok packet, process it */
				if(data.flags == NETWORK_PACKETFLAG_CONNECT)
				{
					found = 0;
					
					/* check if we already got this client */
					for(i = 0; i < s->max_clients; i++)
					{
						if(s->slots[i].conn.state != NETWORK_CONNSTATE_OFFLINE &&
							net_addr4_cmp(&s->slots[i].conn.peeraddr, &addr) == 0)
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
							if(s->slots[i].conn.state == NETWORK_CONNSTATE_OFFLINE)
							{
								found = 1;
								conn_feed(&s->slots[i].conn, &data, &addr);
								if(s->new_client)
									s->new_client(i, s->user_ptr);
								break;
							}
						}
					}
					
					if(!found)
					{
						/* send connectionless packet */
						const char errstring[] = "server full";
						NETPACKETDATA p;
						p.ID[0] = 'T';
						p.ID[1] = 'W';
						p.version = NETWORK_VERSION;
						p.flags = NETWORK_PACKETFLAG_CLOSE;
						p.seq = 0;
						p.ack = 0;
						p.crc = 0;
						p.token = data.token;
						p.data_size = sizeof(errstring);
						p.data = (unsigned char *)errstring;
						send_packet(s->socket, &addr, &p);
					}
				}
				else
				{
					/* find matching slot */
					for(i = 0; i < s->max_clients; i++)
					{
						if(net_addr4_cmp(&s->slots[i].conn.peeraddr, &addr) == 0)
						{
							if(conn_feed(&s->slots[i].conn, &data, &addr))
							{
								if(data.data_size)
								{
									packet->client_id = i;	
									packet->address = addr;
									packet->flags = 0;
									packet->data_size = data.data_size;
									packet->data = data.data;
									return 1;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			/* errornous packet, drop it */
			dbg_msg("server", "crazy packet");
		}
		
		/* read header */
		/* do checksum */
	}	
	
	return 0;
}

int netserver_send(NETSERVER *s, NETPACKET *packet)
{
	dbg_assert(packet->data_size < NETWORK_MAX_PAYLOAD, "packet payload too big");
	
	if(packet->flags&PACKETFLAG_CONNLESS)
	{
		/* send connectionless packet */
		NETPACKETDATA p;
		p.ID[0] = 'T';
		p.ID[1] = 'W';
		p.version = NETWORK_VERSION;
		p.flags = NETWORK_PACKETFLAG_CONNLESS;
		p.seq = 0;
		p.ack = 0;
		p.crc = 0;
		p.data_size = packet->data_size;
		p.data = (unsigned char *)packet->data;
		send_packet(s->socket, &packet->address, &p);
	}
	else
	{
		int flags  = 0;
		dbg_assert(packet->client_id >= 0, "errornous client id");
		dbg_assert(packet->client_id < s->max_clients, "errornous client id");
		if(packet->flags&PACKETFLAG_VITAL)
			flags |= NETWORK_PACKETFLAG_VITAL;
		conn_send(&s->slots[packet->client_id].conn, flags, packet->data_size, packet->data);
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
		if(s->slots[c].conn.state != NETWORK_CONNSTATE_OFFLINE)
		{
			int *sstats = (int *)(&(s->slots[c].conn.stats));
			for(i = 0; i < num_stats; i++)
				istats[i] += sstats[i];
		}
	}
}

NETCLIENT *netclient_open(NETADDR4 bindaddr, int flags)
{
	NETCLIENT *client = (NETCLIENT *)mem_alloc(sizeof(NETCLIENT), 1);
	mem_zero(client, sizeof(NETCLIENT));
	client->socket = net_udp4_create(bindaddr);
	conn_init(&client->conn, client->socket);
	return client;
}

int netclient_close(NETCLIENT *c)
{
	/* TODO: implement me */
	return 0;
}

int netclient_update(NETCLIENT *c)
{
	conn_update(&c->conn);
	if(c->conn.state == NETWORK_CONNSTATE_ERROR)
		netclient_disconnect(c, conn_error(&c->conn));
	return 0;
}

int netclient_disconnect(NETCLIENT *c, const char *reason)
{
	dbg_msg("netclient", "disconnected. reason=\"%s\"", reason);
	conn_disconnect(&c->conn, reason);
	return 0;
}

int netclient_connect(NETCLIENT *c, NETADDR4 *addr)
{
	conn_connect(&c->conn, addr);
	return 0;
}

int netclient_recv(NETCLIENT *c, NETPACKET *packet)
{
	while(1)
	{
		NETADDR4 addr;
		NETPACKETDATA data;
		int r;
		int bytes = net_udp4_recv(c->socket, &addr, c->recv_buffer, NETWORK_MAX_PACKET_SIZE);

		/* no more packets for now */
		if(bytes <= 0)
			break;
		
		r = check_packet(c->recv_buffer, bytes, &data);
		
		if(r == 0)
		{
			if(data.flags&NETWORK_PACKETFLAG_CONNLESS)
			{
				/* connection less packets */
				packet->client_id = -1;
				packet->address = addr;
				packet->flags = PACKETFLAG_CONNLESS;
				packet->data_size = data.data_size;
				packet->data = data.data;
				return 1;		
			}
			else
			{
				if(conn_feed(&c->conn, &data, &addr))
				{
					/* fill in packet */
					packet->client_id = 0;
					packet->address = addr;
					packet->flags = 0;
					packet->data_size = data.data_size;
					packet->data = data.data;
					return 1;
				}
				else
				{
					/* errornous packet, drop it */
				}
			}			
		}
	}
	
	return 0;
}

int netclient_send(NETCLIENT *c, NETPACKET *packet)
{
	dbg_assert(packet->data_size < NETWORK_MAX_PAYLOAD, "packet payload too big");
	
	if(packet->flags&PACKETFLAG_CONNLESS)
	{
		/* send connectionless packet */
		NETPACKETDATA p;
		p.ID[0] = 'T';
		p.ID[1] = 'W';
		p.version = NETWORK_VERSION;
		p.flags = NETWORK_PACKETFLAG_CONNLESS;
		p.seq = 0;
		p.ack = 0;
		p.crc = 0;
		p.token = 0;
		p.data_size = packet->data_size;
		p.data = (unsigned char *)packet->data;
		send_packet(c->socket, &packet->address, &p);
	}
	else
	{
		int flags = 0;		
		dbg_assert(packet->client_id == 0, "errornous client id");
		if(packet->flags&PACKETFLAG_VITAL)
			flags |= NETWORK_PACKETFLAG_VITAL;
		conn_send(&c->conn, flags, packet->data_size, packet->data);
	}
	return 0;
}

int netclient_state(NETCLIENT *c)
{
	if(c->conn.state == NETWORK_CONNSTATE_ONLINE)
		return NETSTATE_ONLINE;
	if(c->conn.state == NETWORK_CONNSTATE_OFFLINE)
		return NETSTATE_OFFLINE;
	return NETSTATE_CONNECTING;
}

void netclient_stats(NETCLIENT *c, NETSTATS *stats)
{
	*stats = c->conn.stats;
}

const char *netclient_error_string(NETCLIENT *c)
{
	return conn_error(&c->conn);
}
