/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "network.h"

bool CNetClient::Open(NETADDR BindAddr, int Flags)
{
	// open socket
	NETSOCKET Socket;
	Socket = net_udp_create(BindAddr, (Flags&NETCREATE_FLAG_RANDOMPORT) ? 1 : 0);
	if(!Socket.type)
		return false;

	// clean it
	mem_zero(this, sizeof(*this));

	// init
	m_Socket = Socket;
	m_Connection.Init(m_Socket, false);

	m_TokenManager.Init(Socket);
	m_TokenCache.Init(Socket, &m_TokenManager);

	m_Flags = Flags;

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
	m_TokenManager.Update();
	m_TokenCache.Update();
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

int CNetClient::Recv(CNetChunk *pChunk, TOKEN *pResponseToken)
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
			if(m_Connection.State() != NET_CONNSTATE_OFFLINE && m_Connection.State() != NET_CONNSTATE_ERROR && net_addr_comp(m_Connection.PeerAddress(), &Addr) == 0)
			{
				if(m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr))
				{
					if(!(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS))
						m_RecvUnpacker.Start(&Addr, &m_Connection, 0);
				}
			}
			else
			{
				int Accept = m_TokenManager.ProcessMessage(&Addr, &m_RecvUnpacker.m_Data, true);
				if(!Accept)
					continue;

				if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL)
				{
					if(m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_TOKEN)
						m_TokenCache.AddToken(&Addr, m_RecvUnpacker.m_Data.m_ResponseToken);
				}
				else if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS)
				{
					pChunk->m_Flags = NETSENDFLAG_CONNLESS;
					pChunk->m_ClientID = -1;
					pChunk->m_Address = Addr;
					pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
					pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;

					if(pResponseToken)
						*pResponseToken = m_RecvUnpacker.m_Data.m_ResponseToken;
					return 1;
				}
			}
		}
	}
	return 0;
}

int CNetClient::Send(CNetChunk *pChunk, TOKEN Token, CSendCBData *pCallbackData)
{
	if(pChunk->m_Flags&NETSENDFLAG_CONNLESS)
	{
		if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
		{
			dbg_msg("netserver", "packet payload too big. %d. dropping packet", pChunk->m_DataSize);
			return -1;
		}

		if(pChunk->m_ClientID == -1 && net_addr_comp(&pChunk->m_Address, m_Connection.PeerAddress()) == 0)
		{
			// upgrade the packet, now that we know its recipent
			pChunk->m_ClientID = 0;
		}


		if(Token != NET_TOKEN_NONE)
		{
			CNetBase::SendPacketConnless(m_Socket, &pChunk->m_Address, Token, m_TokenManager.GenerateToken(&pChunk->m_Address), pChunk->m_pData, pChunk->m_DataSize);
		}
		else
		{
			if(pChunk->m_ClientID == -1)
			{
				m_TokenCache.SendPacketConnless(&pChunk->m_Address, pChunk->m_pData, pChunk->m_DataSize, pCallbackData);
			}
			else
			{
				dbg_assert(pChunk->m_ClientID == 0, "errornous client id");
				m_Connection.SendPacketConnless((const char *)pChunk->m_pData, pChunk->m_DataSize);
			}
		}
	}
	else
	{
		if(pChunk->m_DataSize+NET_MAX_CHUNKHEADERSIZE >= NET_MAX_PAYLOAD)
		{
			dbg_msg("netclient", "chunk payload too big. %d. dropping chunk", pChunk->m_DataSize);
			return -1;
		}

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

void CNetClient::PurgeStoredPacket(int TrackID)
{
	m_TokenCache.PurgeStoredPacket(TrackID);
}

int CNetClient::State() const
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

bool CNetClient::GotProblems() const
{
	return (time_get() - m_Connection.LastRecvTime() > time_freq());
}

const char *CNetClient::ErrorString() const
{
	return m_Connection.ErrorString();
}

