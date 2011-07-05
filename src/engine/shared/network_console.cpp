/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "network.h"

bool CNetConsole::Open(NETADDR BindAddr, int Flags)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));

	// open socket
	m_Socket = net_tcp_create(&BindAddr);
	if(!m_Socket.type)
		return false;
	if(net_tcp_listen(m_Socket, NET_MAX_CONSOLE_CLIENTS))
		return false;
	net_tcp_set_non_blocking(m_Socket);

	for(int i = 0; i < NET_MAX_CLIENTS; i++)
		m_aSlots[i].m_Connection.Init(m_Socket);

	return true;
}

int CNetConsole::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_UserPtr = pUser;
	return 0;
}

int CNetConsole::Close()
{
	// TODO: implement me
	return 0;
}

int CNetConsole::Drop(int ClientID, const char *pReason)
{
	NETADDR Addr = ClientAddr(ClientID);

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr));

	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, pReason, m_UserPtr);

	m_aSlots[ClientID].m_Connection.Disconnect(pReason);

	return 0;
}

int CNetConsole::AcceptClient(NETSOCKET Socket, const NETADDR *pAddr)
{
	char aError[256] = { 0 };
	int FreeSlot = -1;
	
	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(FreeSlot == -1 && m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
			FreeSlot = i;
		if(m_aSlots[i].m_Connection.State() != NET_CONNSTATE_OFFLINE)
		{
			NETADDR PeerAddr = m_aSlots[i].m_Connection.PeerAddress();;
			if(net_addr_comp(pAddr, &PeerAddr) == 0)
			{
				str_copy(aError, "Only one client per IP allowed", sizeof(aError));
				break;
			}
		}
	}

	if(!aError[0] && FreeSlot != -1)
	{
		m_aSlots[FreeSlot].m_Connection.Init(Socket, pAddr);
		if(m_pfnNewClient)
			m_pfnNewClient(FreeSlot, m_UserPtr);
		return 1;
	}

	if(!aError[0])
	{
		str_copy(aError, "No free slot available", sizeof(aError));
	}
	
	dbg_msg("netconsole", "refused client, reason=\"%s\"", aError);

	net_tcp_send(Socket, aError, str_length(aError));
	net_tcp_close(Socket);

	return 0;
}

int CNetConsole::Update()
{
	NETSOCKET Socket;
	NETADDR Addr;

	while(net_tcp_accept(m_Socket, &Socket, &Addr) > 0)
	{
		AcceptClient(Socket, &Addr);
	}

	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ONLINE)
			m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
			Drop(i, m_aSlots[i].m_Connection.ErrorString());
	}

	return 0;
}

int CNetConsole::Recv(char *pLine, int MaxLength, int *pClientID)
{
	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ONLINE && m_aSlots[i].m_Connection.Recv(pLine, MaxLength))
		{
			if(pClientID)
				*pClientID = i;
			return 1;
		}
	}
	return 0;
}

int CNetConsole::Broadcast(const char *pLine)
{
	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ONLINE)
			Send(i, pLine);
	}
	return 0;
}

int CNetConsole::Send(int ClientID, const char *pLine)
{
	return m_aSlots[ClientID].m_Connection.Send(pLine);
}

