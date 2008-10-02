#include <base/system.h>
#include "e_network.h"
#include "e_network_internal.h"

struct NETCLIENT
{
	NETADDR server_addr;
	NETSOCKET socket;
	
	NETRECVINFO recv;
	NETCONNECTION conn;
};

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


int netclient_disconnect(NETCLIENT *c, const char *reason)
{
	dbg_msg("netclient", "disconnected. reason=\"%s\"", reason);
	conn_disconnect(&c->conn, reason);
	return 0;
}

int netclient_update(NETCLIENT *c)
{
	conn_update(&c->conn);
	if(c->conn.state == NET_CONNSTATE_ERROR)
		netclient_disconnect(c, conn_error(&c->conn));
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
