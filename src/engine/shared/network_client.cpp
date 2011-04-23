/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "network.h"

bool CNetClient::Open(NETADDR BindAddr, int Flags)
{
	// open socket
	NETSOCKET Socket;
	Socket = net_udp_create(BindAddr);
	if(!Socket.type)
		return false;

	// clean it
	mem_zero(this, sizeof(*this));

	// init
	m_Socket = Socket;
	m_Connection.Init(m_Socket);
	return true;
}

int CNetClient::Close()
{
	// TODO: implement me
	return 0;
}


int CNetClient::Disconnect(const char *pReason)
{
	//dbg_msg("netclient", "disconnected. reason=\"%s\"", pReason);
	m_Connection.Disconnect(pReason);
	return 0;
}

int CNetClient::Update()
{
	m_Connection.Update();
	if(m_Connection.State() == NET_CONNSTATE_ERROR)
		Disconnect(m_Connection.ErrorString());
	return 0;
}

int CNetClient::Connect(NETADDR *pAddr)
{
	m_Connection.Connect(pAddr);
	return 0;
}

int CNetClient::ResetErrorString()
{
	m_Connection.ResetErrorString();
	return 0;
}

int CNetClient::Recv(CNetChunk *pChunk)
{
	while(1)
	{
		// check for a chunk
		if(m_RecvUnpacker.FetchChunk(pChunk))
			return 1;

		// TODO: empty the recvinfo
		NETADDR Addr;
		int Bytes = net_udp_recv(m_Socket, &Addr, m_RecvUnpacker.m_aBuffer, NET_MAX_PACKETSIZE);

		// no more packets for now
		if(Bytes <= 0)
			break;

		if(CNetBase::UnpackPacket(m_RecvUnpacker.m_aBuffer, Bytes, &m_RecvUnpacker.m_Data) == 0)
		{
			if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS)
			{
				pChunk->m_Flags = NETSENDFLAG_CONNLESS;
				pChunk->m_ClientID = -1;
				pChunk->m_Address = Addr;
				pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
				pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;
				return 1;
			}
			else
			{
				if(m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr))
					m_RecvUnpacker.Start(&Addr, &m_Connection, 0);
			}
		}
	}
	return 0;
}

int CNetClient::Send(CNetChunk *pChunk)
{
	if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netclient", "chunk payload too big. %d. dropping chunk", pChunk->m_DataSize);
		return -1;
	}

	if(pChunk->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// send connectionless packet
		CNetBase::SendPacketConnless(m_Socket, &pChunk->m_Address, pChunk->m_pData, pChunk->m_DataSize);
	}
	else
	{
		int Flags = 0;
		dbg_assert(pChunk->m_ClientID == 0, "errornous client id");

		if(pChunk->m_Flags&NETSENDFLAG_VITAL)
			Flags = NET_CHUNKFLAG_VITAL;

		m_Connection.QueueChunk(Flags, pChunk->m_DataSize, pChunk->m_pData);

		if(pChunk->m_Flags&NETSENDFLAG_FLUSH)
			m_Connection.Flush();
	}
	return 0;
}

int CNetClient::State()
{
	if(m_Connection.State() == NET_CONNSTATE_ONLINE)
		return NETSTATE_ONLINE;
	if(m_Connection.State() == NET_CONNSTATE_OFFLINE)
		return NETSTATE_OFFLINE;
	return NETSTATE_CONNECTING;
}

int CNetClient::Flush()
{
	return m_Connection.Flush();
}

int CNetClient::GotProblems()
{
	if(time_get() - m_Connection.LastRecvTime() > time_freq())
		return 1;
	return 0;
}

const char *CNetClient::ErrorString()
{
	return m_Connection.ErrorString();
}
