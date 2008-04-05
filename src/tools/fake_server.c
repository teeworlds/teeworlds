/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_config.h>
#include <engine/e_system.h>
#include <engine/e_network.h>
#include <mastersrv/mastersrv.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 32 version
 64 servername
 32 mapname
  2 gametype
  2 flags
  4 progression
  3 num players
  3 max players
  {
  	48 name
     6 score
  } * players

         111111111122222222223333333333444444444
123456789012345678901234567890123456789012345678
0.3 2d82e361de24cb25
my own private little server
magnus.auvinen@teeworlds.somehost-strage-host.com
*/

NETSERVER *net;

int progression = 50;
int game_type = 0;
int flags = 0;

const char *version = "0.3.0 2d82e361de24cb25";
const char *map = "somemap";
const char *server_name = "unnamed server";
NETADDR4 master_servers[16] = {{{0},0}};
int num_masters = 0;

const char *player_names[16] = {0};
int player_scores[16] = {0};
int num_players = 0;
int max_players = 0;



static void send_heartbeats()
{
	static unsigned char data[sizeof(SERVERBROWSE_HEARTBEAT) + 2];
	NETCHUNK packet;
	int i;
	
	mem_copy(data, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT));
	
	packet.client_id = -1;
	packet.flags = NETSENDFLAG_CONNLESS;
	packet.data_size = sizeof(SERVERBROWSE_HEARTBEAT) + 2;
	packet.data = &data;

	/* supply the set port that the master can use if it has problems */	
	data[sizeof(SERVERBROWSE_HEARTBEAT)] = 0;
	data[sizeof(SERVERBROWSE_HEARTBEAT)+1] = 0;
	
	for(i = 0; i < num_masters; i++)
	{
		packet.address = master_servers[i];
		netserver_send(net, &packet);
	}
}

char infomsg[1024];
int infomsg_size;

static void writestr(const char *str)
{
	int l = strlen(str)+1;
	memcpy(&infomsg[infomsg_size], str, l);
	infomsg_size += l;
}

static void writeint(int i)
{
	char buf[64];
	sprintf(buf, "%d", i);
	writestr(buf);
}

static void build_infomessage()
{
	int i;
	infomsg_size = sizeof(SERVERBROWSE_INFO);
	memcpy(infomsg, SERVERBROWSE_INFO, infomsg_size);
	
	writestr(version);
	writestr(server_name);
	writestr(map);
	writeint(game_type);
	writeint(flags);
	writeint(progression);
	writeint(num_players);
	writeint(max_players);
	for(i = 0; i < num_players; i++)
	{
		writestr(player_names[i]);
		writeint(player_scores[i]);
	}
}

static void send_serverinfo(NETADDR4 *addr)
{
	NETCHUNK p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	p.data_size = infomsg_size;
	p.data = infomsg;
	netserver_send(net, &p);
}

static void send_fwcheckresponse(NETADDR4 *addr)
{
	NETCHUNK p;
	p.client_id = -1;
	p.address = *addr;
	p.flags = NETSENDFLAG_CONNLESS;
	p.data_size = sizeof(SERVERBROWSE_FWRESPONSE);
	p.data = SERVERBROWSE_FWRESPONSE;
	netserver_send(net, &p);
}

static int run()
{
	int64 next_heartbeat = 0;
	NETADDR4 bindaddr = {{0},0};
	net = netserver_open(bindaddr, 0, 0);
	if(!net)
		return -1;
	
	while(1)
	{
		NETCHUNK p;
		netserver_update(net);
		while(netserver_recv(net, &p))
		{
			if(p.client_id == -1)
			{
				if(p.data_size == sizeof(SERVERBROWSE_GETINFO) &&
					memcmp(p.data, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					send_serverinfo(&p.address);
				}
				else if(p.data_size == sizeof(SERVERBROWSE_FWCHECK) &&
					memcmp(p.data, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
				{
					send_fwcheckresponse(&p.address);
				}
			}
		}
		
		/* send heartbeats if needed */
		if(next_heartbeat < time_get())
		{
			next_heartbeat = time_get()+time_freq()*30;
			send_heartbeats();
		}
		
		thread_sleep(10);
	}
}

int main(int argc, char **argv)
{
	net_init();
	
	while(argc)
	{
		if(strcmp(*argv, "-m") == 0)
		{
			argc--; argv++;
			net_host_lookup(*argv, 0, &master_servers[num_masters]);
			argc--; argv++;
			master_servers[num_masters].port = atoi(*argv);
			num_masters++;
		}
		else if(strcmp(*argv, "-p") == 0)
		{
			argc--; argv++;
			player_names[num_players++] = *argv;
			argc--; argv++;
			player_scores[num_players] = atoi(*argv);
		}
		else if(strcmp(*argv, "-a") == 0)
		{
			argc--; argv++;
			map = *argv;
		}
		else if(strcmp(*argv, "-x") == 0)
		{
			argc--; argv++;
			max_players = atoi(*argv);
		}
		else if(strcmp(*argv, "-t") == 0)
		{
			argc--; argv++;
			game_type = atoi(*argv);
		}
		else if(strcmp(*argv, "-g") == 0)
		{
			argc--; argv++;
			progression = atoi(*argv);
		}
		else if(strcmp(*argv, "-f") == 0)
		{
			argc--; argv++;
			flags = atoi(*argv);
		}
		else if(strcmp(*argv, "-n") == 0)
		{
			argc--; argv++;
			server_name = *argv;
		}
		
		argc--; argv++;
	}
	
	build_infomessage();
	return run();
}

