
struct NETPACKET
{
	// -1 means that it's a stateless packet
	// 0 on the client means the server
	int client_id;
	NETADDR4 address; // only used when client_id == -1
	int flags;
	int data_size;
	const void *data;
};

struct NETSTATS
{
	int send_bytes;
	int recv_bytes;
	int send_packets;
	int recv_packets;
	
	int resend_packets;
	int resend_bytes;
};

struct NETSERVER;
struct NETCLIENT;

enum
{
	NETFLAG_ALLOWSTATELESS=1,
	PACKETFLAG_VITAL=1,
	
	NETSTATE_OFFLINE=0,
	NETSTATE_CONNECTING,
	NETSTATE_ONLINE,
};

// server side
NETSERVER *net_server_open(int port, int max_clients, int flags);
int net_server_recv(NETSERVER *s, NETPACKET *packet);
int net_server_send(NETSERVER *s, NETPACKET *packet);
int net_server_close(NETSERVER *s);
int net_server_update(NETSERVER *s);
int net_server_drop(NETSERVER *s, int client_id, const char *reason);
int net_server_newclient(NETSERVER *s); // -1 when no more, else, client id
int net_server_delclient(NETSERVER *s); // -1 when no more, else, client id
void net_server_stats(NETSERVER *s, NETSTATS *stats);

// client side
NETCLIENT *net_client_open(int flags);
int net_client_disconnect(NETCLIENT *c, const char *reason);
int net_client_connect(NETCLIENT *c, NETADDR4 *addr);
int net_client_recv(NETCLIENT *c, NETPACKET *packet);
int net_client_send(NETCLIENT *c, NETPACKET *packet);
int net_client_close(NETCLIENT *c);
int net_client_update(NETCLIENT *c);
int net_client_state(NETCLIENT *c);
void net_client_stats(NETCLIENT *c, NETSTATS *stats);


// wrapper classes for c++
#ifdef __cplusplus
class net_server
{
	NETSERVER *ptr;
public:
	net_server() : ptr(0) {}
	~net_server() { close(); }
	
	int open(int port, int max, int flags) { ptr = net_server_open(port, max, flags); return ptr != 0; }
	int close() { int r = net_server_close(ptr); ptr = 0; return r; }
	
	int recv(NETPACKET *packet) { return net_server_recv(ptr, packet); }
	int send(NETPACKET *packet) { return net_server_send(ptr, packet); }
	int update() { return net_server_update(ptr); }
	
	int drop(int client_id, const char *reason) { return net_server_drop(ptr, client_id, reason); } 
	int newclient() { return net_server_newclient(ptr); }
	int delclient() { return net_server_delclient(ptr); }
	
	void stats(NETSTATS *stats) { net_server_stats(ptr, stats); }
};


class net_client
{
	NETCLIENT *ptr;
public:
	net_client() : ptr(0) {}
	~net_client() { close(); }
	
	int open(int flags) { ptr = net_client_open(flags); return ptr != 0; }
	int close() { int r = net_client_close(ptr); ptr = 0; return r; }
	
	int connect(NETADDR4 *addr) { return net_client_connect(ptr, addr); }
	int disconnect(const char *reason) { return net_client_disconnect(ptr, reason); }
	
	int recv(NETPACKET *packet) { return net_client_recv(ptr, packet); }
	int send(NETPACKET *packet) { return net_client_send(ptr, packet); }
	int update() { return net_client_update(ptr); }
	
	int state() { return net_client_state(ptr); }
	void stats(NETSTATS *stats) { net_client_stats(ptr, stats); }
};
#endif
