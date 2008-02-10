/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

typedef struct
{
	/* -1 means that it's a stateless packet */
	/* 0 on the client means the server */
	int client_id;
	NETADDR4 address; /* only used when client_id == -1 */
	int flags;
	int data_size;
	const void *data;
} NETPACKET;

typedef struct
{
	int send_bytes;
	int recv_bytes;
	int send_packets;
	int recv_packets;
	
	int resend_packets;
	int resend_bytes;
} NETSTATS;

typedef struct NETSERVER_t NETSERVER;
typedef struct NETCLIENT_t NETCLIENT;

enum
{
	NETFLAG_ALLOWSTATELESS=1,
	PACKETFLAG_VITAL=1,
	PACKETFLAG_CONNLESS=2,
	
	NETSTATE_OFFLINE=0,
	NETSTATE_CONNECTING,
	NETSTATE_ONLINE
};

typedef int (*NETFUNC_DELCLIENT)(int cid, void *user);
typedef int (*NETFUNC_NEWCLIENT)(int cid, void *user);

/* server side */
NETSERVER *netserver_open(NETADDR4 bindaddr, int max_clients, int flags);
int netserver_set_callbacks(NETSERVER *s, NETFUNC_NEWCLIENT new_client, NETFUNC_DELCLIENT del_client, void *user);
int netserver_recv(NETSERVER *s, NETPACKET *packet);
int netserver_send(NETSERVER *s, NETPACKET *packet);
int netserver_close(NETSERVER *s);
int netserver_update(NETSERVER *s);
NETSOCKET netserver_socket(NETSERVER *s);
int netserver_drop(NETSERVER *s, int client_id, const char *reason);
int netserver_client_addr(NETSERVER *s, int client_id, NETADDR4 *addr);
int netserver_max_clients(NETSERVER *s);
void netserver_stats(NETSERVER *s, NETSTATS *stats);

/* client side */
NETCLIENT *netclient_open(NETADDR4 bindaddr, int flags);
int netclient_disconnect(NETCLIENT *c, const char *reason);
int netclient_connect(NETCLIENT *c, NETADDR4 *addr);
int netclient_recv(NETCLIENT *c, NETPACKET *packet);
int netclient_send(NETCLIENT *c, NETPACKET *packet);
int netclient_close(NETCLIENT *c);
int netclient_update(NETCLIENT *c);
int netclient_state(NETCLIENT *c);
int netclient_gotproblems(NETCLIENT *c);
void netclient_stats(NETCLIENT *c, NETSTATS *stats);
const char *netclient_error_string(NETCLIENT *c);

#ifdef __cplusplus
class net_server
{
	NETSERVER *ptr;
public:
	net_server() : ptr(0) {}
	~net_server() { close(); }
	
	int open(NETADDR4 bindaddr, int max, int flags) { ptr = netserver_open(bindaddr, max, flags); return ptr != 0; }
	int close() { int r = netserver_close(ptr); ptr = 0; return r; }
	
	int set_callbacks(NETFUNC_NEWCLIENT new_client, NETFUNC_DELCLIENT del_client, void *user)
	{ return netserver_set_callbacks(ptr, new_client, del_client, user); }
	
	int recv(NETPACKET *packet) { return netserver_recv(ptr, packet); }
	int send(NETPACKET *packet) { return netserver_send(ptr, packet); }
	int update() { return netserver_update(ptr); }
	
	int drop(int client_id, const char *reason) { return netserver_drop(ptr, client_id, reason); } 

	int max_clients() { return netserver_max_clients(ptr); }
	void stats(NETSTATS *stats) { netserver_stats(ptr, stats); }
};


class net_client
{
	NETCLIENT *ptr;
public:
	net_client() : ptr(0) {}
	~net_client() { close(); }
	
	int open(NETADDR4 bindaddr, int flags) { ptr = netclient_open(bindaddr, flags); return ptr != 0; }
	int close() { int r = netclient_close(ptr); ptr = 0; return r; }
	
	int connect(NETADDR4 *addr) { return netclient_connect(ptr, addr); }
	int disconnect(const char *reason) { return netclient_disconnect(ptr, reason); }
	
	int recv(NETPACKET *packet) { return netclient_recv(ptr, packet); }
	int send(NETPACKET *packet) { return netclient_send(ptr, packet); }
	int update() { return netclient_update(ptr); }
	
	const char *error_string() { return netclient_error_string(ptr); }
	
	int state() { return netclient_state(ptr); }
	void stats(NETSTATS *stats) { netclient_stats(ptr, stats); }
};
#endif
