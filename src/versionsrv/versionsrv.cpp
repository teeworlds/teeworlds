/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <base/system.h>

extern "C" {
	#include <engine/e_network.h>
}

#include "versionsrv.h"

static net_client net_op; // main

void send_ver(NETADDR *addr)
{
	NETCHUNK p;
	unsigned char data[sizeof(VERSIONSRV_VERSION) + sizeof(VERSION_DATA)];
	
	memcpy(data, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	memcpy(data + sizeof(VERSIONSRV_VERSION), VERSION_DATA, sizeof(VERSION_DATA));
	
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	p.data = data;
	p.data_size = sizeof(data);

	net_op.send(&p);
}

int main(int argc, char **argv)
{
	NETADDR bindaddr;

	dbg_logger_stdout();
	net_init();

	mem_zero(&bindaddr, sizeof(bindaddr));
	bindaddr.port = VERSIONSRV_PORT;
	net_op.open(bindaddr, 0);
	
	dbg_msg("versionsrv", "started");
	
	while(1)
	{
		net_op.update();
		
		// process packets
		NETCHUNK packet;
		while(net_op.recv(&packet))
		{
			if(packet.data_size == sizeof(VERSIONSRV_GETVERSION) &&
				memcmp(packet.data, VERSIONSRV_GETVERSION, sizeof(VERSIONSRV_GETVERSION)) == 0)
			{
				send_ver(&packet.address);
			}
		}
		
		// be nice to the CPU
		thread_sleep(1);
	}
	
	return 0;
}
