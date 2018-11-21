/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/console.h>

#include "netban.h"
#include "network.h"


bool CNetServer::Open(NETADDR BindAddr, CNetBan *pNetBan, int MaxClients, int MaxClientsPerIP, int Flags)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));

	// open socket
	m_Socket = net_udp_create(BindAddr, 0);
	if(!m_Socket.type)
		return false;

	m_TokenManager.Init(m_Socket);
	m_TokenCache.Init(m_Socket, &m_TokenManager);

	m_pNetBan = pNetBan;

	// clamp clients
	m_MaxClients = MaxClients;
	if(m_MaxClients > NET_MAX_CLIENTS)
		m_MaxClients = NET_MAX_CLIENTS;
	if(m_MaxClients < 1)
		m_MaxClients = 1;

	m_MaxClientsPerIP = MaxClientsPerIP;

	for(int i = 0; i < NET_MAX_CLIENTS; i++)
		m_aSlots[i].m_Connection.Init(m_Socket, true);

	m_Flags = Flags;

	return true;
}

int CNetServer::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_UserPtr = pUser;
	return 0;
}

int CNetServer::Close()
{
	// TODO: implement me
	return 0;
}

int CNetServer::Drop(int ClientID, const char *pReason)
{
	// TODO: insert lots of checks here
	/*NETADDR Addr = ClientAddr(ClientID);

	dbg_msg("net_server", "client dropped. cid=%d ip=%d.%d.%d.%d reason=\"%s\"",
		ClientID,
		Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3],
		pReason
		);*/
	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, pReason, m_UserPtr);

	m_aSlots[ClientID].m_Connection.Disconnect(pReason);

	return 0;
}

int CNetServer::Update()
{
	int64 Now = time_get();
	for(int i = 0; i < MaxClients(); i++)
	{
		m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
		{
			if(Now - m_aSlots[i].m_Connection.ConnectTime() < time_freq() && NetBan())
			{
				if(NetBan()->BanAddr(ClientAddr(i), 60, "Stressing network") == -1)
					Drop(i, m_aSlots[i].m_Connection.ErrorString());
			}
			else
				Drop(i, m_aSlots[i].m_Connection.ErrorString());
		}
	}

	m_TokenManager.Update();
	m_TokenCache.Update();

	return 0;
}

/*
	TODO: chopp up this function into smaller working parts
*/
int CNetServer::Recv(CNetChunk *pChunk, TOKEN *pResponseToken)
{
	while(1)
	{
		NETADDR Addr;

		// check for a chunk
		if(m_RecvUnpacker.FetchChunk(pChunk))
			return 1;

		// TODO: empty the recvinfo
		int Bytes = net_udp_recv(m_Socket, &Addr, m_RecvUnpacker.m_aBuffer, NET_MAX_PACKETSIZE);

		// no more packets for now
		if(Bytes <= 0)
			break;

		if(CNetBase::UnpackPacket(m_RecvUnpacker.m_aBuffer, Bytes, &m_RecvUnpacker.m_Data) == 0)
		{
			// check for bans
			char aBuf[128];
			if(NetBan() && NetBan()->IsBanned(&Addr, aBuf, sizeof(aBuf)))
			{
				// banned, reply with a message
				CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf)+1);
				continue;
			}

			bool Found = false;
			// try to find matching slot
			for(int i = 0; i < MaxClients(); i++)
			{
				if(net_addr_comp(m_aSlots[i].m_Connection.PeerAddress(), &Addr) == 0)
				{
					if(m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr))
					{
						if(m_RecvUnpacker.m_Data.m_DataSize)
						{
							if(!(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS))
								m_RecvUnpacker.Start(&Addr, &m_aSlots[i].m_Connection, i);
							else
							{
								pChunk->m_Flags = NETSENDFLAG_CONNLESS;
								pChunk->m_Address = *m_aSlots[i].m_Connection.PeerAddress();
								pChunk->m_ClientID = i;
								pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
								pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;
								if(pResponseToken)
									*pResponseToken = NET_TOKEN_NONE;
								return 1;
							}
						}
					}
					Found = true;
				}
			}

			if(Found)
				continue;

			int Accept = m_TokenManager.ProcessMessage(&Addr, &m_RecvUnpacker.m_Data);
			if(Accept <= 0)
				continue;

			if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL)
			{
				if(m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_CONNECT)
				{
					bool Found = false;

					// only allow a specific number of players with the same ip
					NETADDR ThisAddr = Addr, OtherAddr;
					int FoundAddr = 1;
					ThisAddr.port = 0;
					for(int i = 0; i < MaxClients(); i++)
					{
						if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
							continue;

						OtherAddr = *m_aSlots[i].m_Connection.PeerAddress();
						OtherAddr.port = 0;
						if(!net_addr_comp(&ThisAddr, &OtherAddr))
						{
							if(FoundAddr++ >= m_MaxClientsPerIP)
							{
								char aBuf[128];
								str_format(aBuf, sizeof(aBuf), "Only %d players with the same IP are allowed", m_MaxClientsPerIP);
								CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf) + 1);
								return 0;
							}
						}
					}

					for(int i = 0; i < MaxClients(); i++)
					{
						if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
						{
							Found = true;
							m_aSlots[i].m_Connection.SetToken(m_RecvUnpacker.m_Data.m_Token);
							m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr);
							if(m_pfnNewClient)
								m_pfnNewClient(i, m_UserPtr);
							break;
						}
					}

					if(!Found)
					{
						const char FullMsg[] = "This server is full";
						CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, FullMsg, sizeof(FullMsg));
					}
				}
				else if(m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_TOKEN)
					m_TokenCache.AddToken(&Addr, m_RecvUnpacker.m_Data.m_ResponseToken, NET_TOKENFLAG_RESPONSEONLY);
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
	return 0;
}

int CNetServer::Send(CNetChunk *pChunk, TOKEN Token)
{
	if(pChunk->m_Flags&NETSENDFLAG_CONNLESS)
	{
		if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
		{
			dbg_msg("netserver", "packet payload too big. %d. dropping packet", pChunk->m_DataSize);
			return -1;
		}

		if(pChunk->m_ClientID == -1)
			for(int i = 0; i < MaxClients(); i++)
				if(net_addr_comp(&pChunk->m_Address, m_aSlots[i].m_Connection.PeerAddress()) == 0)
				{
					// upgrade the packet, now that we know its recipent
					pChunk->m_ClientID = i;
					break;
				}

		if(Token != NET_TOKEN_NONE)
		{
			CNetBase::SendPacketConnless(m_Socket, &pChunk->m_Address, Token, m_TokenManager.GenerateToken(&pChunk->m_Address), pChunk->m_pData, pChunk->m_DataSize);
		}
		else
		{
			if(pChunk->m_ClientID == -1)
			{
				m_TokenCache.SendPacketConnless(&pChunk->m_Address, pChunk->m_pData, pChunk->m_DataSize);
			}
			else
			{
				dbg_assert(pChunk->m_ClientID >= 0, "errornous client id");
				dbg_assert(pChunk->m_ClientID < MaxClients(), "errornous client id");

				m_aSlots[pChunk->m_ClientID].m_Connection.SendPacketConnless((const char *)pChunk->m_pData, pChunk->m_DataSize);
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
		dbg_assert(pChunk->m_ClientID >= 0, "errornous client id");
		dbg_assert(pChunk->m_ClientID < MaxClients(), "errornous client id");

		if(pChunk->m_Flags&NETSENDFLAG_VITAL)
			Flags = NET_CHUNKFLAG_VITAL;

		if(m_aSlots[pChunk->m_ClientID].m_Connection.QueueChunk(Flags, pChunk->m_DataSize, pChunk->m_pData) == 0)
		{
			if(pChunk->m_Flags&NETSENDFLAG_FLUSH)
				m_aSlots[pChunk->m_ClientID].m_Connection.Flush();
		}
		else
		{
			Drop(pChunk->m_ClientID, "Error sending data");
		}
	}
	return 0;
}

void CNetServer::SetMaxClientsPerIP(int Max)
{
	// clamp
	if(Max < 1)
		Max = 1;
	else if(Max > NET_MAX_CLIENTS)
		Max = NET_MAX_CLIENTS;

	m_MaxClientsPerIP = Max;
}
