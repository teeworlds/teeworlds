/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction - Network
*/

#ifndef BASE_NETWORK_H
#define BASE_NETWORK_H

#include "detect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Group: Network General */
typedef struct
{
	int type;
	int ipv4sock;
	int ipv6sock;
} NETSOCKET;

enum
{
	NETADDR_MAXSTRSIZE = 1+(8*4+7)+1+1+5+1, // [XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX]:XXXXX

	NETTYPE_INVALID = 0,
	NETTYPE_IPV4 = 1,
	NETTYPE_IPV6 = 2,
	NETTYPE_LINK_BROADCAST = 4,
	NETTYPE_ALL = NETTYPE_IPV4|NETTYPE_IPV6
};

typedef struct
{
	unsigned int type;
	unsigned char ip[16];
	unsigned short port;
} NETADDR;

/*
	Function: net_init
		Initiates network functionallity.

	Returns:
		Returns 0 on success,

	Remarks:
		You must call this function before using any other network
		functions.
*/
int net_init();

/*
	Function: net_host_lookup
		Does a hostname lookup by name and fills out the passed
		NETADDR struct with the recieved details.

	Returns:
		0 on success.
*/
int net_host_lookup(const char *hostname, NETADDR *addr, int types);

/*
	Function: net_addr_comp
		Compares two network addresses.

	Parameters:
		a - Address to compare
		b - Address to compare to.

	Returns:
		<0 - Address a is lesser then address b
		0 - Address a is equal to address b
		>0 - Address a is greater then address b
*/
int net_addr_comp(const NETADDR *a, const NETADDR *b);

/*
	Function: net_addr_str
		Turns a network address into a representive string.

	Parameters:
		addr - Address to turn into a string.
		string - Buffer to fill with the string.
		max_length - Maximum size of the string.
		add_port - add port to string or not

	Remarks:
		- The string will always be zero terminated

*/
void net_addr_str(const NETADDR *addr, char *string, int max_length, int add_port);

/*
	Function: net_addr_from_str
		Turns string into a network address.

	Returns:
		0 on success

	Parameters:
		addr - Address to fill in.
		string - String to parse.
*/
int net_addr_from_str(NETADDR *addr, const char *string);

/* Group: Network UDP */

/*
	Function: net_udp_create
		Creates a UDP socket and binds it to a port.

	Parameters:
		bindaddr - Address to bind the socket to.
		use_random_port - use a random port

	Returns:
		On success it returns an handle to the socket. On failure it
		returns NETSOCKET_INVALID.
*/
NETSOCKET net_udp_create(NETADDR bindaddr, int use_random_port);

/*
	Function: net_udp_send
		Sends a packet over an UDP socket.

	Parameters:
		sock - Socket to use.
		addr - Where to send the packet.
		data - Pointer to the packet data to send.
		size - Size of the packet.

	Returns:
		On success it returns the number of bytes sent. Returns -1
		on error.
*/
int net_udp_send(NETSOCKET sock, const NETADDR *addr, const void *data, int size);

/*
	Function: net_udp_recv
		Recives a packet over an UDP socket.

	Parameters:
		sock - Socket to use.
		addr - Pointer to an NETADDR that will recive the address.
		data - Pointer to a buffer that will recive the data.
		maxsize - Maximum size to recive.

	Returns:
		On success it returns the number of bytes recived. Returns -1
		on error.
*/
int net_udp_recv(NETSOCKET sock, NETADDR *addr, void *data, int maxsize);

/*
	Function: net_udp_close
		Closes an UDP socket.

	Parameters:
		sock - Socket to close.

	Returns:
		Returns 0 on success. -1 on error.
*/
int net_udp_close(NETSOCKET sock);


/* Group: Network TCP */

/*
	Function: net_tcp_create
		Creates a TCP socket.

	Parameters:
		bindaddr - Address to bind the socket to.

	Returns:
		On success it returns an handle to the socket. On failure it returns NETSOCKET_INVALID.
*/
NETSOCKET net_tcp_create(NETADDR bindaddr);

/*
	Function: net_tcp_listen
		Makes the socket start listening for new connections.

	Parameters:
		sock - Socket to start listen to.
		backlog - Size of the queue of incomming connections to keep.

	Returns:
		Returns 0 on success.
*/
int net_tcp_listen(NETSOCKET sock, int backlog);

/*
	Function: net_tcp_accept
		Polls a listning socket for a new connection.

	Parameters:
		sock - Listning socket to poll.
		new_sock - Pointer to a socket to fill in with the new socket.
		addr - Pointer to an address that will be filled in the remote address (optional, can be NULL).

	Returns:
		Returns a non-negative integer on success. Negative integer on failure.
*/
int net_tcp_accept(NETSOCKET sock, NETSOCKET *new_sock, NETADDR *addr);

/*
	Function: net_tcp_connect
		Connects one socket to another.

	Parameters:
		sock - Socket to connect.
		addr - Address to connect to.

	Returns:
		Returns 0 on success.

*/
int net_tcp_connect(NETSOCKET sock, const NETADDR *addr);

/*
	Function: net_tcp_send
		Sends data to a TCP stream.

	Parameters:
		sock - Socket to send data to.
		data - Pointer to the data to send.
		size - Size of the data to send.

	Returns:
		Number of bytes sent. Negative value on failure.
*/
int net_tcp_send(NETSOCKET sock, const void *data, int size);

/*
	Function: net_tcp_recv
		Recvives data from a TCP stream.

	Parameters:
		sock - Socket to recvive data from.
		data - Pointer to a buffer to write the data to
		max_size - Maximum of data to write to the buffer.

	Returns:
		Number of bytes recvived. Negative value on failure. When in
		non-blocking mode, it returns 0 when there is no more data to
		be fetched.
*/
int net_tcp_recv(NETSOCKET sock, void *data, int maxsize);

/*
	Function: net_tcp_close
		Closes a TCP socket.

	Parameters:
		sock - Socket to close.

	Returns:
		Returns 0 on success. Negative value on failure.
*/
int net_tcp_close(NETSOCKET sock);

/*
	Group: Undocumented
*/


/*
	Function: net_tcp_connect_non_blocking

	DOCTODO: serp
*/
int net_tcp_connect_non_blocking(NETSOCKET sock, NETADDR bindaddr);

/*
	Function: net_set_non_blocking

	DOCTODO: serp
*/
int net_set_non_blocking(NETSOCKET sock);

/*
	Function: net_set_non_blocking

	DOCTODO: serp
*/
int net_set_blocking(NETSOCKET sock);

/*
	Function: net_errno

	DOCTODO: serp
*/
int net_errno();

/*
	Function: net_would_block

	DOCTODO: serp
*/
int net_would_block();

int net_socket_read_wait(NETSOCKET sock, int time);

void swap_endian(void *data, unsigned elem_size, unsigned num);

/*
	Function: bytes_be_to_uint
		Packs 4 big endian bytes into an unsigned

	Returns:
		The packed unsigned

	Remarks:
		- Assumes the passed array is 4 bytes
		- Assumes unsigned is 4 bytes
*/
unsigned bytes_be_to_uint(const unsigned char *bytes);

typedef struct
{
	int sent_packets;
	int sent_bytes;
	int recv_packets;
	int recv_bytes;
} NETSTATS;

void net_stats(NETSTATS *stats);

#ifdef __cplusplus
}
#endif

#endif
