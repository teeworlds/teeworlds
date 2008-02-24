/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_system.h>

enum { NUM_SOCKETS = 64 };

int run(NETADDR4 dest)
{
	NETSOCKET sockets[NUM_SOCKETS];
	int i;
	
	for(i = 0; i < NUM_SOCKETS; i++)
	{
		NETADDR4 bindaddr = {{0,0,0,0}, 0};
	 	sockets[i] = net_udp4_create(bindaddr);
	}
	
	while(1)
	{
		unsigned char data[1024];
		int size = 0;
		int socket_to_use = 0;
		io_read(io_stdin(), &size, 2);
		io_read(io_stdin(), &socket_to_use, 1);
		size %= 256;
		socket_to_use %= NUM_SOCKETS;
		io_read(io_stdin(), data, size);
		net_udp4_send(sockets[socket_to_use], &dest, data, size);
	}
}

int main(int argc, char **argv)
{
	NETADDR4 dest = {{127,0,0,1},8303};
	run(dest);
	return 0;
}
