#include <string.h>
#include <baselib/system.h>
#include <engine/network.h>

#include "mastersrv.h"

enum {
	MTU = 1400,
	EXPIRE_TIME = 90
};

static struct packet_data
{
	unsigned char header[sizeof(SERVERBROWSE_LIST)];
	NETADDR4 servers[MAX_SERVERS];
} data;
static int64 server_expire[MAX_SERVERS];
static int num_servers = 0;

static net_client net;

void add_server(NETADDR4 *info)
{
	// see if server already exists in list
	int i;
	for(i = 0; i < num_servers; i++)
	{
		if(net_addr4_cmp(&data.servers[i], info) == 0)
		{
			dbg_msg("mastersrv", "updated: %d.%d.%d.%d:%d",
				info->ip[0], info->ip[1], info->ip[2], info->ip[3], info->port);
			server_expire[i] = time_get()+time_freq()*EXPIRE_TIME;
			return;
		}
	}
	
	// add server
	if(num_servers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}
	
	dbg_msg("mastersrv", "added: %d.%d.%d.%d:%d",
		info->ip[0], info->ip[1], info->ip[2], info->ip[3], info->port);
	data.servers[num_servers] = *info;
	server_expire[num_servers] = time_get()+time_freq()*EXPIRE_TIME;
	num_servers++;
}

void purge_servers()
{
	int64 now = time_get();
	int i = 0;
	while(i < num_servers)
	{
		if(server_expire[i] < now)
		{
			// remove server
			dbg_msg("mastersrv", "expired: %d.%d.%d.%d:%d",
				data.servers[i].ip[0], data.servers[i].ip[1],
				data.servers[i].ip[2], data.servers[i].ip[3], data.servers[i].port);
			data.servers[i] = data.servers[num_servers-1];
			server_expire[i] = server_expire[num_servers-1];
			num_servers--;
		}
		else
			i++;
	}
}

int main(int argc, char **argv)
{
	net.open(MASTERSERVER_PORT, 0);
	// TODO: check socket for errors
	
	mem_copy(data.header, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));
	dbg_msg("mastersrv", "started");
	
	while(1)
	{
		net.update();
		
		// process packets
		NETPACKET packet;
		while(net.recv(&packet))
		{
			if(packet.data_size == sizeof(SERVERBROWSE_HEARTBEAT) &&
				memcmp(packet.data, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT)) == 0)
			{
				// add it
				add_server(&packet.address);
			}
			else if(packet.data_size == sizeof(SERVERBROWSE_GETLIST) &&
				memcmp(packet.data, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST)) == 0)
			{
				// someone requested the list
				dbg_msg("mastersrv", "requested, responding with %d servers", num_servers);
				NETPACKET p;
				p.client_id = -1;
				p.address = packet.address;
				p.flags = PACKETFLAG_CONNLESS;
				p.data_size = num_servers*sizeof(NETADDR4)+sizeof(SERVERBROWSE_LIST);
				p.data = &data;
				net.send(&p);
			}

		}
		
		// TODO: shouldn't be done every fuckin frame
		purge_servers();
		
		// be nice to the CPU
		thread_sleep(1);
	}
	
	return 0;
}
