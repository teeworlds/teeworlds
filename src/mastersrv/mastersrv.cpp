/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <base/system.h>

extern "C" {
	#include <engine/e_network.h>
}

#include "mastersrv.h"

enum {
	MTU = 1400,
	MAX_SERVERS_PER_PACKET=128,
	MAX_PACKETS=16,
	MAX_SERVERS=MAX_SERVERS_PER_PACKET*MAX_PACKETS,
	EXPIRE_TIME = 90
};

static struct CHECK_SERVER
{
	NETADDR address;
	NETADDR alt_address;
	int try_count;
	int64 try_time;
} check_servers[MAX_SERVERS];
static int num_checkservers = 0;


typedef struct NETADDR_IPv4
{
	unsigned char ip[4];
	unsigned short port;
} NETADDR_IPv4;

static struct SERVER_ENTRY
{
	NETADDR address;
	int64 expire;
} servers[MAX_SERVERS];
static int num_servers = 0;

static struct PACKET_DATA
{
	int size;
	struct {
		unsigned char header[sizeof(SERVERBROWSE_LIST)];
		NETADDR_IPv4 servers[MAX_SERVERS_PER_PACKET];
	} data;
} packets[MAX_PACKETS];
static int num_packets = 0;

static struct COUNT_PACKET_DATA
{
	unsigned char header[sizeof(SERVERBROWSE_COUNT)];
	unsigned char high;
	unsigned char low;
} count_data;

//static int64 server_expire[MAX_SERVERS];

static net_client net_checker; // NAT/FW checker
static net_client net_op; // main

void build_packets()
{
	SERVER_ENTRY *current = &servers[0];
	int servers_left = num_servers;
	int i;
	num_packets = 0;
	while(servers_left && num_packets < MAX_PACKETS)
	{
		int chunk = servers_left;
		if(chunk > MAX_SERVERS_PER_PACKET)
			chunk = MAX_SERVERS_PER_PACKET;
		servers_left -= chunk;
		
		// copy header	
		mem_copy(packets[num_packets].data.header, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));
		
		// copy server addresses
		for(i = 0; i < chunk; i++)
		{
			// TODO: ipv6 support
			packets[num_packets].data.servers[i].ip[0] = current->address.ip[0];
			packets[num_packets].data.servers[i].ip[1] = current->address.ip[1];
			packets[num_packets].data.servers[i].ip[2] = current->address.ip[2];
			packets[num_packets].data.servers[i].ip[3] = current->address.ip[3];
			packets[num_packets].data.servers[i].port = current->address.port;
			current++;
		}
		
		packets[num_packets].size = sizeof(SERVERBROWSE_LIST) + sizeof(NETADDR_IPv4)*chunk;
		
		num_packets++;
	}
}

void send_ok(NETADDR *addr)
{
	NETCHUNK p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWOK);
	p.data = SERVERBROWSE_FWOK;
	
	// send on both to be sure
	net_checker.send(&p);
	net_op.send(&p);
}

void send_error(NETADDR *addr)
{
	NETCHUNK p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWERROR);
	p.data = SERVERBROWSE_FWERROR;
	net_op.send(&p);
}

void send_check(NETADDR *addr)
{
	NETCHUNK p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWCHECK);
	p.data = SERVERBROWSE_FWCHECK;
	net_checker.send(&p);
}

void add_checkserver(NETADDR *info, NETADDR *alt)
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

void add_server(NETADDR *info)
{
	// see if server already exists in list
	int i;
	for(i = 0; i < num_servers; i++)
	{
		if(net_addr_comp(&servers[i].address, info) == 0)
		{
			dbg_msg("mastersrv", "updated: %d.%d.%d.%d:%d",
				info->ip[0], info->ip[1], info->ip[2], info->ip[3], info->port);
			servers[i].expire = time_get()+time_freq()*EXPIRE_TIME;
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
	servers[num_servers].address = *info;
	servers[num_servers].expire = time_get()+time_freq()*EXPIRE_TIME;
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
		if(servers[i].expire < now)
		{
			// remove server
			dbg_msg("mastersrv", "expired: %d.%d.%d.%d:%d",
				servers[i].address.ip[0], servers[i].address.ip[1],
				servers[i].address.ip[2], servers[i].address.ip[3], servers[i].address.port);
			servers[i] = servers[num_servers-1];
			num_servers--;
		}
		else
			i++;
	}
}

int main(int argc, char **argv)
{
	int64 last_build = 0;
	NETADDR bindaddr;

	dbg_logger_stdout();
	net_init();

	mem_zero(&bindaddr, sizeof(bindaddr));
	bindaddr.port = MASTERSERVER_PORT;
	
	
	net_op.open(bindaddr, 0);

	bindaddr.port = MASTERSERVER_PORT+1;
	net_checker.open(bindaddr, 0);
	// TODO: check socket for errors
	
	//mem_copy(data.header, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));
	mem_copy(count_data.header, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT));
	
	dbg_msg("mastersrv", "started");
	
	while(1)
	{
		net_op.update();
		net_checker.update();
		
		// process packets
		NETCHUNK packet;
		while(net_op.recv(&packet))
		{
			if(packet.data_size == sizeof(SERVERBROWSE_HEARTBEAT)+2 &&
				memcmp(packet.data, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT)) == 0)
			{
				NETADDR alt;
				unsigned char *d = (unsigned char *)packet.data;
				alt = packet.address;
				alt.port =
					(d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8) |
					d[sizeof(SERVERBROWSE_HEARTBEAT)+1];
				
				// add it
				add_checkserver(&packet.address, &alt);
			}
			else if(packet.data_size == sizeof(SERVERBROWSE_GETCOUNT) &&
				memcmp(packet.data, SERVERBROWSE_GETCOUNT, sizeof(SERVERBROWSE_GETCOUNT)) == 0)
			{
				dbg_msg("mastersrv", "count requested, responding with %d", num_servers);
				
				NETCHUNK p;
				p.client_id = -1;
				p.address = packet.address;
				p.flags = NETSENDFLAG_CONNLESS;
				p.data_size = sizeof(count_data);
				p.data = &count_data;
				count_data.high = (num_servers>>8)&0xff;
				count_data.low = num_servers&0xff;
				net_op.send(&p);
			}
			else if(packet.data_size == sizeof(SERVERBROWSE_GETLIST) &&
				memcmp(packet.data, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST)) == 0)
			{
				// someone requested the list
				dbg_msg("mastersrv", "requested, responding with %d servers", num_servers);
				NETCHUNK p;
				p.client_id = -1;
				p.address = packet.address;
				p.flags = NETSENDFLAG_CONNLESS;
				
				for(int i = 0; i < num_packets; i++)
				{
					p.data_size = packets[i].size;
					p.data = &packets[i].data;
					net_op.send(&p);
				}
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
					if(net_addr_comp(&check_servers[i].address, &packet.address) == 0 ||
						net_addr_comp(&check_servers[i].alt_address, &packet.address) == 0)
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
		
		if(time_get()-last_build > time_freq()*5)
		{
			last_build = time_get();
			
			purge_servers();
			update_servers();
			build_packets();
		}
		
		// be nice to the CPU
		thread_sleep(1);
	}
	
	return 0;
}
