/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

enum { NUM_SOCKETS = 64 };

void Run(NETADDR Dest)
{
	NETSOCKET aSockets[NUM_SOCKETS];

	for(int i = 0; i < NUM_SOCKETS; i++)
	{
		NETADDR BindAddr = {NETTYPE_IPV4, {0}, 0};
	 	aSockets[i] = net_udp_create(BindAddr);
	}

	while(1)
	{
		unsigned char aData[1024];
		int Size = 0;
		int SocketToUse = 0;
		io_read(io_stdin(), &Size, 2);
		io_read(io_stdin(), &SocketToUse, 1);
		Size %= 256;
		SocketToUse %= NUM_SOCKETS;
		io_read(io_stdin(), aData, Size);
		net_udp_send(aSockets[SocketToUse], &Dest, aData, Size);
	}
}

int main(int argc, char **argv)
{
	NETADDR Dest = {NETTYPE_IPV4, {127,0,0,1}, 8303};
	Run(Dest);
	return 0;
}
