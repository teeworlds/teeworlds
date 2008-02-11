#include <string.h>
#include <engine/e_system.h>
#include <engine/e_network.h>
#include <engine/e_config.h>
#include <engine/e_engine.h>

#include <mastersrv/mastersrv.h>

extern NETSERVER *net;

enum
{
	REGISTERSTATE_START=0,
	REGISTERSTATE_UPDATE_ADDRS,
	REGISTERSTATE_QUERY_COUNT,
	REGISTERSTATE_HEARTBEAT,
	REGISTERSTATE_REGISTERED,
	REGISTERSTATE_ERROR
};

static int register_state = REGISTERSTATE_START;
static int64 register_state_start = 0;
static int register_first = 1;

static void register_new_state(int state)
{
	register_state = state;
	register_state_start = time_get();
}

static void register_send_fwcheckresponse(NETADDR4 *addr)
{
	NETPACKET packet;
	packet.client_id = -1;
	packet.address = *addr;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_FWRESPONSE);
	packet.data = SERVERBROWSE_FWRESPONSE;
	netserver_send(net, &packet);
}
	
static void register_send_heartbeat(NETADDR4 addr)
{
	static unsigned char data[sizeof(SERVERBROWSE_HEARTBEAT) + 2];
	unsigned short port = config.sv_port;
	NETPACKET packet;
	
	mem_copy(data, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT));
	
	packet.client_id = -1;
	packet.address = addr;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_HEARTBEAT) + 2;
	packet.data = &data;

	/* supply the set port that the master can use if it has problems */	
	if(config.sv_external_port)
		port = config.sv_external_port;
	data[sizeof(SERVERBROWSE_HEARTBEAT)] = port >> 8;
	data[sizeof(SERVERBROWSE_HEARTBEAT)+1] = port&0xff;
	netserver_send(net, &packet);
}

static void register_send_count_request(NETADDR4 addr)
{
	NETPACKET packet;
	packet.client_id = -1;
	packet.address = addr;
	packet.flags = PACKETFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_GETCOUNT);
	packet.data = SERVERBROWSE_GETCOUNT;
	netserver_send(net, &packet);
}

typedef struct
{
	NETADDR4 addr;
	int count;
	int valid;
	int64 last_send;
} MASTERSERVER_INFO;

static MASTERSERVER_INFO masterserver_info[MAX_MASTERSERVERS] = {{{{0}}}};
static int register_registered_server = -1;

void register_update()
{
	int64 now = time_get();
	int64 freq = time_freq();
	
	if(!config.sv_sendheartbeats)
		return;
	
	mastersrv_update();
	
	if(register_state == REGISTERSTATE_START)
	{
		register_first = 1;
		register_new_state(REGISTERSTATE_UPDATE_ADDRS);
		mastersrv_refresh_addresses();
		dbg_msg("register", "refreshing ip addresses");
	}
	else if(register_state == REGISTERSTATE_UPDATE_ADDRS)
	{
		register_registered_server = -1;
		
		if(!mastersrv_refreshing())
		{
			int i;
			for(i = 0; i < MAX_MASTERSERVERS; i++)
			{
				NETADDR4 addr = mastersrv_get(i);
				masterserver_info[i].addr = addr;
				masterserver_info[i].count = 0;
				
				if(!addr.ip[0] && !addr.ip[1] && !addr.ip[2] && !addr.ip[3])
					masterserver_info[i].valid = 0;
				else
				{
					masterserver_info[i].valid = 1;
					masterserver_info[i].count = -1;
					masterserver_info[i].last_send = 0;
				}
			}
			
			dbg_msg("register", "fetching server counts");
			register_new_state(REGISTERSTATE_QUERY_COUNT);
		}
	}
	else if(register_state == REGISTERSTATE_QUERY_COUNT)
	{
		int i;
		int left = 0;
		for(i = 0; i < MAX_MASTERSERVERS; i++)
		{
			if(!masterserver_info[i].valid)
				continue;
				
			if(masterserver_info[i].count == -1)
			{
				left++;
				if(masterserver_info[i].last_send+freq < now)
				{
					masterserver_info[i].last_send = now;
					register_send_count_request(masterserver_info[i].addr);
				}
			}
		}
		
		/* check if we are done or timed out */
		if(left == 0 || now > register_state_start+freq*3)
		{
			/* choose server */
			int best = -1;
			int i;
			for(i = 0; i < MAX_MASTERSERVERS; i++)
			{
				if(!masterserver_info[i].valid || masterserver_info[i].count == -1)
					continue;
					
				if(best == -1 || masterserver_info[i].count < masterserver_info[best].count)
					best = i;
			}

			/* server chosen */
			register_registered_server = best;
			if(register_registered_server == -1)
			{
				dbg_msg("register", "WARNING: No master servers. Retrying in 60 seconds");
				register_new_state(REGISTERSTATE_ERROR);
			}
			else
			{			
				dbg_msg("register", "choosen '%s' as master, sending heartbeats", mastersrv_name(register_registered_server));
				masterserver_info[register_registered_server].last_send = 0;
				register_new_state(REGISTERSTATE_HEARTBEAT);
			}
		}
	}
	else if(register_state == REGISTERSTATE_HEARTBEAT)
	{
		/* check if we should send heartbeat */
		if(now > masterserver_info[register_registered_server].last_send+freq*15)
		{
			masterserver_info[register_registered_server].last_send = now;
			register_send_heartbeat(masterserver_info[register_registered_server].addr);
		}
		
		if(now > register_state_start+freq*60)
		{
			dbg_msg("register", "WARNING: Master server is not responding, switching master");
			register_new_state(REGISTERSTATE_START);
		}
	}
	else if(register_state == REGISTERSTATE_REGISTERED)
	{
		if(register_first)
			dbg_msg("register", "server registered");
			
		register_first = 0;
		
		/* check if we should send new heartbeat again */
		if(now > register_state_start+freq*30)
			register_new_state(REGISTERSTATE_HEARTBEAT);
	}
	else if(register_state == REGISTERSTATE_ERROR)
	{
		/* check for restart */
		if(now > register_state_start+freq*60)
			register_new_state(REGISTERSTATE_START);
	}
}

static void register_got_count(NETPACKET *p)
{
	unsigned char *data = (unsigned char *)p->data;
	int count = (data[sizeof(SERVERBROWSE_COUNT)]<<8) | data[sizeof(SERVERBROWSE_COUNT)+1];
	int i;

	for(i = 0; i < MAX_MASTERSERVERS; i++)
	{
		if(net_addr4_cmp(&masterserver_info[i].addr, &p->address) == 0)
		{
			masterserver_info[i].count = count;
			break;
		}
	}
}

int register_process_packet(NETPACKET *packet)
{
	if(packet->data_size == sizeof(SERVERBROWSE_FWCHECK) &&
		memcmp(packet->data, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
	{
		register_send_fwcheckresponse(&packet->address);
		return 1;
	}
	else if(packet->data_size == sizeof(SERVERBROWSE_FWOK) &&
		memcmp(packet->data, SERVERBROWSE_FWOK, sizeof(SERVERBROWSE_FWOK)) == 0)
	{
		if(register_first)
			dbg_msg("register", "no firewall/nat problems detected");
		register_new_state(REGISTERSTATE_REGISTERED);
		return 1;
	}
	else if(packet->data_size == sizeof(SERVERBROWSE_FWERROR) &&
		memcmp(packet->data, SERVERBROWSE_FWERROR, sizeof(SERVERBROWSE_FWERROR)) == 0)
	{
		dbg_msg("register", "ERROR: the master server reports that clients can not connect to this server.");
		dbg_msg("register", "ERROR: configure your firewall/nat to let trough udp on port %d.", config.sv_port);
		register_new_state(REGISTERSTATE_ERROR);
		return 1;
	}
	else if(packet->data_size == sizeof(SERVERBROWSE_COUNT)+2 &&
		memcmp(packet->data, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT)) == 0)
	{
		register_got_count(packet);
		return 1;
	}

	return 0;
}
