/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/kernel.h>
#include <engine/storage.h>

#include <engine/shared/config.h>
#include <engine/shared/netban.h>
#include <engine/shared/network.h>

#include "mastersrv.h"


enum {
	MTU = 1400,
	MAX_SERVERS_PER_PACKET=75,
	MAX_PACKETS=16,
	MAX_SERVERS=MAX_SERVERS_PER_PACKET*MAX_PACKETS,
	EXPIRE_TIME = 90
};

struct CCheckServer
{
	enum ServerType m_Type;
	NETADDR m_Address;
	NETADDR m_AltAddress;
	int m_TryCount;
	int64 m_TryTime;
};

static CCheckServer m_aCheckServers[MAX_SERVERS];
static int m_NumCheckServers = 0;

struct CServerEntry
{
	enum ServerType m_Type;
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
		CMastersrvAddr m_aServers[MAX_SERVERS_PER_PACKET];
	} m_Data;
};

CPacketData m_aPackets[MAX_PACKETS];
static int m_NumPackets = 0;

// legacy code
struct CPacketDataLegacy
{
	int m_Size;
	struct {
		unsigned char m_aHeader[sizeof(SERVERBROWSE_LIST_LEGACY)];
		CMastersrvAddrLegacy m_aServers[MAX_SERVERS_PER_PACKET];
	} m_Data;
};

CPacketDataLegacy m_aPacketsLegacy[MAX_PACKETS];
static int m_NumPacketsLegacy = 0;


struct CCountPacketData
{
	unsigned char m_Header[sizeof(SERVERBROWSE_COUNT)];
	unsigned char m_High;
	unsigned char m_Low;
};

static CCountPacketData m_CountData;
static CCountPacketData m_CountDataLegacy;


CNetBan m_NetBan;

static CNetClient m_NetChecker; // NAT/FW checker
static CNetClient m_NetOp; // main

IConsole *m_pConsole;

void BuildPackets()
{
	CServerEntry *pCurrent = &m_aServers[0];
	int ServersLeft = m_NumServers;
	m_NumPackets = 0;
	m_NumPacketsLegacy = 0;
	int PacketIndex = 0;
	int PacketIndexLegacy = 0;
	while(ServersLeft-- && (m_NumPackets + m_NumPacketsLegacy) < MAX_PACKETS)
	{
		if(pCurrent->m_Type == SERVERTYPE_NORMAL)
		{
			if(PacketIndex % MAX_SERVERS_PER_PACKET == 0)
			{
				PacketIndex = 0;
				m_NumPackets++;
			}

			// copy header
			mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aHeader, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));

			// copy server addresses
			if(pCurrent->m_Address.type == NETTYPE_IPV6)
			{
				mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp, pCurrent->m_Address.ip,
					sizeof(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp));
			}
			else
			{
				static unsigned char IPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };

				mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp, IPV4Mapping, sizeof(IPV4Mapping));
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[12] = pCurrent->m_Address.ip[0];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[13] = pCurrent->m_Address.ip[1];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[14] = pCurrent->m_Address.ip[2];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[15] = pCurrent->m_Address.ip[3];
			}

			m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aPort[0] = (pCurrent->m_Address.port>>8)&0xff;
			m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aPort[1] = pCurrent->m_Address.port&0xff;

			PacketIndex++;

			m_aPackets[m_NumPackets-1].m_Size = sizeof(SERVERBROWSE_LIST) + sizeof(CMastersrvAddr)*PacketIndex;

			pCurrent++;
		}
		else if(pCurrent->m_Type == SERVERTYPE_LEGACY)
		{
			if(PacketIndexLegacy % MAX_SERVERS_PER_PACKET == 0)
			{
				PacketIndexLegacy = 0;
				m_NumPacketsLegacy++;
			}

			// copy header
			mem_copy(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aHeader, SERVERBROWSE_LIST_LEGACY, sizeof(SERVERBROWSE_LIST_LEGACY));

			// copy server addresses
			mem_copy(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aIp, pCurrent->m_Address.ip,
				sizeof(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aIp));
			// 0.5 has the port in little endian on the network
			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aPort[0] = pCurrent->m_Address.port&0xff;
			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aPort[1] = (pCurrent->m_Address.port>>8)&0xff;

			PacketIndexLegacy++;

			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Size = sizeof(SERVERBROWSE_LIST_LEGACY) + sizeof(CMastersrvAddrLegacy)*PacketIndexLegacy;

			pCurrent++;
		}
		else
		{
			*pCurrent = m_aServers[m_NumServers-1];
			m_NumServers--;
			dbg_msg("mastersrv", "error: server of invalid type, dropping it");
		}
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

void AddCheckserver(NETADDR *pInfo, NETADDR *pAlt, ServerType Type)
{
	// add server
	if(m_NumCheckServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pInfo, aAddrStr, sizeof(aAddrStr), true);
	char aAltAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pAlt, aAltAddrStr, sizeof(aAltAddrStr), true);
	dbg_msg("mastersrv", "checking: %s (%s)", aAddrStr, aAltAddrStr);
	m_aCheckServers[m_NumCheckServers].m_Address = *pInfo;
	m_aCheckServers[m_NumCheckServers].m_AltAddress = *pAlt;
	m_aCheckServers[m_NumCheckServers].m_TryCount = 0;
	m_aCheckServers[m_NumCheckServers].m_TryTime = 0;
	m_aCheckServers[m_NumCheckServers].m_Type = Type;
	m_NumCheckServers++;
}

void AddServer(NETADDR *pInfo, ServerType Type)
{
	// see if server already exists in list
	for(int i = 0; i < m_NumServers; i++)
	{
		if(net_addr_comp(&m_aServers[i].m_Address, pInfo) == 0)
		{
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(pInfo, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("mastersrv", "updated: %s", aAddrStr);
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

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pInfo, aAddrStr, sizeof(aAddrStr), true);
	dbg_msg("mastersrv", "added: %s", aAddrStr);
	m_aServers[m_NumServers].m_Address = *pInfo;
	m_aServers[m_NumServers].m_Expire = time_get()+time_freq()*EXPIRE_TIME;
	m_aServers[m_NumServers].m_Type = Type;
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
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&m_aCheckServers[i].m_Address, aAddrStr, sizeof(aAddrStr), true);
				char aAltAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&m_aCheckServers[i].m_AltAddress, aAltAddrStr, sizeof(aAltAddrStr), true);
				dbg_msg("mastersrv", "check failed: %s (%s)", aAddrStr, aAltAddrStr);

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
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&m_aServers[i].m_Address, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("mastersrv", "expired: %s", aAddrStr);
			m_aServers[i] = m_aServers[m_NumServers-1];
			m_NumServers--;
		}
		else
			i++;
	}
}

void ReloadBans()
{
	m_NetBan.UnbanAll();
	m_pConsole->ExecuteFile("master.cfg");
}

int main(int argc, const char **argv) // ignore_convention
{
	int64 LastBuild = 0, LastBanReload = 0;
	ServerType Type = SERVERTYPE_INVALID;
	NETADDR BindAddr;

	dbg_logger_stdout();
	net_init();

	mem_copy(m_CountData.m_Header, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT));
	mem_copy(m_CountDataLegacy.m_Header, SERVERBROWSE_COUNT_LEGACY, sizeof(SERVERBROWSE_COUNT_LEGACY));

	IKernel *pKernel = IKernel::Create();
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	IConfig *pConfig = CreateConfig();
	m_pConsole = CreateConsole(CFGFLAG_MASTER);
	
	bool RegisterFail = !pKernel->RegisterInterface(pStorage);
	RegisterFail |= !pKernel->RegisterInterface(m_pConsole);
	RegisterFail |= !pKernel->RegisterInterface(pConfig);

	if(RegisterFail)
		return -1;

	pConfig->Init();
	m_NetBan.Init(m_pConsole, pStorage);
	if(argc > 1) // ignore_convention
		m_pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
	{
		// got bindaddr
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = MASTERSERVER_PORT;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = MASTERSERVER_PORT;
	}

	if(!m_NetOp.Open(BindAddr, 0))
	{
		dbg_msg("mastersrv", "couldn't start network (op)");
		return -1;
	}
	BindAddr.port = MASTERSERVER_PORT+1;
	if(!m_NetChecker.Open(BindAddr, 0))
	{
		dbg_msg("mastersrv", "couldn't start network (checker)");
		return -1;
	}

	// process pending commands
	m_pConsole->StoreCommands(false);

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
			if(m_NetBan.IsBanned(&Packet.m_Address, 0, 0))
				continue;

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
				AddCheckserver(&Packet.m_Address, &Alt, SERVERTYPE_NORMAL);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_HEARTBEAT_LEGACY)+2 &&
				mem_comp(Packet.m_pData, SERVERBROWSE_HEARTBEAT_LEGACY, sizeof(SERVERBROWSE_HEARTBEAT_LEGACY)) == 0)
			{
				NETADDR Alt;
				unsigned char *d = (unsigned char *)Packet.m_pData;
				Alt = Packet.m_Address;
				Alt.port =
					(d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8) |
					d[sizeof(SERVERBROWSE_HEARTBEAT)+1];

				// add it
				AddCheckserver(&Packet.m_Address, &Alt, SERVERTYPE_LEGACY);
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
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETCOUNT_LEGACY) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETCOUNT_LEGACY, sizeof(SERVERBROWSE_GETCOUNT_LEGACY)) == 0)
			{
				dbg_msg("mastersrv", "count requested, responding with %d", m_NumServers);

				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;
				p.m_DataSize = sizeof(m_CountData);
				p.m_pData = &m_CountDataLegacy;
				m_CountDataLegacy.m_High = (m_NumServers>>8)&0xff;
				m_CountDataLegacy.m_Low = m_NumServers&0xff;
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
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETLIST_LEGACY) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETLIST_LEGACY, sizeof(SERVERBROWSE_GETLIST_LEGACY)) == 0)
			{
				// someone requested the list
				dbg_msg("mastersrv", "requested, responding with %d m_aServers", m_NumServers);

				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;

				for(int i = 0; i < m_NumPacketsLegacy; i++)
				{
					p.m_DataSize = m_aPacketsLegacy[i].m_Size;
					p.m_pData = &m_aPacketsLegacy[i].m_Data;
					m_NetOp.Send(&p);
				}
			}
		}

		// process m_aPackets
		while(m_NetChecker.Recv(&Packet))
		{
			// check if the server is banned
			if(m_NetBan.IsBanned(&Packet.m_Address, 0, 0))
				continue;

			if(Packet.m_DataSize == sizeof(SERVERBROWSE_FWRESPONSE) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_FWRESPONSE, sizeof(SERVERBROWSE_FWRESPONSE)) == 0)
			{
				Type = SERVERTYPE_INVALID;
				// remove it from checking
				for(int i = 0; i < m_NumCheckServers; i++)
				{
					if(net_addr_comp(&m_aCheckServers[i].m_Address, &Packet.m_Address) == 0 ||
						net_addr_comp(&m_aCheckServers[i].m_AltAddress, &Packet.m_Address) == 0)
					{
						Type = m_aCheckServers[i].m_Type;
						m_NumCheckServers--;
						m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers];
						break;
					}
				}

				// drops servers that were not in the CheckServers list
				if(Type == SERVERTYPE_INVALID)
					continue;

				AddServer(&Packet.m_Address, Type);
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
