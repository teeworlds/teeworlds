#include <baselib/system.h>
#include <string.h>

#include "network.h"
#include "ringbuffer.h"

/*
	header:
		unsigned char ID[2]; 2 'T' 'W'
		unsigned char version; 3 
		unsigned char flags; 4 
		unsigned short seq;  6
		unsigned short ack;  8
		unsigned crc;       12 bytes

	header v2:
		unsigned char flags;		1
		unsigned char seq_ack[3];	4
		unsigned char token[2];		6
*/

enum
{
	NETWORK_VERSION = 1,
	
	NETWORK_HEADER_SIZE = 6,
	NETWORK_MAX_PACKET_SIZE = 1024,
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
};

static int current_token = 1;

struct NETPACKETDATA
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
};


static void send_packet(NETSOCKET socket, NETADDR4 *addr, NETPACKETDATA *packet)
{
	unsigned char buffer[NETWORK_MAX_PACKET_SIZE];
	buffer[0] = packet->flags;
	buffer[1] = ((packet->seq>>4)&0xf0) | ((packet->ack>>8)&0x0f);
	buffer[2] = packet->seq;
	buffer[3] = packet->ack;
	buffer[4] = packet->token>>8;
	buffer[5] = packet->token&0xff;
	mem_copy(buffer+NETWORK_HEADER_SIZE, packet->data, packet->data_size);
	int send_size = NETWORK_HEADER_SIZE+packet->data_size;
	//dbg_msg("network", "sending packet, size=%d (%d + %d)", send_size, NETWORK_HEADER_SIZE, packet->data_size);
	net_udp4_send(socket, addr, buffer, send_size);
}

struct NETCONNECTION
{
	unsigned seq;
	unsigned ack;
	unsigned state;
	
	int token;
	
	int connected;
	int disconnected;
	
	ring_buffer buffer;
	
	int64 last_recv_time;
	int64 last_send_time;
	char error_string[256];
	
	NETADDR4 peeraddr;
	NETSOCKET socket;
	NETSTATS stats;
};

struct NETSLOT
{
	NETCONNECTION conn;
};

struct NETSERVER
{
	NETSOCKET socket;
	NETSLOT slots[NETWORK_MAX_CLIENTS];
	unsigned char recv_buffer[NETWORK_MAX_PACKET_SIZE];
};

struct NETCLIENT
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
	//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_OFFLINE);
	
	if(conn->state == NETWORK_CONNSTATE_ONLINE ||
		conn->state == NETWORK_CONNSTATE_ERROR)
	{
		conn->disconnected++;
	}
		
	conn->state = NETWORK_CONNSTATE_OFFLINE;
	conn->last_send_time = 0;
	conn->last_recv_time = 0;
	conn->token = -1;
	conn->buffer.reset();
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
	mem_zero(conn->error_string, sizeof(conn->error_string));
}

static void conn_ack(NETCONNECTION *conn, int ack)
{
	while(1)
	{
		ring_buffer::item *i = conn->buffer.first();
		if(!i)
			break;
			
		NETPACKETDATA *resend = (NETPACKETDATA *)i->data();
		if(resend->seq <= ack)
			conn->buffer.pop_first();
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
	ring_buffer::item *i = conn->buffer.first();
	while(i)
	{
		NETPACKETDATA *resend = (NETPACKETDATA *)i->data();
		conn->stats.resend_packets++;
		conn->stats.resend_bytes += resend->data_size + NETWORK_HEADER_SIZE;
		conn_send_raw(conn, resend);
		i = i->next;
	}
}

static void conn_send(NETCONNECTION *conn, int flags, int data_size, const void *data)
{
	if(flags&NETWORK_PACKETFLAG_VITAL)
		conn->seq++;
	
	NETPACKETDATA p;
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
		// save packet if we need to resend
		NETPACKETDATA *resend = (NETPACKETDATA *)conn->buffer.alloc(sizeof(NETPACKETDATA)+p.data_size);
		*resend = p;
		resend->data = (unsigned char *)(resend+1);
		mem_copy(resend->data, p.data, p.data_size);
	}
	
	// TODO: calc crc
	conn_send_raw(conn, &p);
}

static int conn_connect(NETCONNECTION *conn, NETADDR4 *addr)
{
	if(conn->state != NETWORK_CONNSTATE_OFFLINE)
		return -1;
	
	// init connection
	conn_reset(conn);
	conn->peeraddr = *addr;
	conn->token = current_token++;
	//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_CONNECT);
	conn->state = NETWORK_CONNSTATE_CONNECT;
	conn_send(conn, NETWORK_PACKETFLAG_CONNECT, 0, 0);
	return 0;
}

static void conn_disconnect(NETCONNECTION *conn, const char *reason)
{
	if(reason)
		conn_send(conn, NETWORK_PACKETFLAG_CLOSE, strlen(reason)+1, reason);
	else
		conn_send(conn, NETWORK_PACKETFLAG_CLOSE, 0, 0);
	conn_reset(conn);
}

static int conn_feed(NETCONNECTION *conn, NETPACKETDATA *p, NETADDR4 *addr)
{
	conn->last_recv_time = time_get();
	conn->stats.recv_packets++;
	conn->stats.recv_bytes += p->data_size + NETWORK_HEADER_SIZE;
	
	if(p->flags&NETWORK_PACKETFLAG_CLOSE)
	{
		conn_reset(conn);
		if(p->data_size)
			conn_set_error(conn, (char *)p->data);
		else
			conn_set_error(conn, "no reason given");
		dbg_msg("conn", "closed reason='%s'", conn_error(conn));
		return 0;
	}
	
	if(conn->state == NETWORK_CONNSTATE_OFFLINE)
	{
		if(p->flags == NETWORK_PACKETFLAG_CONNECT)
		{
			// send response and init connection
			//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_ONLINE);
			conn->state = NETWORK_CONNSTATE_ONLINE;
			conn->connected++;
			conn->peeraddr = *addr;
			conn->token = p->token;
			//dbg_msg("connection", "token set to %d", p->token);
			conn_send(conn, NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT, 0, 0);
			dbg_msg("connection", "got connection, sending connect+accept");
		}
	}
	else if(net_addr4_cmp(&conn->peeraddr, addr) == 0)
	{
		if(p->token != conn->token)
		{
			//dbg_msg("connection", "wrong token %d, %d", p->token, conn->token);
			return 0;
		}

		if(conn->state == NETWORK_CONNSTATE_ONLINE)
		{
			// remove packages that are acked
			conn_ack(conn, p->ack);
			
			// 
			if(p->flags&NETWORK_PACKETFLAG_RESEND)
				conn_resend(conn);
				
			if(p->flags&NETWORK_PACKETFLAG_VITAL)
			{
				if(p->seq == conn->ack+1)
				{
					// in sequence
					conn->ack++;
				}
				else
				{
					// out of sequence, request resend
					//dbg_msg("conn", "asking for resend");
					conn_send(conn, NETWORK_PACKETFLAG_RESEND, 0, 0);
					return 0;
				}
			}
			else
			{
				if(p->seq > conn->ack)
				{
					//dbg_msg("conn", "asking for resend");
					conn_send(conn, NETWORK_PACKETFLAG_RESEND, 0, 0);
				}
			}
			
			if(p->data_size == 0)
				return 0;
				
			return 1;
		}
		else if(conn->state == NETWORK_CONNSTATE_CONNECT)
		{
			// connection made
			if(p->flags == (NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT))
			{
				conn_send(conn, NETWORK_PACKETFLAG_ACCEPT, 0, 0);
				//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_ONLINE);
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
			conn_reset(conn);
			// strange packet, wrong state
		}
	}
	else
	{
		// strange packet, not ment for me
	}
	
	return 0;
}



static int conn_update(NETCONNECTION *conn)
{
	if(conn->state == NETWORK_CONNSTATE_OFFLINE || conn->state == NETWORK_CONNSTATE_ERROR)
		return 0;
	
	// check for timeout
	if(conn->state != NETWORK_CONNSTATE_OFFLINE &&
		conn->state != NETWORK_CONNSTATE_CONNECT &&
		(time_get()-conn->last_recv_time) > time_freq()*3)
	{
		//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_ERROR);
		conn->state = NETWORK_CONNSTATE_ERROR;
		conn_set_error(conn, "timeout");
	}
	
	// check for large buffer errors
	if(conn->buffer.size() > 1024*64)
	{
		//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_ERROR);
		conn->state = NETWORK_CONNSTATE_ERROR;
		conn_set_error(conn, "too weak connection (out of buffer)");
	}
	
	if(conn->buffer.first())
	{
		NETPACKETDATA *resend = (NETPACKETDATA *)conn->buffer.first()->data();
		if(time_get()-resend->first_send_time > time_freq()*3)
		{
			//dbg_msg("connection", "state = %d->%d", conn->state, NETWORK_CONNSTATE_ERROR);
			conn->state = NETWORK_CONNSTATE_ERROR;
			conn_set_error(conn, "too weak connection (not acked for 3 seconds)");
		}
	}
	
	// send keep alives if nothing has happend for 250ms
	if(conn->state == NETWORK_CONNSTATE_ONLINE)
	{
		if(time_get()-conn->last_send_time> time_freq()/4)
			conn_send(conn, NETWORK_PACKETFLAG_VITAL, 0, 0);
	}
	else if(conn->state == NETWORK_CONNSTATE_CONNECT)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) // send a new connect every 500ms
			conn_send(conn, NETWORK_PACKETFLAG_CONNECT, 0, 0);
	}
	else if(conn->state == NETWORK_CONNSTATE_CONNECTACCEPTED)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) // send a new connect/accept every 500ms
			conn_send(conn, NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT, 0, 0);
	}
	
	return 0;
}


static int check_packet(unsigned char *buffer, int size, NETPACKETDATA *packet)
{
	// check the size
	if(size < NETWORK_HEADER_SIZE || size > NETWORK_MAX_PACKET_SIZE)
		return -1;
	
	// read the packet
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
	
	// check the packet
	if(packet->ID[0] != 'T' || packet->ID[1] != 'W')
		return 1;
	
	if(packet->version != NETWORK_VERSION)
		return 1;

	// TODO: perform crc check
	
	// return success
	return 0;
}

NETSERVER *net_server_open(int port, int max_clients, int flags)
{
	NETSERVER *server = (NETSERVER *)mem_alloc(sizeof(NETSERVER), 1);
	mem_zero(server, sizeof(NETSERVER));
	server->socket = net_udp4_create(port);
	
	for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
		conn_init(&server->slots[i].conn, server->socket);

	return server;
}

int net_server_close(NETSERVER *s)
{
	// TODO: implement me
	return 0;
}

int net_server_newclient(NETSERVER *s)
{
	for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
	{
		if(s->slots[i].conn.connected)
		{
			s->slots[i].conn.connected = 0;
			return i;
		}
	}
	
	return -1;
}

int net_server_delclient(NETSERVER *s)
{
	for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
	{
		if(s->slots[i].conn.disconnected)
		{
			s->slots[i].conn.disconnected = 0;
			return i;
		}
	}
	
	return -1;
}

int net_server_drop(NETSERVER *s, int client_id, const char *reason)
{
	// TODO: insert lots of checks here
	dbg_msg("net_server", "client dropped. cid=%d reason=\"%s\"", client_id, reason);
	conn_disconnect(&s->slots[client_id].conn, reason);
	//conn_reset(&s->slots[client_id].conn);
	return 0;
}

int net_server_update(NETSERVER *s)
{
	for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
	{
		conn_update(&s->slots[i].conn);
		if(s->slots[i].conn.state == NETWORK_CONNSTATE_ERROR)
			net_server_drop(s, i, conn_error(&s->slots[i].conn));
	}
	return 0;
}

int net_server_recv(NETSERVER *s, NETPACKET *packet)
{
	while(1)
	{
		NETADDR4 addr;
		int bytes = net_udp4_recv(s->socket, &addr, s->recv_buffer, NETWORK_MAX_PACKET_SIZE);

		// no more packets for now
		if(bytes <= 0)
			break;
		
		NETPACKETDATA data;
		int r = check_packet(s->recv_buffer, bytes, &data);
		if(r == 0)
		{
			if(data.flags&NETWORK_PACKETFLAG_CONNLESS)
			{
				// connection less packets
				packet->client_id = -1;
				packet->address = addr;
				packet->flags = PACKETFLAG_CONNLESS;
				packet->data_size = data.data_size;
				packet->data = data.data;
				return 1;		
			}
			else
			{
				// ok packet, process it
				if(data.flags == NETWORK_PACKETFLAG_CONNECT)
				{
					int found = 0;
					
					// check if we already got this client
					for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
					{
						if(s->slots[i].conn.state != NETWORK_CONNSTATE_OFFLINE &&
							net_addr4_cmp(&s->slots[i].conn.peeraddr, &addr) == 0)
						{
							found = 1; // silent ignore.. we got this client already
							//dbg_msg("netserver", "ignored connect request %d", i);
							break;
						}
					}
					
					// client that wants to connect
					if(!found)
					{
						for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
						{
							if(s->slots[i].conn.state == NETWORK_CONNSTATE_OFFLINE)
							{
								//dbg_msg("netserver", "connection started %d", i);
								conn_feed(&s->slots[i].conn, &data, &addr);
								found = 1;
								break;
							}
						}
					}
					
					if(found)
					{
						// TODO: send error
					}
				}
				else
				{
					// find matching slot
					for(int i = 0; i < NETWORK_MAX_CLIENTS; i++)
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
			// errornous packet, drop it
			dbg_msg("server", "crazy packet");
		}
		
		// read header
		// do checksum
	}	
	
	return 0;
}

int net_server_send(NETSERVER *s, NETPACKET *packet)
{
	if(packet->flags&PACKETFLAG_CONNLESS)
	{
		// send connectionless packet
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
		dbg_assert(packet->client_id >= 0, "errornous client id");
		dbg_assert(packet->client_id < NETWORK_MAX_CLIENTS, "errornous client id");
		int flags  = 0;
		if(packet->flags&PACKETFLAG_VITAL)
			flags |= NETWORK_PACKETFLAG_VITAL;
		conn_send(&s->slots[packet->client_id].conn, flags, packet->data_size, packet->data);
	}
	return 0;
}

void net_server_stats(NETSERVER *s, NETSTATS *stats)
{
	mem_zero(stats, sizeof(NETSTATS));
	
	int num_stats = sizeof(NETSTATS)/sizeof(int);
	int *istats = (int *)stats;
	
	for(int c = 0; c < NETWORK_MAX_CLIENTS; c++)
	{
		int *sstats = (int *)(&(s->slots[c].conn.stats));
		for(int i = 0; i < num_stats; i++)
			istats[i] += sstats[i];
	}
}

//
NETCLIENT *net_client_open(int port, int flags)
{
	NETCLIENT *client = (NETCLIENT *)mem_alloc(sizeof(NETCLIENT), 1);
	mem_zero(client, sizeof(NETCLIENT));
	client->socket = net_udp4_create(port);
	conn_init(&client->conn, client->socket);
	return client;
}

int net_client_close(NETCLIENT *c)
{
	// TODO: implement me
	return 0;
}

int net_client_update(NETCLIENT *c)
{
	// TODO: implement me
	conn_update(&c->conn);
	if(c->conn.state == NETWORK_CONNSTATE_ERROR)
		net_client_disconnect(c, conn_error(&c->conn));
	return 0;
}

int net_client_disconnect(NETCLIENT *c, const char *reason)
{
	// TODO: do this more graceful
	dbg_msg("net_client", "disconnected. reason=\"%s\"", reason);
	conn_disconnect(&c->conn, reason);
	return 0;
}

int net_client_connect(NETCLIENT *c, NETADDR4 *addr)
{
	//net_client_disconnect(c);
	conn_connect(&c->conn, addr);
	return 0;
}

int net_client_recv(NETCLIENT *c, NETPACKET *packet)
{
	while(1)
	{
		NETADDR4 addr;
		int bytes = net_udp4_recv(c->socket, &addr, c->recv_buffer, NETWORK_MAX_PACKET_SIZE);

		// no more packets for now
		if(bytes <= 0)
			break;
		
		NETPACKETDATA data;
		int r = check_packet(c->recv_buffer, bytes, &data);
		
		if(r == 0)
		{
			if(data.flags&NETWORK_PACKETFLAG_CONNLESS)
			{
				// connection less packets
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
					// fill in packet
					packet->client_id = 0;
					packet->address = addr;
					packet->flags = 0;
					packet->data_size = data.data_size;
					packet->data = data.data;
					return 1;
				}
				else
				{
					// errornous packet, drop it
				}
			}			
		}
	}
	
	return 0;
}

int net_client_send(NETCLIENT *c, NETPACKET *packet)
{
	if(packet->flags&PACKETFLAG_CONNLESS)
	{
		// send connectionless packet
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
		send_packet(c->socket, &packet->address, &p);
	}
	else
	{
		// TODO: insert stuff for stateless stuff
		dbg_assert(packet->client_id == 0, "errornous client id");

		int flags = 0;		
		if(packet->flags&PACKETFLAG_VITAL)
			flags |= NETWORK_PACKETFLAG_VITAL;
		conn_send(&c->conn, flags, packet->data_size, packet->data);
	}
	return 0;
}

int net_client_state(NETCLIENT *c)
{
	if(c->conn.state == NETWORK_CONNSTATE_ONLINE)
		return NETSTATE_ONLINE;
	if(c->conn.state == NETWORK_CONNSTATE_OFFLINE)
		return NETSTATE_OFFLINE;
	return NETSTATE_CONNECTING;
}

void net_client_stats(NETCLIENT *c, NETSTATS *stats)
{
	*stats = c->conn.stats;
}

const char *net_client_error_string(NETCLIENT *c)
{
	return conn_error(&c->conn);
}
