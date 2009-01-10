#ifndef ENGINE_NETWORK_H
#define ENGINE_NETWORK_H
/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */


typedef struct
{
	/* -1 means that it's a stateless packet */
	/* 0 on the client means the server */
	int client_id;
	NETADDR address; /* only used when client_id == -1 */
	int flags;
	int data_size;
	const void *data;
} NETCHUNK;


typedef struct
{
	NETADDR addr;
	int expires;
} NETBANINFO;

/*typedef struct
{
	int send_bytes;
	int recv_bytes;
	int send_packets;
	int recv_packets;
	
	int resend_packets;
	int resend_bytes;
} NETSTATS;*/

typedef struct NETSERVER NETSERVER;
typedef struct NETCLIENT NETCLIENT;

enum
{
	NETFLAG_ALLOWSTATELESS=1,
	NETSENDFLAG_VITAL=1,
	NETSENDFLAG_CONNLESS=2,
	NETSENDFLAG_FLUSH=4,
	
	NETSTATE_OFFLINE=0,
	NETSTATE_CONNECTING,
	NETSTATE_ONLINE,
	
	NETBANTYPE_SOFT=1,
	NETBANTYPE_DROP=2
};

typedef int (*NETFUNC_DELCLIENT)(int cid, void *user);
typedef int (*NETFUNC_NEWCLIENT)(int cid, void *user);

/* both */
void netcommon_openlog(const char *sentlog, const char *recvlog);
void netcommon_init();
int netcommon_compress(const void *data, int data_size, void *output, int output_size);
int netcommon_decompress(const void *data, int data_size, void *output, int output_size);

/* server side */
NETSERVER *netserver_open(NETADDR bindaddr, int max_clients, int flags);
int netserver_set_callbacks(NETSERVER *s, NETFUNC_NEWCLIENT new_client, NETFUNC_DELCLIENT del_client, void *user);
int netserver_recv(NETSERVER *s, NETCHUNK *chunk);
int netserver_send(NETSERVER *s, NETCHUNK *chunk);
int netserver_close(NETSERVER *s);
int netserver_update(NETSERVER *s);
NETSOCKET netserver_socket(NETSERVER *s);
int netserver_drop(NETSERVER *s, int client_id, const char *reason);
int netserver_client_addr(NETSERVER *s, int client_id, NETADDR *addr);
int netserver_max_clients(NETSERVER *s);

int netserver_ban_add(NETSERVER *s, NETADDR addr, int seconds);
int netserver_ban_remove(NETSERVER *s, NETADDR addr);
int netserver_ban_num(NETSERVER *s); /* caution, slow */
int netserver_ban_get(NETSERVER *s, int index, NETBANINFO *info); /* caution, slow */

/*void netserver_stats(NETSERVER *s, NETSTATS *stats);*/

/* client side */
NETCLIENT *netclient_open(NETADDR bindaddr, int flags);
int netclient_disconnect(NETCLIENT *c, const char *reason);
int netclient_connect(NETCLIENT *c, NETADDR *addr);
int netclient_recv(NETCLIENT *c, NETCHUNK *chunk);
int netclient_send(NETCLIENT *c, NETCHUNK *chunk);
int netclient_close(NETCLIENT *c);
int netclient_update(NETCLIENT *c);
int netclient_state(NETCLIENT *c);
int netclient_flush(NETCLIENT *c);
int netclient_gotproblems(NETCLIENT *c);
/*void netclient_stats(NETCLIENT *c, NETSTATS *stats);*/
int netclient_error_string_reset(NETCLIENT *c);
const char *netclient_error_string(NETCLIENT *c);

#ifdef __cplusplus
class net_server
{
	NETSERVER *ptr;
public:
	net_server() : ptr(0) {}
	~net_server() { close(); }
	
	int open(NETADDR bindaddr, int max, int flags) { ptr = netserver_open(bindaddr, max, flags); return ptr != 0; }
	int close() { int r = netserver_close(ptr); ptr = 0; return r; }
	
	int set_callbacks(NETFUNC_NEWCLIENT new_client, NETFUNC_DELCLIENT del_client, void *user)
	{ return netserver_set_callbacks(ptr, new_client, del_client, user); }
	
	int recv(NETCHUNK *chunk) { return netserver_recv(ptr, chunk); }
	int send(NETCHUNK *chunk) { return netserver_send(ptr, chunk); }
	int update() { return netserver_update(ptr); }
	
	int drop(int client_id, const char *reason) { return netserver_drop(ptr, client_id, reason); } 

	int max_clients() { return netserver_max_clients(ptr); }
	/*void stats(NETSTATS *stats) { netserver_stats(ptr, stats); }*/
};


class net_client
{
	NETCLIENT *ptr;
public:
	net_client() : ptr(0) {}
	~net_client() { close(); }
	
	int open(NETADDR bindaddr, int flags) { ptr = netclient_open(bindaddr, flags); return ptr != 0; }
	int close() { int r = netclient_close(ptr); ptr = 0; return r; }
	
	int connect(NETADDR *addr) { return netclient_connect(ptr, addr); }
	int disconnect(const char *reason) { return netclient_disconnect(ptr, reason); }
	
	int recv(NETCHUNK *chunk) { return netclient_recv(ptr, chunk); }
	int send(NETCHUNK *chunk) { return netclient_send(ptr, chunk); }
	int update() { return netclient_update(ptr); }
	
	const char *error_string() { return netclient_error_string(ptr); }
	
	int state() { return netclient_state(ptr); }
	/*void stats(NETSTATS *stats) { netclient_stats(ptr, stats); }*/
};
#endif


#endif
