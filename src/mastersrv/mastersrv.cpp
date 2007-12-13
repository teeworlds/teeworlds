/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>

extern "C" {
	#include <engine/system.h>
	#include <engine/network.h>
}

#include "mastersrv.h"

enum {
	MTU = 1400,
	EXPIRE_TIME = 90
};

static struct check_server
{
	NETADDR4 address;
	NETADDR4 alt_address;
	int try_count;
	int64 try_time;
} check_servers[MAX_SERVERS];
static int num_checkservers = 0;

static struct packet_data
{
	unsigned char header[sizeof(SERVERBROWSE_LIST)];
	NETADDR4 servers[MAX_SERVERS];
} data;
static int64 server_expire[MAX_SERVERS];
static int num_servers = 0;

static net_client net_checker; // NAT/FW checker
static net_client net_op; // main

void send_ok(NETADDR4 *addr)
{
	NETPACKET p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = PACKETFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWOK);
	p.data = SERVERBROWSE_FWOK;
	net_op.send(&p);
}

void send_error(NETADDR4 *addr)
{
	NETPACKET p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = PACKETFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWERROR);
	p.data = SERVERBROWSE_FWERROR;
	net_op.send(&p);
}

void send_check(NETADDR4 *addr)
{
	NETPACKET p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = PACKETFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWCHECK);
	p.data = SERVERBROWSE_FWCHECK;
	net_checker.send(&p);
}

void add_checkserver(NETADDR4 *info, NETADDR4 *alt)
{
	// add server
	if(num_checkservers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}
	
	dbg_msg("mastersrv", "checking: %d.%d.%d.%d:%d (%d.%d.%d.%d:%d)",
		info->ip[0], info->ip[1], info->ip[2], info->ip[3], info->port,
		alt->ip[0], alt->ip[1], alt->ip[2], alt->ip[3], alt->port);
	check_servers[num_checkservers].address = *info;
	check_servers[num_checkservers].alt_address = *alt;
	check_servers[num_checkservers].try_count = 0;
	check_servers[num_checkservers].try_time = 0;
	num_checkservers++;
}

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

void update_servers()
{
	int64 now = time_get();
	int64 freq = time_freq();
	for(int i = 0; i < num_checkservers; i++)
	{
		if(now > check_servers[i].try_time+freq)
		{
			if(check_servers[i].try_count == 10)
			{
				dbg_msg("mastersrv", "check failed: %d.%d.%d.%d:%d",
					check_servers[i].address.ip[0], check_servers[i].address.ip[1],
					check_servers[i].address.ip[2], check_servers[i].address.ip[3],check_servers[i].address.port,
					check_servers[i].alt_address.ip[0], check_servers[i].alt_address.ip[1],
					check_servers[i].alt_address.ip[2], check_servers[i].alt_address.ip[3],check_servers[i].alt_address.port);
					
				// FAIL!!
				send_error(&check_servers[i].address);
				check_servers[i] = check_servers[num_checkservers-1];
				num_checkservers--;
				i--;
			}
			else
			{
				check_servers[i].try_count++;
				check_servers[i].try_time = now;
				if(check_servers[i].try_count&1)
					send_check(&check_servers[i].address);
				else
					send_check(&check_servers[i].alt_address);
			}
		}
	}
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
	NETADDR4 bindaddr;
	mem_zero(&bindaddr, sizeof(bindaddr));
	bindaddr.port = MASTERSERVER_PORT;
	
	net_op.open(bindaddr, 0);

	bindaddr.port = MASTERSERVER_PORT+1;
	net_checker.open(bindaddr, 0);
	// TODO: check socket for errors
	
	mem_copy(data.header, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));
	dbg_msg("mastersrv", "started");
	
	while(1)
	{
		net_op.update();
		net_checker.update();
		
		// process packets
		NETPACKET packet;
		while(net_op.recv(&packet))
		{
			if(packet.data_size == sizeof(SERVERBROWSE_HEARTBEAT)+2 &&
				memcmp(packet.data, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT)) == 0)
			{
				NETADDR4 alt;
				unsigned char *d = (unsigned char *)packet.data;
				alt = packet.address;
				alt.port =
					(d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8) |
					d[sizeof(SERVERBROWSE_HEARTBEAT)+1];
				
				// add it
				add_checkserver(&packet.address, &alt);
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
				net_op.send(&p);
			}
		}

		// process packets
		while(net_checker.recv(&packet))
		{
			if(packet.data_size == sizeof(SERVERBROWSE_FWRESPONSE) &&
				memcmp(packet.data, SERVERBROWSE_FWRESPONSE, sizeof(SERVERBROWSE_FWRESPONSE)) == 0)
			{
				// remove it from checking
				for(int i = 0; i < num_checkservers; i++)
				{
					if(net_addr4_cmp(&check_servers[i].address, &packet.address) == 0)
					{
						num_checkservers--;
						check_servers[i] = check_servers[num_checkservers];
						break;
					}
				}
				
				add_server(&packet.address);
				send_ok(&packet.address);
			}
		}
		
		// TODO: shouldn't be done every fuckin frame
		purge_servers();
		update_servers();
		
		// be nice to the CPU
		thread_sleep(1);
	}
	
	return 0;
}
