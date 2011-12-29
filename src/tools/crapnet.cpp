/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <cstdlib>

struct CPacket
{
	CPacket *m_pPrev;
	CPacket *m_pNext;

	NETADDR m_SendTo;
	int64 m_Timestamp;
	int m_ID;
	int m_DataSize;
	char m_aData[1];
};

static CPacket *m_pFirst = (CPacket *)0;
static CPacket *m_pLast = (CPacket *)0;
static int m_CurrentLatency = 0;

struct CPingConfig
{
	int m_Base;
	int m_Flux;
	int m_Spike;
	int m_Loss;
	int m_Delay;
	int m_DelayFreq;
};

static CPingConfig m_aConfigPings[] = {
//		base	flux	spike	loss	delay	delayfreq
		{0,		0,		0,		0,		0,		0},
		{40,	20,		100,		0,		0,		0},
		{140,	40,		200,		0,		0,		0},
};

static int m_ConfigNumpingconfs = sizeof(m_aConfigPings)/sizeof(CPingConfig);
static int m_ConfigInterval = 10; // seconds between different pingconfigs
static int m_ConfigLog = 0;
static int m_ConfigReorder = 0;

void Run(int Port, NETADDR Dest)
{
	NETADDR Src = {NETTYPE_IPV4, {0,0,0,0}, Port};
	NETSOCKET Socket = net_udp_create(Src);

	char aBuffer[1024*2];
	int ID = 0;
	int Delaycounter = 0;

	while(1)
	{
		static int Lastcfg = 0;
		int n = ((time_get()/time_freq())/m_ConfigInterval) % m_ConfigNumpingconfs;
		CPingConfig Ping = m_aConfigPings[n];

		if(n != Lastcfg)
			dbg_msg("crapnet", "cfg = %d", n);
		Lastcfg = n;

		// handle incomming packets
		while(1)
		{
			// fetch data
			int DataTrash = 0;
			NETADDR From;
			int Bytes = net_udp_recv(Socket, &From, aBuffer, 1024*2);
			if(Bytes <= 0)
				break;

			if((rand()%100) < Ping.m_Loss) // drop the packet
			{
				if(m_ConfigLog)
					dbg_msg("crapnet", "dropped packet");
				continue;
			}

			// create new packet
			CPacket *p = (CPacket *)mem_alloc(sizeof(CPacket)+Bytes, 1);

			if(net_addr_comp(&From, &Dest) == 0)
				p->m_SendTo = Src; // from the server
			else
			{
				Src = From; // from the client
				p->m_SendTo = Dest;
			}

			// queue packet
			p->m_pPrev = m_pLast;
			p->m_pNext = 0;
			if(m_pLast)
				m_pLast->m_pNext = p;
			else
			{
				m_pFirst = p;
				m_pLast = p;
			}
			m_pLast = p;

			// set data in packet
			p->m_Timestamp = time_get();
			p->m_DataSize = Bytes;
			p->m_ID = ID++;
			mem_copy(p->m_aData, aBuffer, Bytes);

			if(ID > 20 && Bytes > 6 && DataTrash)
			{
				p->m_aData[6+(rand()%(Bytes-6))] = rand()&255; // modify a byte
				if((rand()%10) == 0)
				{
					p->m_DataSize -= rand()%32;
					if(p->m_DataSize < 6)
						p->m_DataSize = 6;
				}
			}

			if(Delaycounter <= 0)
			{
				if(Ping.m_Delay)
					p->m_Timestamp += (time_freq()*1000)/Ping.m_Delay;
				Delaycounter = Ping.m_DelayFreq;
			}
			Delaycounter--;

			if(m_ConfigLog)
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&From, aAddrStr, sizeof(aAddrStr), true);
				dbg_msg("crapnet", "<< %08d %s (%d)", p->m_ID, aAddrStr, p->m_DataSize);
			}
		}

		//
		/*while(1)
		{*/
		CPacket *p = 0;
		CPacket *pNext = m_pFirst;
		while(1)
		{
			p = pNext;
			if(!p)
				break;
			pNext = p->m_pNext;

			if((time_get()-p->m_Timestamp) > m_CurrentLatency)
			{
				char aFlags[] = "  ";

				if(m_ConfigReorder && (rand()%2) == 0 && p->m_pNext)
				{
					aFlags[0] = 'R';
					p = m_pFirst->m_pNext;
				}

				if(p->m_pNext)
					p->m_pNext->m_pPrev = p->m_pPrev;
				else
					m_pLast = p->m_pPrev;

				if(p->m_pPrev)
					p->m_pPrev->m_pNext = p->m_pNext;
				else
					m_pFirst = p->m_pNext;

				/*CPacket *cur = first;
				while(cur)
				{
					dbg_assert(cur != p, "p still in list");
					cur = cur->next;
				}*/

				// send and remove packet
				//if((rand()%20) != 0) // heavy packetloss
				net_udp_send(Socket, &p->m_SendTo, p->m_aData, p->m_DataSize);

				// update lag
				double Flux = rand()/(double)RAND_MAX;
				int MsSpike = Ping.m_Spike;
				int MsFlux = Ping.m_Flux;
				int MsPing = Ping.m_Base;
				m_CurrentLatency = ((time_freq()*MsPing)/1000) + (int64)(((time_freq()*MsFlux)/1000)*Flux); // 50ms

				if(MsSpike && (p->m_ID%100) == 0)
				{
					m_CurrentLatency += (time_freq()*MsSpike)/1000;
					aFlags[1] = 'S';
				}

				if(m_ConfigLog)
				{
					char aAddrStr[NETADDR_MAXSTRSIZE];
					net_addr_str(&p->m_SendTo, aAddrStr, sizeof(aAddrStr), true);
					dbg_msg("crapnet", ">> %08d %s (%d) %s", p->m_ID, aAddrStr, p->m_DataSize, aFlags);
				}


				mem_free(p);
			}
		}

		thread_sleep(1);
	}
}

int main(int argc, char **argv) // ignore_convention
{
	NETADDR Addr = {NETTYPE_IPV4, {127,0,0,1},8303};
	dbg_logger_stdout();
	Run(8302, Addr);
	return 0;
}
