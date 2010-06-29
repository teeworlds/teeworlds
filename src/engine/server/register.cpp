// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <base/system.h>
#include <engine/shared/network.h>
#include <engine/shared/config.h>
#include <engine/shared/engine.h>
#include <engine/masterserver.h>

#include <mastersrv/mastersrv.h>

#include "register.h"

CRegister::CRegister()
{
	m_pNetServer = 0;
	m_pMasterServer = 0;

	m_RegisterState = REGISTERSTATE_START;
	m_RegisterStateStart = 0;
	m_RegisterFirst = 1;
	m_RegisterCount = 0;

	mem_zero(m_aMasterserverInfo, sizeof(m_aMasterserverInfo));
	m_RegisterRegisteredServer = -1;
}

void CRegister::RegisterNewState(int State)
{
	m_RegisterState = State;
	m_RegisterStateStart = time_get();
}

void CRegister::RegisterSendFwcheckresponse(NETADDR *pAddr)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_FWRESPONSE);
	Packet.m_pData = SERVERBROWSE_FWRESPONSE;
	m_pNetServer->Send(&Packet);
}
	
void CRegister::RegisterSendHeartbeat(NETADDR Addr)
{
	static unsigned char aData[sizeof(SERVERBROWSE_HEARTBEAT) + 2];
	unsigned short Port = g_Config.m_SvPort;
	CNetChunk Packet;
	
	mem_copy(aData, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT));
	
	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_HEARTBEAT) + 2;
	Packet.m_pData = &aData;

	// supply the set port that the master can use if it has problems
	if(g_Config.m_SvExternalPort)
		Port = g_Config.m_SvExternalPort;
	aData[sizeof(SERVERBROWSE_HEARTBEAT)] = Port >> 8;
	aData[sizeof(SERVERBROWSE_HEARTBEAT)+1] = Port&0xff;
	m_pNetServer->Send(&Packet);
}

void CRegister::RegisterSendCountRequest(NETADDR Addr)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_GETCOUNT);
	Packet.m_pData = SERVERBROWSE_GETCOUNT;
	m_pNetServer->Send(&Packet);
}

void CRegister::RegisterGotCount(CNetChunk *pChunk)
{
	unsigned char *pData = (unsigned char *)pChunk->m_pData;
	int Count = (pData[sizeof(SERVERBROWSE_COUNT)]<<8) | pData[sizeof(SERVERBROWSE_COUNT)+1];

	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		if(net_addr_comp(&m_aMasterserverInfo[i].m_Addr, &pChunk->m_Address) == 0)
		{
			m_aMasterserverInfo[i].m_Count = Count;
			break;
		}
	}
}

void CRegister::Init(CNetServer *pNetServer, IEngineMasterServer *pMasterServer)
{
	m_pNetServer = pNetServer;
	m_pMasterServer = pMasterServer;
}

void CRegister::RegisterUpdate()
{
	int64 Now = time_get();
	int64 Freq = time_freq();

	if(!g_Config.m_SvRegister)
		return;

	m_pMasterServer->Update();

	if(m_RegisterState == REGISTERSTATE_START)
	{
		m_RegisterCount = 0;
		m_RegisterFirst = 1;
		RegisterNewState(REGISTERSTATE_UPDATE_ADDRS);
		m_pMasterServer->RefreshAddresses();
		dbg_msg("register", "refreshing ip addresses");
	}
	else if(m_RegisterState == REGISTERSTATE_UPDATE_ADDRS)
	{
		m_RegisterRegisteredServer = -1;
	
		if(!m_pMasterServer->IsRefreshing())
		{
			int i;
			for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				NETADDR addr = m_pMasterServer->GetAddr(i);
				m_aMasterserverInfo[i].m_Addr = addr;
				m_aMasterserverInfo[i].m_Count = 0;
			
				if(!addr.ip[0] && !addr.ip[1] && !addr.ip[2] && !addr.ip[3])
					m_aMasterserverInfo[i].m_Valid = 0;
				else
				{
					m_aMasterserverInfo[i].m_Valid = 1;
					m_aMasterserverInfo[i].m_Count = -1;
					m_aMasterserverInfo[i].m_LastSend = 0;
				}
			}
			
			dbg_msg("register", "fetching server counts");
			RegisterNewState(REGISTERSTATE_QUERY_COUNT);
		}
	}
	else if(m_RegisterState == REGISTERSTATE_QUERY_COUNT)
	{
		int Left = 0;
		for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
		{
			if(!m_aMasterserverInfo[i].m_Valid)
				continue;
				
			if(m_aMasterserverInfo[i].m_Count == -1)
			{
				Left++;
				if(m_aMasterserverInfo[i].m_LastSend+Freq < Now)
				{
					m_aMasterserverInfo[i].m_LastSend = Now;
					RegisterSendCountRequest(m_aMasterserverInfo[i].m_Addr);
				}
			}
		}
		
		// check if we are done or timed out
		if(Left == 0 || Now > m_RegisterStateStart+Freq*3)
		{
			// choose server
			int Best = -1;
			int i;
			for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				if(!m_aMasterserverInfo[i].m_Valid || m_aMasterserverInfo[i].m_Count == -1)
					continue;
					
				if(Best == -1 || m_aMasterserverInfo[i].m_Count < m_aMasterserverInfo[Best].m_Count)
					Best = i;
			}

			// server chosen
			m_RegisterRegisteredServer = Best;
			if(m_RegisterRegisteredServer == -1)
			{
				dbg_msg("register", "WARNING: No master servers. Retrying in 60 seconds");
				RegisterNewState(REGISTERSTATE_ERROR);
			}
			else
			{			
				dbg_msg("register", "choosen '%s' as master, sending heartbeats", m_pMasterServer->GetName(m_RegisterRegisteredServer));
				m_aMasterserverInfo[m_RegisterRegisteredServer].m_LastSend = 0;
				RegisterNewState(REGISTERSTATE_HEARTBEAT);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_HEARTBEAT)
	{
		// check if we should send heartbeat
		if(Now > m_aMasterserverInfo[m_RegisterRegisteredServer].m_LastSend+Freq*15)
		{
			m_aMasterserverInfo[m_RegisterRegisteredServer].m_LastSend = Now;
			RegisterSendHeartbeat(m_aMasterserverInfo[m_RegisterRegisteredServer].m_Addr);
		}
		
		if(Now > m_RegisterStateStart+Freq*60)
		{
			dbg_msg("register", "WARNING: Master server is not responding, switching master");
			RegisterNewState(REGISTERSTATE_START);
		}
	}
	else if(m_RegisterState == REGISTERSTATE_REGISTERED)
	{
		if(m_RegisterFirst)
			dbg_msg("register", "server registered");
			
		m_RegisterFirst = 0;
		
		// check if we should send new heartbeat again
		if(Now > m_RegisterStateStart+Freq)
		{
			if(m_RegisterCount == 120) // redo the whole process after 60 minutes to balance out the master servers
				RegisterNewState(REGISTERSTATE_START);
			else
			{
				m_RegisterCount++;
				RegisterNewState(REGISTERSTATE_HEARTBEAT);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_ERROR)
	{
		// check for restart
		if(Now > m_RegisterStateStart+Freq*60)
			RegisterNewState(REGISTERSTATE_START);
	}
}

int CRegister::RegisterProcessPacket(CNetChunk *pPacket)
{
	// check for masterserver address
	bool Valid = false;
	NETADDR Addr1 = pPacket->m_Address;
	Addr1.port = 0;
	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		NETADDR Addr2 = m_aMasterserverInfo[i].m_Addr;
		Addr2.port = 0;
		if(net_addr_comp(&Addr1, &Addr2) == 0)
		{
			Valid = true;
			break;
		}
	}
	if(!Valid)
		return 0;
			
	if(pPacket->m_DataSize == sizeof(SERVERBROWSE_FWCHECK) &&
		mem_comp(pPacket->m_pData, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
	{
		RegisterSendFwcheckresponse(&pPacket->m_Address);
		return 1;
	}
	else if(pPacket->m_DataSize == sizeof(SERVERBROWSE_FWOK) &&
		mem_comp(pPacket->m_pData, SERVERBROWSE_FWOK, sizeof(SERVERBROWSE_FWOK)) == 0)
	{
		if(m_RegisterFirst)
			dbg_msg("register", "no firewall/nat problems detected");
		RegisterNewState(REGISTERSTATE_REGISTERED);
		return 1;
	}
	else if(pPacket->m_DataSize == sizeof(SERVERBROWSE_FWERROR) &&
		mem_comp(pPacket->m_pData, SERVERBROWSE_FWERROR, sizeof(SERVERBROWSE_FWERROR)) == 0)
	{
		dbg_msg("register", "ERROR: the master server reports that clients can not connect to this server.");
		dbg_msg("register", "ERROR: configure your firewall/nat to let through udp on port %d.", g_Config.m_SvPort);
		RegisterNewState(REGISTERSTATE_ERROR);
		return 1;
	}
	else if(pPacket->m_DataSize == sizeof(SERVERBROWSE_COUNT)+2 &&
		mem_comp(pPacket->m_pData, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT)) == 0)
	{
		RegisterGotCount(pPacket);
		return 1;
	}

	return 0;
}
