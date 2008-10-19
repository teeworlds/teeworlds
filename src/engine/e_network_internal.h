#include <base/system.h>
#include "e_network.h"
#include "e_ringbuffer.h"

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
	NET_MAX_CLIENTS = 16,
	NET_MAX_SEQUENCE = 1<<10,
	NET_SEQUENCE_MASK = NET_MAX_SEQUENCE-1,

	NET_CONNSTATE_OFFLINE=0,
	NET_CONNSTATE_CONNECT=1,
	NET_CONNSTATE_PENDING=2,
	NET_CONNSTATE_ONLINE=3,
	NET_CONNSTATE_ERROR=4,

	NET_PACKETFLAG_CONTROL=1,
	NET_PACKETFLAG_CONNLESS=2,
	NET_PACKETFLAG_RESEND=4,
	NET_PACKETFLAG_COMPRESSION=8,

	NET_CHUNKFLAG_VITAL=1,
	NET_CHUNKFLAG_RESEND=2,
	
	NET_CTRLMSG_KEEPALIVE=0,
	NET_CTRLMSG_CONNECT=1,
	NET_CTRLMSG_CONNECTACCEPT=2,
	NET_CTRLMSG_ACCEPT=3,
	NET_CTRLMSG_CLOSE=4,
	
	NET_SERVER_MAXBANS=1024,
	
	NET_CONN_BUFFERSIZE=1024*16,
	
	NET_ENUM_TERMINATOR
};


typedef struct NETPACKETCONSTRUCT
{
	int flags;
	int ack;
	int num_chunks;
	int data_size;
	unsigned char chunk_data[NET_MAX_PAYLOAD];
} NETPACKETCONSTRUCT;

typedef struct NETCHUNKHEADER
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
	int64 last_send_time;
	int64 first_send_time;
} NETCHUNKDATA;

typedef struct
{
	unsigned short seq;
	unsigned short ack;
	unsigned state;
	
	int token;
	int remote_closed;
	
	RINGBUFFER *buffer;
	
	int64 last_update_time;
	int64 last_recv_time;
	int64 last_send_time;
	
	char error_string[256];
	
	NETPACKETCONSTRUCT construct;
	
	NETADDR peeraddr;
	NETSOCKET socket;
	NETSTATS stats;

	char buffer_memory[NET_CONN_BUFFERSIZE];
} NETCONNECTION;

typedef struct NETRECVINFO
{
	NETADDR addr;
	NETCONNECTION *conn;
	int current_chunk;
	int client_id;
	int valid;
	NETPACKETCONSTRUCT data;
	unsigned char buffer[NET_MAX_PACKETSIZE];
} NETRECVINFO;


/* connection functions */
void conn_init(NETCONNECTION *conn, NETSOCKET socket);
int conn_connect(NETCONNECTION *conn, NETADDR *addr);
void conn_disconnect(NETCONNECTION *conn, const char *reason);
int conn_update(NETCONNECTION *conn);
int conn_feed(NETCONNECTION *conn, NETPACKETCONSTRUCT *packet, NETADDR *addr);
void conn_queue_chunk(NETCONNECTION *conn, int flags, int data_size, const void *data);
const char *conn_error(NETCONNECTION *conn);
void conn_want_resend(NETCONNECTION *conn);
int conn_flush(NETCONNECTION *conn);

/* recvinfo functions */
void recvinfo_clear(NETRECVINFO *info);
void recvinfo_start(NETRECVINFO *info, NETADDR *addr, NETCONNECTION *conn, int cid);
int recvinfo_fetch_chunk(NETRECVINFO *info, NETCHUNK *chunk);

/* misc helper functions */
void send_packet_connless(NETSOCKET socket, NETADDR *addr, const void *data, int data_size);
void send_packet(NETSOCKET socket, NETADDR *addr, NETPACKETCONSTRUCT *packet);
int unpack_packet(unsigned char *buffer, int size, NETPACKETCONSTRUCT *packet);
unsigned char *pack_chunk_header(unsigned char *data, int flags, int size, int sequence);
unsigned char *unpack_chunk_header(unsigned char *data, NETCHUNKHEADER *header);
