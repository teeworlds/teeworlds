#include <base/system.h>
#include <string.h>
#include "e_config.h"
#include "e_network_internal.h"

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
	conn->last_send_time = 0;
	conn->last_recv_time = 0;
	conn->last_update_time = 0;
	conn->token = -1;
	mem_zero(&conn->peeraddr, sizeof(conn->peeraddr));
	
	conn->buffer = ringbuf_init(conn->buffer_memory, sizeof(conn->buffer_memory), 0);
	
	mem_zero(&conn->construct, sizeof(conn->construct));
}


const char *conn_error(NETCONNECTION *conn)
{
	return conn->error_string;
}

static void conn_set_error(NETCONNECTION *conn, const char *str)
{
	str_copy(conn->error_string, str, sizeof(conn->error_string));
}

void conn_init(NETCONNECTION *conn, NETSOCKET socket)
{
	conn_reset(conn);
	conn_reset_stats(conn);
	conn->socket = socket;
	mem_zero(conn->error_string, sizeof(conn->error_string));
}


static void conn_ack(NETCONNECTION *conn, int ack)
{
	
	while(1)
	{
		NETCHUNKDATA *resend = (NETCHUNKDATA *)ringbuf_first(conn->buffer);
		if(!resend)
			break;
		
		if(seq_in_backroom(resend->sequence, ack))
			ringbuf_popfirst(conn->buffer);
		else
			break;
	}
}

void conn_want_resend(NETCONNECTION *conn)
{
	conn->construct.flags |= NET_PACKETFLAG_RESEND;
}

int conn_flush(NETCONNECTION *conn)
{
	int num_chunks = conn->construct.num_chunks;
	if(!num_chunks && !conn->construct.flags)
		return 0;

	/* send of the packets */	
	conn->construct.ack = conn->ack;
	send_packet(conn->socket, &conn->peeraddr, &conn->construct);
	
	/* update send times */
	conn->last_send_time = time_get();
	
	/* clear construct so we can start building a new package */
	mem_zero(&conn->construct, sizeof(conn->construct));
	return num_chunks;
}

static void conn_queue_chunk_ex(NETCONNECTION *conn, int flags, int data_size, const void *data, int sequence)
{
	unsigned char *chunk_data;
	
	/* check if we have space for it, if not, flush the connection */
	if(conn->construct.data_size + data_size + NET_MAX_CHUNKHEADERSIZE > sizeof(conn->construct.chunk_data))
		conn_flush(conn);

	/* pack all the data */
	chunk_data = &conn->construct.chunk_data[conn->construct.data_size];
	chunk_data = pack_chunk_header(chunk_data, flags, data_size, sequence);
	mem_copy(chunk_data, data, data_size);
	chunk_data += data_size;

	/* */
	conn->construct.num_chunks++;
	conn->construct.data_size = (int)(chunk_data-conn->construct.chunk_data);
	
	/* set packet flags aswell */
	
	if(flags&NET_CHUNKFLAG_VITAL && !(flags&NET_CHUNKFLAG_RESEND))
	{
		/* save packet if we need to resend */
		NETCHUNKDATA *resend = (NETCHUNKDATA *)ringbuf_allocate(conn->buffer, sizeof(NETCHUNKDATA)+data_size);
		if(resend)
		{
			resend->sequence = sequence;
			resend->flags = flags;
			resend->data_size = data_size;
			resend->data = (unsigned char *)(resend+1);
			resend->first_send_time = time_get();
			resend->last_send_time = resend->first_send_time;
			mem_copy(resend->data, data, data_size);
		}
		else
		{
			/* out of buffer */
			conn_disconnect(conn, "too weak connection (out of buffer)");
		}
	}
}

void conn_queue_chunk(NETCONNECTION *conn, int flags, int data_size, const void *data)
{
	if(flags&NET_CHUNKFLAG_VITAL)
		conn->seq = (conn->seq+1)%NET_MAX_SEQUENCE;
	conn_queue_chunk_ex(conn, flags, data_size, data, conn->seq);
}


static void conn_send_control(NETCONNECTION *conn, int controlmsg, const void *extra, int extra_size)
{
	/* send the control message */
	send_controlmsg(conn->socket, &conn->peeraddr, conn->ack, controlmsg, extra, extra_size);
}

static void conn_resend_chunk(NETCONNECTION *conn, NETCHUNKDATA *resend)
{
	conn_queue_chunk_ex(conn, resend->flags|NET_CHUNKFLAG_RESEND, resend->data_size, resend->data, resend->sequence);
	resend->last_send_time = time_get();
}

static void conn_resend(NETCONNECTION *conn)
{
	int resend_count = 0;
	int first = 0, last = 0;
	void *item = ringbuf_first(conn->buffer);
	
	while(item)
	{
		NETCHUNKDATA *resend = item;
		
		if(resend_count == 0)
			first = resend->sequence;
		last = resend->sequence;
			
		conn_resend_chunk(conn, resend);
		item = ringbuf_next(conn->buffer, item);
		resend_count++;
	}
	
	if(config.debug)
		dbg_msg("conn", "resent %d packets (%d to %d)", resend_count, first, last);
}

int conn_connect(NETCONNECTION *conn, NETADDR *addr)
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

void conn_disconnect(NETCONNECTION *conn, const char *reason)
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

int conn_feed(NETCONNECTION *conn, NETPACKETCONSTRUCT *packet, NETADDR *addr)
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
					conn->state = NET_CONNSTATE_PENDING;
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
	else
	{
		if(conn->state == NET_CONNSTATE_PENDING)
		{
			conn->state = NET_CONNSTATE_ONLINE;
			if(config.debug)
				dbg_msg("connection", "connecting online");
		}
	}
	
	if(conn->state == NET_CONNSTATE_ONLINE)
	{
		
		conn_ack(conn, packet->ack);
	}
	
	return 1;
}

int conn_update(NETCONNECTION *conn)
{
	int64 now = time_get();

	if(conn->state == NET_CONNSTATE_OFFLINE || conn->state == NET_CONNSTATE_ERROR)
		return 0;
	
	/* check for timeout */
	if(conn->state != NET_CONNSTATE_OFFLINE &&
		conn->state != NET_CONNSTATE_CONNECT &&
		(now-conn->last_recv_time) > time_freq()*10)
	{
		conn->state = NET_CONNSTATE_ERROR;
		conn_set_error(conn, "timeout");
	}

	/* fix resends */
	if(ringbuf_first(conn->buffer))
	{
		NETCHUNKDATA *resend = (NETCHUNKDATA *)ringbuf_first(conn->buffer);

		/* check if we have some really old stuff laying around and abort if not acked */
		if(now-resend->first_send_time > time_freq()*10)
		{
			conn->state = NET_CONNSTATE_ERROR;
			conn_set_error(conn, "too weak connection (not acked for 10 seconds)");
		}
		else
		{
			/* resend packet if we havn't got it acked in 1 second */
			if(now-resend->last_send_time > time_freq())
				conn_resend_chunk(conn, resend);
		}
	}
	
	/* send keep alives if nothing has happend for 250ms */
	if(conn->state == NET_CONNSTATE_ONLINE)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) /* flush connection after 500ms if needed */
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
	}
	else if(conn->state == NET_CONNSTATE_PENDING)
	{
		if(time_get()-conn->last_send_time > time_freq()/2) /* send a new connect/accept every 500ms */
			conn_send_control(conn, NET_CTRLMSG_CONNECTACCEPT, 0, 0);
	}
	
	return 0;
}
