#include <engine/system.h>

#include <cstdlib>

struct packet
{
	packet *prev;
	packet *next;
	
	NETADDR4 send_to;
	int64 timestamp;
	int id;
	int data_size;
	char data[1];
};

static packet *first = (packet *)0;
static packet *last = (packet *)0;
static int current_latency = 0;
static int debug = 0;

int run(int port, NETADDR4 dest)
{
	NETADDR4 src = {{0,0,0,0},port};
	NETSOCKET socket = net_udp4_create(src);
	
	char buffer[1024*2];
	int id = 0;
	
	while(1)
	{
		// handle incomming packets
		while(1)
		{
			// fetch data
			NETADDR4 from;
			int bytes = net_udp4_recv(socket, &from, buffer, 1024*2);
			if(bytes <= 0)
				break;
				
			//if((rand()%10) == 0) // drop the packet
			//	continue;

			// create new packet				
			packet *p = (packet *)mem_alloc(sizeof(packet)+bytes, 1);

			if(net_addr4_cmp(&from, &dest) == 0)
				p->send_to = src;
			else
			{
				src = from;
				p->send_to = dest;
			}

			// queue packet
			p->prev = last;
			p->next = 0;
			if(last)
				last->next = p;
			else
			{
				first = p;
				last = p;
			}
			last = p;

			// set data in packet			
			p->timestamp = time_get();
			p->data_size = bytes;
			p->id = id++;
			mem_copy(p->data, buffer, bytes);

			if(debug)
				dbg_msg("crapnet", "<< %08d %d.%d.%d.%d:%5d (%d)", p->id, from.ip[0], from.ip[1], from.ip[2], from.ip[3], from.port, p->data_size);
		}
		
		//
		while(1)
		{
			//dbg_msg("crapnet", "%p", first);
			if(first && (time_get()-first->timestamp) > current_latency)
			{
				packet *p = first;
				first = first->next;
				if(first)
					first->prev = 0;
				else
					last = 0;
				
				if(debug)
				{
					dbg_msg("crapnet", ">> %08d %d.%d.%d.%d:%5d (%d)", p->id,
						p->send_to.ip[0], p->send_to.ip[1],
						p->send_to.ip[2], p->send_to.ip[3],
						p->send_to.port, p->data_size);
				}
				
				// send and remove packet
				//if((rand()%20) != 0) // heavy packetloss
				//	net_udp4_send(socket, &p->send_to, p->data, p->data_size);
				
				// update lag
				double flux = rand()/(double)RAND_MAX;
				int ms_spike = 0;
				int ms_flux = 50;
				int ms_ping = 50;
				current_latency = ((time_freq()*ms_ping)/1000) + (int64)(((time_freq()*ms_flux)/1000)*flux); // 50ms
				
				if(ms_spike && (p->id%100) == 0)
					current_latency += (time_freq()*ms_spike)/1000;

				mem_free(p);
			}
			else
				break;
		}
		
		thread_sleep(1);
	}
}

int main(int argc, char **argv)
{
	NETADDR4 a = {{127,0,0,1},8303};
	run(8302, a);
	return 0;
}
