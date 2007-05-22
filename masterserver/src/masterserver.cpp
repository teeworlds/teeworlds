#include <ctime>

#include "masterserver.h"

void CMasterServer::Init(int port)
{
	netaddr4 addr(0, 0, 0, 0, port);
	addr.port = port;
	
	net_init();
	m_UDPSocket.open(port);
	m_TCPSocket.open(&addr);
	m_TCPSocket.set_non_blocking();
	m_TCPSocket.listen();
}

void CMasterServer::Shutdown()
{
	m_UDPSocket.close();
}

void CMasterServer::Tick()
{
	m_CurrentTime = time(NULL);
	
	ListenForHeartBeats();
	ListenForServerListPolls();

	CleanUpServerList();
}

void CMasterServer::ListenForHeartBeats()
{
	netaddr4 from;
	char data[1024];
	int dataSize;

	// read udp data while there is data to read :)
	while ((dataSize = m_UDPSocket.recv(&from, (char *)data, sizeof(data))) > 0)
	{
		// compare the received data size to the expected size
		if (dataSize == HEARTBEAT_SIZE)
		{
			char *d = data;
			int32 signature;
			d = ReadInt32(d, &signature);
		
			// make sure the signature is correct
			if (signature == HEARTBEAT_SIGNATURE)
			{
				CServerInfo info;
				info.Deserialize(d);

				from.port = 8303;
				info.SetAddress(&from);

				dbg_msg("got heartbeat", "IP: %i.%i.%i.%i:%i", (int)from.ip[0], (int)from.ip[1], (int)from.ip[2], (int)from.ip[3], from.port);

				dbg_msg("refresh", "okay. server count: %i", m_ServerCount);

				ProcessHeartBeat(info);	
			}
			else
			{}	// unexpected signature
		}
		else
		{}	// unknown data received
	}
}

void CMasterServer::ProcessHeartBeat(CServerInfo info)
{
	// find the corresponding server info
	CServerInfo *serverInfo = FindServerInfo(info.IP(), info.Port());

	// if it isn't in the list already, try to get an unused slot
	if (!serverInfo)
		serverInfo = GetUnusedSlot();

	// if we STILL don't have one, we're out of luck.
	if (!serverInfo)
		return;
	
	*serverInfo = info;
	serverInfo->Refresh(m_CurrentTime);

	// mark the server list packet as old
	m_ServerListPacketIsOld = true;
}

CServerInfo *CMasterServer::FindServerInfo(int32 ip, int32 port)
{
	// for now, just loop over the array
	for (int i = 0; i < m_ServerCount; i++)
	{
		CServerInfo *info = &m_Servers[i];
		if (info->IP() == ip && info->Port() == port)
			return info;
	}

	return 0x0;
}

CServerInfo *CMasterServer::GetUnusedSlot()
{
	if (m_ServerCount == MAXSERVERS)
		return 0x0;
	else
		return &m_Servers[m_ServerCount++];
}

void CMasterServer::CleanUpServerList()
{
	for (int i = 0; i < m_ServerCount; i++)
	{
		CServerInfo *serverInfo = &m_Servers[i];
		
		// check if it's time to remove it from the list
		if (serverInfo->LastRefresh() + HEARTBEAT_LIFETIME < m_CurrentTime)
		{
			if (i + 1 == m_ServerCount)
			{
				// if we're at the last one, just decrease m_ServerCount
				--m_ServerCount;
			}
			else
			{
				// otherwise, copy the last slot here and then decrease i and m_ServerCount
				*serverInfo = m_Servers[m_ServerCount - 1];
				--i;
				--m_ServerCount;
			}

			// mark the server list packet as old and outdated
			m_ServerListPacketIsOld = true;
		}
	}
}

void CMasterServer::ListenForServerListPolls()
{
	socket_tcp4 client;

	// accept clients while there are clients to be accepted
	while (m_TCPSocket.accept(&client))
	{
		// maybe we've prepared the packet already... it'd be silly to do it twice
		if (m_ServerListPacketIsOld)
		{
			BuildServerListPacket();
		}
		
		// send the server list and then close the socket
		client.send(m_ServerListPacket, m_ServerListPacketSize);
		client.close();
	}
}

void CMasterServer::BuildServerListPacket()
{
	char *d = m_ServerListPacket;

	d = WriteInt32(d, 'TWSL');
	d = WriteInt32(d, MASTERSERVER_VERSION);

	d = WriteInt32(d, m_ServerCount);
	
	for (int i = 0; i < m_ServerCount; i++)
	{
		CServerInfo *info = &m_Servers[i];
		d = info->Serialize(d);
	}

	m_ServerListPacketSize = d - m_ServerListPacket;
	m_ServerListPacketIsOld = false;
}
