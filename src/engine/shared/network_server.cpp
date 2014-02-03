/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/console.h>

#include "netban.h"
#include "network.h"


bool CNet::Open(const NETADDR *pBindAddr, CNetBan *pNetBan, int MaxClients, int Flags)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));

	m_Flags = Flags;

	// open socket
	m_Socket = net_udp_create(*pBindAddr);
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

	m_MaxClientsPerIP = NET_MAX_CLIENTS;

	for(int i = 0; i < NET_MAX_CLIENTS; i++)
		m_aSlots[i].m_Connection.Init(m_Socket, (bool)(m_Flags&NETFLAG_IGNOREPEERCLOSEMSG));

	return true;
}

int CNet::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUserData)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_UserData = pUserData;
	return 0;
}

int CNet::SetRecvRawCallback(NETFUNC_RECVRAW pfnRecvRaw, void *pUserData)
{
	m_pfnRecvRaw = pfnRecvRaw;
	m_RecvRawUserData = pUserData;
	return 0;
}

int CNet::Close()
{
	// TODO: implement me
	return 0;
}

int CNet::Connect(int ClientID, const NETADDR *pAddr)
{
	m_aSlots[ClientID].m_Connection.Connect((NETADDR *)pAddr);
	return 0;
}

int CNet::Disconnect(int ClientID, const char *pReason)
{
	// TODO: insert lots of checks here
	/*NETADDR Addr = ClientAddr(ClientID);

	dbg_msg("net_server", "client dropped. cid=%d ip=%d.%d.%d.%d reason=\"%s\"",
		ClientID,
		Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3],
		pReason
		);*/
	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, pReason, m_UserData);

	m_aSlots[ClientID].m_Connection.Disconnect(pReason);

	return 0;
}

int CNet::Update()
{
	int64 Now = time_get();
	for(int i = 0; i < MaxClients(); i++)
	{
		m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
		{
			if(Now - m_aSlots[i].m_Connection.ConnectTime() < time_freq() && NetBan())
				NetBan()->BanAddr(ClientAddr(i), 60, "Stressing network");
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
int CNet::Recv(CNetChunk *pChunk, TOKEN *pResponseToken)
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

		// callback
		if(m_pfnRecvRaw)
		{
			if(m_pfnRecvRaw(&Addr, m_RecvUnpacker.m_aBuffer, Bytes, m_RecvRawUserData))
				continue;
		}

		if(CNetBase::UnpackPacket(m_RecvUnpacker.m_aBuffer, Bytes, &m_RecvUnpacker.m_Data) == 0)
		{
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

			// no matching slot, check for bans
			char aBuf[128];
			if(NetBan() && NetBan()->IsBanned(&Addr, aBuf, sizeof(aBuf)))
			{
				// banned, reply with a message
				CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf)+1);
				continue;
			}

			int Accept = m_TokenManager.ProcessMessage(&Addr, &m_RecvUnpacker.m_Data, true);

			if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL)
			{
				if(!Accept)
					continue;
				if(m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_CONNECT)
				{
					if(!(m_Flags&NETFLAG_ACCEPTCONNS))
						continue;
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
								CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, aBuf, sizeof(aBuf));
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
							m_aSlots[i].m_Connection.SetToken(m_RecvUnpacker.m_Data.m_Token); // HACK!
							if(m_pfnNewClient)
								m_pfnNewClient(i, m_UserData);
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
					m_TokenCache.AddToken(&Addr, m_RecvUnpacker.m_Data.m_ResponseToken);
			}
			else if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS)
			{
				pChunk->m_Flags = NETSENDFLAG_CONNLESS;
				if(Accept < 0)
				{
					if(!(m_Flags&NETFLAG_ALLOWSTATELESS))
						continue;
					pChunk->m_Flags |= NETSENDFLAG_STATELESS;
				}
				pChunk->m_ClientID = -1;
				pChunk->m_Address = Addr;
				pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
				pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;
				if(pResponseToken)
					*pResponseToken = m_RecvUnpacker.m_Data.m_ResponseToken;
				return 1;
			}
			else
			{
				if(!Accept)
					continue;

				// TODO: check size here
				if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL && m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_CONNECT)
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
								CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, aBuf, sizeof(aBuf));
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
							m_aSlots[i].m_Connection.SetToken(m_RecvUnpacker.m_Data.m_Token); // HACK!
							if(m_pfnNewClient)
								m_pfnNewClient(i, m_UserData);
							break;
						}
					}

					if(!Found)
					{
						const char FullMsg[] = "This server is full";
						CNetBase::SendControlMsg(m_Socket, &Addr, m_RecvUnpacker.m_Data.m_ResponseToken, 0, NET_CTRLMSG_CLOSE, FullMsg, sizeof(FullMsg));
					}
				}
			}
		}
	}
	return 0;
}

int CNet::SendRaw(const NETADDR *pAddr, const void *pData, int DataSize)
{
	net_udp_send(m_Socket, pAddr, pData, DataSize);
	return 0;
}

int CNet::Send(CNetChunk *pChunk, TOKEN Token)
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
					pChunk->m_Flags &= ~NETSENDFLAG_STATELESS;
					pChunk->m_ClientID = i;
					break;
				}

		if(pChunk->m_Flags&NETSENDFLAG_STATELESS || Token != NET_TOKEN_NONE)
		{
			if(pChunk->m_Flags&NETSENDFLAG_STATELESS)
			{
				dbg_assert(pChunk->m_ClientID == -1, "errornous client id, connless packets can only be sent to cid=-1");
				dbg_assert(Token == NET_TOKEN_NONE, "stateless packets can't have a token");
			}
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

int CNet::GotProblems(int ClientID)
{
	if(time_get() - m_aSlots[ClientID].m_Connection.LastRecvTime() > time_freq())
		return 1;
	return 0;
}

int CNet::State(int ClientID)
{
	if(m_aSlots[ClientID].m_Connection.State() == NET_CONNSTATE_ONLINE)
		return NETSTATE_ONLINE;
	if(m_aSlots[ClientID].m_Connection.State() == NET_CONNSTATE_OFFLINE)
		return NETSTATE_OFFLINE;
	return NETSTATE_CONNECTING;
}

void CNet::SetMaxClientsPerIP(int Max)
{
	// clamp
	if(Max < 1)
		Max = 1;
	else if(Max > NET_MAX_CLIENTS)
		Max = NET_MAX_CLIENTS;

	m_MaxClientsPerIP = Max;
}
