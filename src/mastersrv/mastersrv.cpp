/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/shared/network.h>
#include <engine/shared/config.h>
#include <engine/console.h>
#include <engine/storage.h>
#include <engine/kernel.h>

#include "mastersrv.h"

enum {
	MTU = 1400,
	MAX_SERVERS_PER_PACKET=128,
	MAX_PACKETS=16,
	MAX_SERVERS=MAX_SERVERS_PER_PACKET*MAX_PACKETS,
	MAX_BANS=128,
	EXPIRE_TIME = 90
};

struct CCheckServer
{
	NETADDR m_Address;
	NETADDR m_AltAddress;
	int m_TryCount;
	int64 m_TryTime;
};

static CCheckServer m_aCheckServers[MAX_SERVERS];
static int m_NumCheckServers = 0;

struct NETADDR_IPv4
{
	unsigned char m_aIp[4];
	unsigned short m_Port;
};

struct CServerEntry
{
	NETADDR m_Address;
	int64 m_Expire;
};

static CServerEntry m_aServers[MAX_SERVERS];
static int m_NumServers = 0;

struct CPacketData
{
	int m_Size;
	struct {
		unsigned char m_aHeader[sizeof(SERVERBROWSE_LIST)];
		NETADDR_IPv4 m_aServers[MAX_SERVERS_PER_PACKET];
	} m_Data;
};

CPacketData m_aPackets[MAX_PACKETS];
static int m_NumPackets = 0;

struct CCountPacketData
{
	unsigned char m_Header[sizeof(SERVERBROWSE_COUNT)];
	unsigned char m_High;
	unsigned char m_Low;
};

static CCountPacketData m_CountData;


struct CBanEntry
{
	NETADDR m_Address;
	int64 m_Expire;
};

static CBanEntry m_aBans[MAX_BANS];
static int m_NumBans = 0;

//static int64 server_expire[MAX_SERVERS];

static CNetClient m_NetChecker; // NAT/FW checker
static CNetClient m_NetOp; // main

IConsole *m_pConsole;

void BuildPackets()
{
	CServerEntry *pCurrent = &m_aServers[0];
	int ServersLeft = m_NumServers;
	m_NumPackets = 0;
	while(ServersLeft && m_NumPackets < MAX_PACKETS)
	{
		int Chunk = ServersLeft;
		if(Chunk > MAX_SERVERS_PER_PACKET)
			Chunk = MAX_SERVERS_PER_PACKET;
		ServersLeft -= Chunk;
		
		// copy header	
		mem_copy(m_aPackets[m_NumPackets].m_Data.m_aHeader, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));
		
		// copy server addresses
		for(int i = 0; i < Chunk; i++)
		{
			// TODO: ipv6 support
			m_aPackets[m_NumPackets].m_Data.m_aServers[i].m_aIp[0] = pCurrent->m_Address.ip[0];
			m_aPackets[m_NumPackets].m_Data.m_aServers[i].m_aIp[1] = pCurrent->m_Address.ip[1];
			m_aPackets[m_NumPackets].m_Data.m_aServers[i].m_aIp[2] = pCurrent->m_Address.ip[2];
			m_aPackets[m_NumPackets].m_Data.m_aServers[i].m_aIp[3] = pCurrent->m_Address.ip[3];
			m_aPackets[m_NumPackets].m_Data.m_aServers[i].m_Port = pCurrent->m_Address.port;
			pCurrent++;
		}
		
		m_aPackets[m_NumPackets].m_Size = sizeof(SERVERBROWSE_LIST) + sizeof(NETADDR_IPv4)*Chunk;
		
		m_NumPackets++;
	}
}

void SendOk(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWOK);
	p.m_pData = SERVERBROWSE_FWOK;
	
	// send on both to be sure
	m_NetChecker.Send(&p);
	m_NetOp.Send(&p);
}

void SendError(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWERROR);
	p.m_pData = SERVERBROWSE_FWERROR;
	m_NetOp.Send(&p);
}

void SendCheck(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWCHECK);
	p.m_pData = SERVERBROWSE_FWCHECK;
	m_NetChecker.Send(&p);
}

void AddCheckserver(NETADDR *pInfo, NETADDR *pAlt)
{
	// add server
	if(m_NumCheckServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}
	
	dbg_msg("mastersrv", "checking: %d.%d.%d.%d:%d (%d.%d.%d.%d:%d)",
		pInfo->ip[0], pInfo->ip[1], pInfo->ip[2], pInfo->ip[3], pInfo->port,
		pAlt->ip[0], pAlt->ip[1], pAlt->ip[2], pAlt->ip[3], pAlt->port);
	m_aCheckServers[m_NumCheckServers].m_Address = *pInfo;
	m_aCheckServers[m_NumCheckServers].m_AltAddress = *pAlt;
	m_aCheckServers[m_NumCheckServers].m_TryCount = 0;
	m_aCheckServers[m_NumCheckServers].m_TryTime = 0;
	m_NumCheckServers++;
}

void AddServer(NETADDR *pInfo)
{
	// see if server already exists in list
	int i;
	for(i = 0; i < m_NumServers; i++)
	{
		if(net_addr_comp(&m_aServers[i].m_Address, pInfo) == 0)
		{
			dbg_msg("mastersrv", "updated: %d.%d.%d.%d:%d",
				pInfo->ip[0], pInfo->ip[1], pInfo->ip[2], pInfo->ip[3], pInfo->port);
			m_aServers[i].m_Expire = time_get()+time_freq()*EXPIRE_TIME;
			return;
		}
	}
	
	// add server
	if(m_NumServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}
	
	dbg_msg("mastersrv", "added: %d.%d.%d.%d:%d",
		pInfo->ip[0], pInfo->ip[1], pInfo->ip[2], pInfo->ip[3], pInfo->port);
	m_aServers[m_NumServers].m_Address = *pInfo;
	m_aServers[m_NumServers].m_Expire = time_get()+time_freq()*EXPIRE_TIME;
	m_NumServers++;
}

void UpdateServers()
{
	int64 Now = time_get();
	int64 Freq = time_freq();
	for(int i = 0; i < m_NumCheckServers; i++)
	{
		if(Now > m_aCheckServers[i].m_TryTime+Freq)
		{
			if(m_aCheckServers[i].m_TryCount == 10)
			{
				dbg_msg("mastersrv", "check failed: %d.%d.%d.%d:%d",
					m_aCheckServers[i].m_Address.ip[0], m_aCheckServers[i].m_Address.ip[1],
					m_aCheckServers[i].m_Address.ip[2], m_aCheckServers[i].m_Address.ip[3],m_aCheckServers[i].m_Address.port,
					m_aCheckServers[i].m_AltAddress.ip[0], m_aCheckServers[i].m_AltAddress.ip[1],
					m_aCheckServers[i].m_AltAddress.ip[2], m_aCheckServers[i].m_AltAddress.ip[3],m_aCheckServers[i].m_AltAddress.port);
					
				// FAIL!!
				SendError(&m_aCheckServers[i].m_Address);
				m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers-1];
				m_NumCheckServers--;
				i--;
			}
			else
			{
				m_aCheckServers[i].m_TryCount++;
				m_aCheckServers[i].m_TryTime = Now;
				if(m_aCheckServers[i].m_TryCount&1)
					SendCheck(&m_aCheckServers[i].m_Address);
				else
					SendCheck(&m_aCheckServers[i].m_AltAddress);
			}
		}
	}
}

void PurgeServers()
{
	int64 Now = time_get();
	int i = 0;
	while(i < m_NumServers)
	{
		if(m_aServers[i].m_Expire < Now)
		{
			// remove server
			dbg_msg("mastersrv", "expired: %d.%d.%d.%d:%d",
				m_aServers[i].m_Address.ip[0], m_aServers[i].m_Address.ip[1],
				m_aServers[i].m_Address.ip[2], m_aServers[i].m_Address.ip[3], m_aServers[i].m_Address.port);
			m_aServers[i] = m_aServers[m_NumServers-1];
			m_NumServers--;
		}
		else
			i++;
	}
}

bool CheckBan(NETADDR Addr)
{
	for(int i = 0; i < m_NumBans; i++)
	{
		if(net_addr_comp(&m_aBans[i].m_Address, &Addr) == 0)
		{
			return true;
		}
	}
	Addr.port = 0;
	for(int i = 0; i < m_NumBans; i++)
	{
		if(net_addr_comp(&m_aBans[i].m_Address, &Addr) == 0)
		{
			return true;
		}
	}
	return false;
}

void ConAddBan(IConsole::IResult *pResult, void *pUser)
{
	if(m_NumBans == MAX_BANS)
	{
		dbg_msg("mastersrv", "error: banlist is full");
		return;
	}
	
	net_addr_from_str(&m_aBans[m_NumBans].m_Address, pResult->GetString(0));
	
	if(CheckBan(m_aBans[m_NumBans].m_Address))
	{
		dbg_msg("mastersrv", "duplicate ban: %d.%d.%d.%d:%d",
			m_aBans[m_NumBans].m_Address.ip[0], m_aBans[m_NumBans].m_Address.ip[1],
			m_aBans[m_NumBans].m_Address.ip[2], m_aBans[m_NumBans].m_Address.ip[3],
			m_aBans[m_NumBans].m_Address.port);
		return;
	}
	
	dbg_msg("mastersrv", "ban added: %d.%d.%d.%d:%d",
		m_aBans[m_NumBans].m_Address.ip[0], m_aBans[m_NumBans].m_Address.ip[1],
		m_aBans[m_NumBans].m_Address.ip[2], m_aBans[m_NumBans].m_Address.ip[3],
		m_aBans[m_NumBans].m_Address.port);
	m_NumBans++;
}

void ReloadBans()
{
	m_NumBans = 0;
	m_pConsole->ExecuteFile("master.cfg");
}

int main(int argc, const char **argv) // ignore_convention
{
	int64 LastBuild = 0, LastBanReload = 0;
	NETADDR BindAddr;

	dbg_logger_stdout();
	net_init();

	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.port = MASTERSERVER_PORT;
		
	m_NetOp.Open(BindAddr, 0);

	BindAddr.port = MASTERSERVER_PORT+1;
	m_NetChecker.Open(BindAddr, 0);
	// TODO: check socket for errors
	
	//mem_copy(data.header, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));
	mem_copy(m_CountData.m_Header, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT));

	IKernel *pKernel = IKernel::Create();
	IStorage *pStorage = CreateStorage("Teeworlds", argc, argv);

	m_pConsole = CreateConsole(CFGFLAG_MASTER);
	m_pConsole->Register("ban", "s", CFGFLAG_MASTER, ConAddBan, 0, "Ban IP from mastersrv");

	bool RegisterFail = !pKernel->RegisterInterface(pStorage);
	RegisterFail |= !pKernel->RegisterInterface(m_pConsole);

	if(RegisterFail)
		return -1;
	
	dbg_msg("mastersrv", "started");
	
	while(1)
	{
		m_NetOp.Update();
		m_NetChecker.Update();
		
		// process m_aPackets
		CNetChunk Packet;
		while(m_NetOp.Recv(&Packet))
		{
			// check if the server is banned
			if(CheckBan(Packet.m_Address)) continue;

			if(Packet.m_DataSize == sizeof(SERVERBROWSE_HEARTBEAT)+2 &&
				mem_comp(Packet.m_pData, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT)) == 0)
			{
				NETADDR Alt;
				unsigned char *d = (unsigned char *)Packet.m_pData;
				Alt = Packet.m_Address;
				Alt.port =
					(d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8) |
					d[sizeof(SERVERBROWSE_HEARTBEAT)+1];
				
				// add it
				AddCheckserver(&Packet.m_Address, &Alt);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETCOUNT) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETCOUNT, sizeof(SERVERBROWSE_GETCOUNT)) == 0)
			{
				dbg_msg("mastersrv", "count requested, responding with %d", m_NumServers);
				
				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;
				p.m_DataSize = sizeof(m_CountData);
				p.m_pData = &m_CountData;
				m_CountData.m_High = (m_NumServers>>8)&0xff;
				m_CountData.m_Low = m_NumServers&0xff;
				m_NetOp.Send(&p);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETLIST) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST)) == 0)
			{
				// someone requested the list
				dbg_msg("mastersrv", "requested, responding with %d m_aServers", m_NumServers);
				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;
				
				for(int i = 0; i < m_NumPackets; i++)
				{
					p.m_DataSize = m_aPackets[i].m_Size;
					p.m_pData = &m_aPackets[i].m_Data;
					m_NetOp.Send(&p);
				}
			}
		}

		// process m_aPackets
		while(m_NetChecker.Recv(&Packet))
		{
			// check if the server is banned
			if(CheckBan(Packet.m_Address)) continue;
			
			if(Packet.m_DataSize == sizeof(SERVERBROWSE_FWRESPONSE) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_FWRESPONSE, sizeof(SERVERBROWSE_FWRESPONSE)) == 0)
			{
				// remove it from checking
				for(int i = 0; i < m_NumCheckServers; i++)
				{
					if(net_addr_comp(&m_aCheckServers[i].m_Address, &Packet.m_Address) == 0 ||
						net_addr_comp(&m_aCheckServers[i].m_AltAddress, &Packet.m_Address) == 0)
					{
						m_NumCheckServers--;
						m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers];
						break;
					}
				}
				
				AddServer(&Packet.m_Address);
				SendOk(&Packet.m_Address);
			}
		}
		
		if(time_get()-LastBanReload > time_freq()*300)
		{
			LastBanReload = time_get();

			ReloadBans();
		}

		if(time_get()-LastBuild > time_freq()*5)
		{
			LastBuild = time_get();
			
			PurgeServers();
			UpdateServers();
			BuildPackets();
		}
		
		// be nice to the CPU
		thread_sleep(1);
	}
	
	return 0;
}
