/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>

#include <string.h> /* strlen */

#include "e_config.h"
#include "e_network.h"
#include "e_huffman.h"

/*

CURRENT:
	packet header: 3 bytes
		unsigned char flags_ack; // 4bit flags, 4bit ack
		unsigned char ack; // 8 bit ack
		unsigned char num_chunks; // 8 bit chunks
		
		(unsigned char padding[3])	// 24 bit extra incase it's a connection less packet
									// this is to make sure that it's compatible with the
									// old protocol

	chunk header: 2-3 bytes
		unsigned char flags_size; // 2bit flags, 6 bit size
		unsigned char size_seq; // 4bit size, 4bit seq
		(unsigned char seq;) // 8bit seq, if vital flag is set
*/

enum
{
	NET_VERSION = 2,

	NET_MAX_CHUNKSIZE = 1024,
	NET_MAX_PAYLOAD = NET_MAX_CHUNKSIZE+16,
	NET_MAX_PACKETSIZE = NET_MAX_PAYLOAD+16,
	NET_MAX_CHUNKHEADERSIZE = 5,
	NET_PACKETHEADERSIZE = 3,
	NET_MAX_CLIENTS = 128,
	NET_MAX_SEQUENCE = 1<<10,
	NET_SEQUENCE_MASK = NET_MAX_SEQUENCE-1,

	NET_CONNSTATE_OFFLINE=0,
	NET_CONNSTATE_CONNECT=1,
	NET_CONNSTATE_CONNECTACCEPTED=2,
	NET_CONNSTATE_ONLINE=3,
	NET_CONNSTATE_ERROR=4,

	NET_PACKETFLAG_CONTROL=1,
	NET_PACKETFLAG_CONNLESS=2,
	NET_PACKETFLAG_RESEND=4,

	NET_CHUNKFLAG_VITAL=1,
	NET_CHUNKFLAG_RESEND=2,
	
	NET_CTRLMSG_KEEPALIVE=0,
	NET_CTRLMSG_CONNECT=1,
	NET_CTRLMSG_CONNECTACCEPT=2,
	NET_CTRLMSG_ACCEPT=3,
	NET_CTRLMSG_CLOSE=4,
	
	NET_ENUM_TERMINATOR
};

typedef struct
{
	int flags;
	int ack;
	int num_chunks;
	int data_size;
	unsigned char chunk_data[NET_MAX_PAYLOAD];
} NETPACKETCONSTRUCT;

typedef struct
{
	int flags;
	int size;
	int sequence;
} NETCHUNKHEADER;

typedef struct
{
	int flags;
	int data_size;
	unsigned char *data;

	int sequence;
	int64 first_send_time;
} NETCHUNKDATA;

typedef struct RINGBUFFER_ITEM
{
	struct RINGBUFFER_ITEM *next;
	struct RINGBUFFER_ITEM *prev;
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
			rb->first->prev = (void*)0;
		else
			rb->last = (void*)0;
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
	
	RINGBUFFER buffer;
	
	int64 last_update_time;
	int64 last_recv_time;
	int64 last_send_time;
	
	char error_string[256];
	
	NETPACKETCONSTRUCT construct;
	
	NETADDR peeraddr;
	NETSOCKET socket;
	NETSTATS stats;
} NETCONNECTION;

typedef struct
{
	NETCONNECTION conn;
} NETSLOT;

typedef struct
{
	NETADDR addr;
	NETCONNECTION *conn;
	int current_chunk;
	int client_id;
	int valid;
	NETPACKETCONSTRUCT data;
	unsigned char buffer[NET_MAX_PACKETSIZE];
} NETRECVINFO;

struct NETSERVER
{
	NETSOCKET socket;
	NETSLOT slots[NET_MAX_CLIENTS];
	int max_clients;
	NETFUNC_NEWCLIENT new_client;
	NETFUNC_NEWCLIENT del_client;
	void *user_ptr;
	
	NETRECVINFO recv;
};

struct NETCLIENT
{
	NETADDR server_addr;
	NETSOCKET socket;
	
	NETRECVINFO recv;
	NETCONNECTION conn;
};

static IOHANDLE datalog = 0;
static HUFFMAN_STATE huffmanstate;

#define COMPRESSION 1

/* packs the data tight and sends it */
static void send_packet_connless(NETSOCKET socket, NETADDR *addr, const void *data, int data_size)
{
	unsigned char buffer[NET_MAX_PACKETSIZE];
	buffer[0] = 0xff;
	buffer[1] = 0xff;
	buffer[2] = 0xff;
	buffer[3] = 0xff;
	buffer[4] = 0xff;
	buffer[5] = 0xff;
	mem_copy(&buffer[6], data, data_size);
	net_udp_send(socket, addr, buffer, 6+data_size);
}

static void send_packet(NETSOCKET socket, NETADDR *addr, NETPACKETCONSTRUCT *packet)
{
	unsigned char buffer[NET_MAX_PACKETSIZE];
	buffer[0] = ((packet->flags<<4)&0xf0)|((packet->ack>>8)&0xf);
	buffer[1] = packet->ack&0xff;
	buffer[2] = packet->num_chunks;
	if(datalog)
	{
		io_write(datalog, &packet->data_size, sizeof(packet->data_size));
		io_write(datalog, &packet->chunk_data, packet->data_size);
	}
	
	if(COMPRESSION)
	{
		int compressed_size = huffman_compress(&huffmanstate, packet->chunk_data, packet->data_size, &buffer[3], NET_MAX_PACKETSIZE-4);
		net_udp_send(socket, addr, buffer, NET_PACKETHEADERSIZE+compressed_size);
	}
	else
	{
		mem_copy(&buffer[3], packet->chunk_data, packet->data_size);
		net_udp_send(socket, addr, buffer, NET_PACKETHEADERSIZE+packet->data_size);
	}
}

/* TODO: rename this function */
static int unpack_packet(unsigned char *buffer, int size, NETPACKETCONSTRUCT *packet)
{
	/* check the size */
	if(size < NET_PACKETHEADERSIZE || size > NET_MAX_PACKETSIZE)
	{
		dbg_msg("", "packet too small");
		return -1;
	}
	
	/* read the packet */
	packet->flags = buffer[0]>>4;
	packet->ack = ((buffer[0]&0xf)<<8) | buffer[1];
	packet->num_chunks = buffer[2];
	packet->data_size = size - NET_PACKETHEADERSIZE;
	
	if(packet->flags&NET_PACKETFLAG_CONNLESS)
	{
		packet->flags = NET_PACKETFLAG_CONNLESS;
		packet->ack = 0;
		packet->num_chunks = 0;
		packet->data_size = size - 6;
		mem_copy(packet->chunk_data, &buffer[6], packet->data_size);
	}
	else
	{
		if(COMPRESSION)
			huffman_decompress(&huffmanstate, &buffer[3], packet->data_size, packet->chunk_data, sizeof(packet->chunk_data));
		else
			mem_copy(packet->chunk_data, &buffer[3], packet->data_size);
	}
	
	/* return success */
	return 0;
}


/* TODO: change the arguments of this function */
static unsigned char *pack_chunk_header(unsigned char *data, int flags, int size, int sequence)
{
	data[0] = ((flags&3)<<6)|((size>>4)&0x3f);
	data[1] = (size&0xf);
	if(flags&NET_CHUNKFLAG_VITAL)
	{
		data[1] |= (sequence>>2)&0xf0;
		data[2] = sequence&0xff;
		return data + 3;
	}
	return data + 2;
}

static unsigned char *unpack_chunk_header(unsigned char *data, NETCHUNKHEADER *header)
{
	header->flags = (data[0]>>6)&3;
	header->size = ((data[0]&0x3f)<<4) | (data[1]&0xf);
	header->sequence = -1;
	if(header->flags&NET_CHUNKFLAG_VITAL)
	{
		header->sequence = ((data[1]&0xf0)<<2) | data[2];
		return data + 3;
	}
	return data + 2;
}


static void conn_reset_stats(NETCONNECTION *conn)
{
	mem_zero(&conn->stats, sizeof(conn->stats));
}

static void conn_reset(NETCONNECTION *conn)
{
	conn->seq = 0;
	conn->ack = 0;
	conn->remote_closed = 0;
	
	conn->state = NET_CONNSTATE_OFFLINE;
	conn->state = NET_CONNSTATE_OFFLINE;
	conn->last_send_time = 0;
	conn->last_recv_time = 0;
	conn->last_update_time = 0;
	conn->token = -1;
	mem_zero(&conn->peeraddr, sizeof(conn->peeraddr));
	
	rb_clear(&conn->buffer);
	
	mem_zero(&conn->construct, sizeof(conn->construct));
}


static const char *conn_error(NETCONNECTION *conn)
{
	return conn->error_string;
}

static void conn_set_error(NETCONNECTION *conn, const char *str)
{
	str_copy(conn->error_string, str, sizeof(conn->error_string));
}

static void conn_init(NETCONNECTION *conn, NETSOCKET socket)
{
	conn_reset(conn);
	conn_reset_stats(conn);
	conn->socket = socket;
	rb_init(&conn->buffer);
	mem_zero(conn->error_string, sizeof(conn->error_string));
}


static void conn_ack(NETCONNECTION *conn, int ack)
{
	while(1)
	{
		RINGBUFFER_ITEM *item = conn->buffer.first;
		NETCHUNKDATA *resend;
		if(!item)
			break;
			
		resend = (NETCHUNKDATA *)rb_item_data(item);
		if(resend->sequence <= ack || (ack < NET_MAX_SEQUENCE/3 && resend->sequence > NET_MAX_SEQUENCE/2))
			rb_pop_first(&conn->buffer);
		else
			break;
	}
}

static void conn_want_resend(NETCONNECTION *conn)
{
	conn->construct.flags |= NET_PACKETFLAG_RESEND;
}

static int conn_flush(NETCONNECTION *conn)
{
	int num_chunks = conn->construct.num_chunks;
	if(!num_chunks && !conn->construct.flags)
		return 0;
	
	conn->construct.ack = conn->ack;
	send_packet(conn->socket, &conn->peeraddr, &conn->construct);
	conn->last_send_time = time_get();
	
	/* clear construct so we can start building a new package */
	mem_zero(&conn->construct, sizeof(conn->construct));
	return num_chunks;
}

/*NETCHUNKDATA *data*/

static void conn_queue_chunk(NETCONNECTION *conn, int flags, int data_size, const void *data)
{
	unsigned char *chunk_data;
	/* check if we have space for it, if not, flush the connection */
	if(conn->construct.data_size + data_size + NET_MAX_CHUNKHEADERSIZE > sizeof(conn->construct.chunk_data))
		conn_flush(conn);

	if(flags&NET_CHUNKFLAG_VITAL && !(flags&NET_CHUNKFLAG_RESEND))
		conn->seq = (conn->seq+1)%NET_MAX_SEQUENCE;

	/* pack all the data */
	chunk_data = &conn->construct.chunk_data[conn->construct.data_size];
	chunk_data = pack_chunk_header(chunk_data, flags, data_size, conn->seq);
	mem_copy(chunk_data, data, data_size);
	chunk_data += data_size;

	/* */
	conn->construct.num_chunks++;
	conn->construct.data_size = (int)(chunk_data-conn->construct.chunk_data);
	
	/* set packet flags aswell */
	
	if(flags&NET_CHUNKFLAG_VITAL && !(flags&NET_CHUNKFLAG_RESEND))
	{
		/* save packet if we need to resend */
		NETCHUNKDATA *resend = (NETCHUNKDATA *)rb_alloc(&conn->buffer, sizeof(NETCHUNKDATA)+data_size);
		resend->sequence = conn->seq;
		resend->flags = flags;
		resend->data_size = data_size;
		resend->data = (unsigned char *)(resend+1);
		resend->first_send_time = time_get();
		mem_copy(resend->data, data, data_size);
	}
}

static void conn_send_control(NETCONNECTION *conn, int controlmsg, const void *extra, int extra_size)
{
	NETPACKETCONSTRUCT construct;
	construct.flags = NET_PACKETFLAG_CONTROL;
	construct.ack = conn->ack;
	construct.num_chunks = 0;
	construct.data_size = 1+extra_size;
	construct.chunk_data[0] = controlmsg;
	mem_copy(&construct.chunk_data[1], extra, extra_size);

	/* send the control message */
	send_packet(conn->socket, &conn->peeraddr, &construct);
	conn->last_send_time = time_get();
}

static void conn_resend(NETCONNECTION *conn)
{
	int resend_count = 0;
	int max = 10;
	RINGBUFFER_ITEM *item = conn->buffer.first;
	while(item)
	{
		NETCHUNKDATA *resend = (NETCHUNKDATA *)rb_item_data(item);
		conn_queue_chunk(conn, resend->flags|NET_CHUNKFLAG_RESEND, resend->data_size, resend->data);
		item = item->next;
		max--;
		resend_count++;
		if(!max)
			break;
	}
	
	dbg_msg("conn", "resent %d packets", resend_count);
}

static int conn_connect(NETCONNECTION *conn, NETADDR *addr)
{
	if(conn->state != NET_CONNSTATE_OFFLINE)
		return -1;
	
	/* init connection */
	conn_reset(conn);
	conn->peeraddr = *addr;
	mem_zero(conn->error_string, sizeof(conn->error_string));
	conn->state = NET_CONNSTATE_CONNECT;
	conn_send_control(conn, NET_CTRLMSG_CONNECT, 0, 0);
	return 0;
}

static void conn_disconnect(NETCONNECTION *conn, const char *reason)
{
	if(conn->state == NET_CONNSTATE_OFFLINE)
		return;

	if(conn->remote_closed == 0)
	{
		if(reason)
			conn_send_control(conn, NET_CTRLMSG_CLOSE, reason, strlen(reason)+1);
		else
			conn_send_control(conn, NET_CTRLMSG_CLOSE, 0, 0);

		conn->error_string[0] = 0;
		if(reason)
			str_copy(conn->error_string, reason, sizeof(conn->error_string));
	}
	
	conn_reset(conn);
}

static int conn_feed(NETCONNECTION *conn, NETPACKETCONSTRUCT *packet, NETADDR *addr)
{
	int64 now = time_get();
	conn->last_recv_time = now;
	
	/* check if resend is requested */
	if(packet->flags&NET_PACKETFLAG_RESEND)
		conn_resend(conn);

	/* */									
	if(packet->flags&NET_PACKETFLAG_CONTROL)
	{
		int ctrlmsg = packet->chunk_data[0];
		
		if(ctrlmsg == NET_CTRLMSG_CLOSE)
		{
			conn->state = NET_CONNSTATE_ERROR;
			conn->remote_closed = 1;
			
			if(packet->data_size)
			{
				/* make sure to sanitize the error string form the other party*/
				char str[128];
				if(packet->data_size < 128)
					str_copy(str, (char *)packet->chunk_data, packet->data_size);
				else
					str_copy(str, (char *)packet->chunk_data, 128);
				str_sanitize_strong(str);
				
				/* set the error string */
				conn_set_error(conn, str);
			}
			else
				conn_set_error(conn, "no reason given");
				
			if(config.debug)
				dbg_msg("conn", "closed reason='%s'", conn_error(conn));
			return 0;			
		}
		else
		{
			if(conn->state == NET_CONNSTATE_OFFLINE)
			{
				if(ctrlmsg == NET_CTRLMSG_CONNECT)
				{
					/* send response and init connection */
					conn_reset(conn);
					conn->state = NET_CONNSTATE_ONLINE;
					conn->peeraddr = *addr;
					conn->last_send_time = now;
					conn->last_recv_time = now;
					conn->last_update_time = now;
					conn_send_control(conn, NET_CTRLMSG_CONNECTACCEPT, 0, 0);
					if(config.debug)
						dbg_msg("connection", "got connection, sending connect+accept");			
				}
			}
			else if(conn->state == NET_CONNSTATE_CONNECT)
			{
				/* connection made */
				if(ctrlmsg == NET_CTRLMSG_CONNECTACCEPT)
				{
					conn_send_control(conn, NET_CTRLMSG_ACCEPT, 0, 0);
					conn->state = NET_CONNSTATE_ONLINE;
					if(config.debug)
						dbg_msg("connection", "got connect+accept, sending accept. connection online");
				}
			}
			else if(conn->state == NET_CONNSTATE_ONLINE)
			{
				/* connection made */
				/*
				if(ctrlmsg == NET_CTRLMSG_CONNECTACCEPT)
				{
					
				}*/
			}
		}
	}
	
	if(conn->state == NET_CONNSTATE_ONLINE)
	{
		conn_ack(conn, packet->ack);
	}
	
	return 1;
}

static int conn_update(NETCONNECTION *conn)
{
	int64 now = time_get();

	if(conn->state == NET_CONNSTATE_OFFLINE || conn->state == NET_CONNSTATE_ERROR)
		return 0;

	/* watch out for major hitches */
	{
		/* TODO: fix this */
		/*
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
		*/
	}
		
	
	/* check for timeout */
	if(conn->state != NET_CONNSTATE_OFFLINE &&
		conn->state != NET_CONNSTATE_CONNECT &&
		(now-conn->last_recv_time) > time_freq()*10)
	{
		conn->state = NET_CONNSTATE_ERROR;
		conn_set_error(conn, "timeout");
	}
	
	/* check for large buffer errors */
	if(conn->buffer.buffer_size > 1024*64)
	{
		conn->state = NET_CONNSTATE_ERROR;
		conn_set_error(conn, "too weak connection (out of buffer)");
	}
	
	if(conn->buffer.first)
	{
		/* TODO: fix this */
		NETCHUNKDATA *resend = (NETCHUNKDATA *)(conn->buffer.first+1);
		if(now-resend->first_send_time > time_freq()*10)
		{
			conn->state = NET_CONNSTATE_ERROR;
			conn_set_error(conn, "too weak connection (not acked for 10 seconds)");
		}
	}
	
	/* send keep alives if nothing has happend for 1000ms */
	if(conn->state == NET_CONNSTATE_ONLINE)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) /* flush connection after 250ms if needed */
		{
			int num_flushed_chunks = conn_flush(conn);
			if(num_flushed_chunks && config.debug)
				dbg_msg("connection", "flushed connection due to timeout. %d chunks.", num_flushed_chunks);
		}
			
		if(time_get()-conn->last_send_time > time_freq())
			conn_send_control(conn, NET_CTRLMSG_KEEPALIVE, 0, 0);
	}
	else if(conn->state == NET_CONNSTATE_CONNECT)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) /* send a new connect every 500ms */
			conn_send_control(conn, NET_CTRLMSG_CONNECT, 0, 0);
			/*conn_send(conn, NETWORK_PACKETFLAG_CONNECT, 0, 0);*/
	}
	else if(conn->state == NET_CONNSTATE_CONNECTACCEPTED)
	{

		if(time_get()-conn->last_send_time > time_freq()/2) /* send a new connect/accept every 500ms */
			conn_send_control(conn, NET_CTRLMSG_CONNECTACCEPT, 0, 0);
			/*conn_send(conn, NETWORK_PACKETFLAG_CONNECT|NETWORK_PACKETFLAG_ACCEPT, 0, 0);*/
	}
	
	return 0;
}

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
		if(s->slots[i].conn.state == NET_CONNSTATE_ERROR)
			netserver_drop(s, i, conn_error(&s->slots[i].conn));
	}
	return 0;
}

static void recvinfo_clear(NETRECVINFO *info)
{
	info->valid = 0;
}

static void recvinfo_start(NETRECVINFO *info, NETADDR *addr, NETCONNECTION *conn, int cid)
{
	info->addr = *addr;
	info->conn = conn;
	info->client_id = cid;
	info->current_chunk = 0;
	info->valid = 1;
}

/* TODO: rename this function */
static int recvinfo_fetch_chunk(NETRECVINFO *info, NETCHUNK *chunk)
{
	NETCHUNKHEADER header;
	unsigned char *data = info->data.chunk_data;
	int i;
	
	while(1)
	{
		/* check for old data to unpack */
		if(!info->valid || info->current_chunk >= info->data.num_chunks)
		{
			recvinfo_clear(info);
			return 0;
		}
		
		/* TODO: add checking here so we don't read too far */
		for(i = 0; i < info->current_chunk; i++)
		{
			data = unpack_chunk_header(data, &header);
			data += header.size;
		}
		
		/* unpack the header */	
		data = unpack_chunk_header(data, &header);
		info->current_chunk++;
		
		/* handle sequence stuff */
		if(info->conn && (header.flags&NET_CHUNKFLAG_VITAL))
		{
			if(header.sequence == (info->conn->ack+1)%NET_MAX_SEQUENCE)
			{
				/* in sequence */
				info->conn->ack = (info->conn->ack+1)%NET_MAX_SEQUENCE;
			}
			else
			{
				/* out of sequence, request resend */
				dbg_msg("conn", "asking for resend %d %d", header.sequence, (info->conn->ack+1)%NET_MAX_SEQUENCE);
				conn_want_resend(info->conn);
				continue; /* take the next chunk in the packet */
			}
		}
		
		/* fill in the info */
		chunk->client_id = info->client_id;
		chunk->address = info->addr;
		chunk->flags = 0;
		chunk->data_size = header.size;
		chunk->data = data;
		return 1;
	}
}

int netserver_recv(NETSERVER *s, NETCHUNK *chunk)
{
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
						found = 0;
						
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
							/* TODO: send server full emssage */
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

NETCLIENT *netclient_open(NETADDR bindaddr, int flags)
{
	NETCLIENT *client = (NETCLIENT *)mem_alloc(sizeof(NETCLIENT), 1);
	mem_zero(client, sizeof(NETCLIENT));
	client->socket = net_udp_create(bindaddr);
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
	if(c->conn.state == NET_CONNSTATE_ERROR)
		netclient_disconnect(c, conn_error(&c->conn));
	return 0;
}

int netclient_disconnect(NETCLIENT *c, const char *reason)
{
	dbg_msg("netclient", "disconnected. reason=\"%s\"", reason);
	conn_disconnect(&c->conn, reason);
	return 0;
}

int netclient_connect(NETCLIENT *c, NETADDR *addr)
{
	conn_connect(&c->conn, addr);
	return 0;
}

int netclient_recv(NETCLIENT *c, NETCHUNK *chunk)
{
	while(1)
	{
		NETADDR addr;
		int bytes;
			
		/* check for a chunk */
		if(recvinfo_fetch_chunk(&c->recv, chunk))
			return 1;
		
		/* TODO: empty the recvinfo */
		bytes = net_udp_recv(c->socket, &addr, c->recv.buffer, NET_MAX_PACKETSIZE);

		/* no more packets for now */
		if(bytes <= 0)
			break;

		if(unpack_packet(c->recv.buffer, bytes, &c->recv.data) == 0)
		{
			if(c->recv.data.flags&NET_PACKETFLAG_CONNLESS)
			{
				chunk->flags = NETSENDFLAG_CONNLESS;
				chunk->client_id = -1;
				chunk->address = addr;
				chunk->data_size = c->recv.data.data_size;
				chunk->data = c->recv.data.chunk_data;
				return 1;
			}
			else
			{
				if(conn_feed(&c->conn, &c->recv.data, &addr))
					recvinfo_start(&c->recv, &addr, &c->conn, 0);
			}
		}
	}
	return 0;
}

int netclient_send(NETCLIENT *c, NETCHUNK *chunk)
{
	if(chunk->data_size >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netclient", "chunk payload too big. %d. dropping chunk", chunk->data_size);
		return -1;
	}
	
	if(chunk->flags&NETSENDFLAG_CONNLESS)
	{
		/* send connectionless packet */
		send_packet_connless(c->socket, &chunk->address, chunk->data, chunk->data_size);
	}
	else
	{
		int f = 0;
		dbg_assert(chunk->client_id == 0, "errornous client id");
		
		if(chunk->flags&NETSENDFLAG_VITAL)
			f = NET_CHUNKFLAG_VITAL;
		
		conn_queue_chunk(&c->conn, f, chunk->data_size, chunk->data);

		if(chunk->flags&NETSENDFLAG_FLUSH)
			conn_flush(&c->conn);
	}
	return 0;
}

int netclient_state(NETCLIENT *c)
{
	if(c->conn.state == NET_CONNSTATE_ONLINE)
		return NETSTATE_ONLINE;
	if(c->conn.state == NET_CONNSTATE_OFFLINE)
		return NETSTATE_OFFLINE;
	return NETSTATE_CONNECTING;
}

int netclient_flush(NETCLIENT *c)
{
	return conn_flush(&c->conn);
}

int netclient_gotproblems(NETCLIENT *c)
{
	if(time_get() - c->conn.last_recv_time > time_freq())
		return 1;
	return 0;
}

void netclient_stats(NETCLIENT *c, NETSTATS *stats)
{
	*stats = c->conn.stats;
}

const char *netclient_error_string(NETCLIENT *c)
{
	return conn_error(&c->conn);
}

void netcommon_openlog(const char *filename)
{
	datalog = io_open(filename, IOFLAG_WRITE);
}


static const unsigned freq_table[256+1] = {
	1<<30,4545,2657,431,1950,919,444,482,2244,617,838,542,715,1814,304,240,754,212,647,186,
	283,131,146,166,543,164,167,136,179,859,363,113,157,154,204,108,137,180,202,176,
	872,404,168,134,151,111,113,109,120,126,129,100,41,20,16,22,18,18,17,19,
	16,37,13,21,362,166,99,78,95,88,81,70,83,284,91,187,77,68,52,68,
	59,66,61,638,71,157,50,46,69,43,11,24,13,19,10,12,12,20,14,9,
	20,20,10,10,15,15,12,12,7,19,15,14,13,18,35,19,17,14,8,5,
	15,17,9,15,14,18,8,10,2173,134,157,68,188,60,170,60,194,62,175,71,
	148,67,167,78,211,67,156,69,1674,90,174,53,147,89,181,51,174,63,163,80,
	167,94,128,122,223,153,218,77,200,110,190,73,174,69,145,66,277,143,141,60,
	136,53,180,57,142,57,158,61,166,112,152,92,26,22,21,28,20,26,30,21,
	32,27,20,17,23,21,30,22,22,21,27,25,17,27,23,18,39,26,15,21,
	12,18,18,27,20,18,15,19,11,17,33,12,18,15,19,18,16,26,17,18,
	9,10,25,22,22,17,20,16,6,16,15,20,14,18,24,335,1517};

void netcommon_init()
{
	huffman_init(&huffmanstate, freq_table);
}
