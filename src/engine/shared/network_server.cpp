/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "network.h"

#define MACRO_LIST_LINK_FIRST(Object, First, Prev, Next) \
	{ if(First) First->Prev = Object; \
	Object->Prev = (struct CBan *)0; \
	Object->Next = First; \
	First = Object; }

#define MACRO_LIST_LINK_AFTER(Object, After, Prev, Next) \
	{ Object->Prev = After; \
	Object->Next = After->Next; \
	After->Next = Object; \
	if(Object->Next) \
		Object->Next->Prev = Object; \
	}

#define MACRO_LIST_UNLINK(Object, First, Prev, Next) \
	{ if(Object->Next) Object->Next->Prev = Object->Prev; \
	if(Object->Prev) Object->Prev->Next = Object->Next; \
	else First = Object->Next; \
	Object->Next = 0; Object->Prev = 0; }

#define MACRO_LIST_FIND(Start, Next, Expression) \
	{ while(Start && !(Expression)) Start = Start->Next; }

bool CNetServer::Open(NETADDR BindAddr, int MaxClients, int MaxClientsPerIP, int Flags)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));

	// open socket
	m_Socket = net_udp_create(BindAddr);
	if(!m_Socket.type)
		return false;

	// clamp clients
	m_MaxClients = MaxClients;
	if(m_MaxClients > NET_MAX_CLIENTS)
		m_MaxClients = NET_MAX_CLIENTS;
	if(m_MaxClients < 1)
		m_MaxClients = 1;

	m_MaxClientsPerIP = MaxClientsPerIP;

	for(int i = 0; i < NET_MAX_CLIENTS; i++)
		m_aSlots[i].m_Connection.Init(m_Socket);

	// setup all pointers for bans
	for(int i = 1; i < NET_SERVER_MAXBANS-1; i++)
	{
		m_BanPool[i].m_pNext = &m_BanPool[i+1];
		m_BanPool[i].m_pPrev = &m_BanPool[i-1];
	}

	m_BanPool[0].m_pNext = &m_BanPool[1];
	m_BanPool[NET_SERVER_MAXBANS-1].m_pPrev = &m_BanPool[NET_SERVER_MAXBANS-2];
	m_BanPool_FirstFree = &m_BanPool[0];

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

int CNetServer::BanGet(int Index, CBanInfo *pInfo)
{
	CBan *pBan;
	for(pBan = m_BanPool_FirstUsed; pBan && Index; pBan = pBan->m_pNext, Index--)
		{}

	if(!pBan)
		return 0;
	*pInfo = pBan->m_Info;
	return 1;
}

int CNetServer::BanNum()
{
	int Count = 0;
	CBan *pBan;
	for(pBan = m_BanPool_FirstUsed; pBan; pBan = pBan->m_pNext)
		Count++;
	return Count;
}

void CNetServer::BanRemoveByObject(CBan *pBan)
{
	int IpHash = (pBan->m_Info.m_Addr.ip[0]+pBan->m_Info.m_Addr.ip[1]+pBan->m_Info.m_Addr.ip[2]+pBan->m_Info.m_Addr.ip[3]+
					pBan->m_Info.m_Addr.ip[4]+pBan->m_Info.m_Addr.ip[5]+pBan->m_Info.m_Addr.ip[6]+pBan->m_Info.m_Addr.ip[7]+
					pBan->m_Info.m_Addr.ip[8]+pBan->m_Info.m_Addr.ip[9]+pBan->m_Info.m_Addr.ip[10]+pBan->m_Info.m_Addr.ip[11]+
					pBan->m_Info.m_Addr.ip[12]+pBan->m_Info.m_Addr.ip[13]+pBan->m_Info.m_Addr.ip[14]+pBan->m_Info.m_Addr.ip[15])&0xff;
	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&pBan->m_Info.m_Addr, aAddrStr, sizeof(aAddrStr));
	dbg_msg("netserver", "removing ban on %s", aAddrStr);
	MACRO_LIST_UNLINK(pBan, m_BanPool_FirstUsed, m_pPrev, m_pNext);
	MACRO_LIST_UNLINK(pBan, m_aBans[IpHash], m_pHashPrev, m_pHashNext);
	MACRO_LIST_LINK_FIRST(pBan, m_BanPool_FirstFree, m_pPrev, m_pNext);
}

int CNetServer::BanRemove(NETADDR Addr)
{
	int IpHash = (Addr.ip[0]+Addr.ip[1]+Addr.ip[2]+Addr.ip[3]+Addr.ip[4]+Addr.ip[5]+Addr.ip[6]+Addr.ip[7]+
					Addr.ip[8]+Addr.ip[9]+Addr.ip[10]+Addr.ip[11]+Addr.ip[12]+Addr.ip[13]+Addr.ip[14]+Addr.ip[15])&0xff;
	CBan *pBan = m_aBans[IpHash];

	MACRO_LIST_FIND(pBan, m_pHashNext, net_addr_comp(&pBan->m_Info.m_Addr, &Addr) == 0);

	if(pBan)
	{
		BanRemoveByObject(pBan);
		return 0;
	}

	return -1;
}

int CNetServer::BanAdd(NETADDR Addr, int Seconds, const char *pReason)
{
	int IpHash = (Addr.ip[0]+Addr.ip[1]+Addr.ip[2]+Addr.ip[3]+Addr.ip[4]+Addr.ip[5]+Addr.ip[6]+Addr.ip[7]+
					Addr.ip[8]+Addr.ip[9]+Addr.ip[10]+Addr.ip[11]+Addr.ip[12]+Addr.ip[13]+Addr.ip[14]+Addr.ip[15])&0xff;
	int Stamp = -1;
	CBan *pBan;

	// remove the port
	Addr.port = 0;

	if(Seconds)
		Stamp = time_timestamp() + Seconds;

	// search to see if it already exists
	pBan = m_aBans[IpHash];
	MACRO_LIST_FIND(pBan, m_pHashNext, net_addr_comp(&pBan->m_Info.m_Addr, &Addr) == 0);
	if(pBan)
	{
		// adjust the ban
		pBan->m_Info.m_Expires = Stamp;
		return 0;
	}

	if(!m_BanPool_FirstFree)
		return -1;

	// fetch and clear the new ban
	pBan = m_BanPool_FirstFree;
	MACRO_LIST_UNLINK(pBan, m_BanPool_FirstFree, m_pPrev, m_pNext);

	// setup the ban info
	pBan->m_Info.m_Expires = Stamp;
	pBan->m_Info.m_Addr = Addr;
	str_copy(pBan->m_Info.m_Reason, pReason, sizeof(pBan->m_Info.m_Reason));

	// add it to the ban hash
	MACRO_LIST_LINK_FIRST(pBan, m_aBans[IpHash], m_pHashPrev, m_pHashNext);

	// insert it into the used list
	{
		if(m_BanPool_FirstUsed)
		{
			CBan *pInsertAfter = m_BanPool_FirstUsed;
			MACRO_LIST_FIND(pInsertAfter, m_pNext, Stamp < pInsertAfter->m_Info.m_Expires);

			if(pInsertAfter)
				pInsertAfter = pInsertAfter->m_pPrev;
			else
			{
				// add to last
				pInsertAfter = m_BanPool_FirstUsed;
				while(pInsertAfter->m_pNext)
					pInsertAfter = pInsertAfter->m_pNext;
			}

			if(pInsertAfter)
			{
				MACRO_LIST_LINK_AFTER(pBan, pInsertAfter, m_pPrev, m_pNext);
			}
			else
			{
				MACRO_LIST_LINK_FIRST(pBan, m_BanPool_FirstUsed, m_pPrev, m_pNext);
			}
		}
		else
		{
			MACRO_LIST_LINK_FIRST(pBan, m_BanPool_FirstUsed, m_pPrev, m_pNext);
		}
	}

	// drop banned clients
	{
		char Buf[128];
		NETADDR BanAddr;

		int Mins = (Seconds + 59) / 60;
		if(Mins)
		{
			if(Mins == 1)
				str_format(Buf, sizeof(Buf), "You have been banned for 1 minute (%s)", pReason);
			else
				str_format(Buf, sizeof(Buf), "You have been banned for %d minutes (%s)", Mins, pReason);
		}
		else
			str_format(Buf, sizeof(Buf), "You have been banned for life (%s)", pReason);

		for(int i = 0; i < MaxClients(); i++)
		{
			BanAddr = m_aSlots[i].m_Connection.PeerAddress();
			BanAddr.port = 0;

			if(net_addr_comp(&Addr, &BanAddr) == 0)
				Drop(i, Buf);
		}
	}
	return 0;
}

int CNetServer::Update()
{
	int Now = time_timestamp();
	for(int i = 0; i < MaxClients(); i++)
	{
		m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
			Drop(i, m_aSlots[i].m_Connection.ErrorString());
	}

	// remove expired bans
	while(m_BanPool_FirstUsed && m_BanPool_FirstUsed->m_Info.m_Expires < Now)
	{
		CBan *pBan = m_BanPool_FirstUsed;
		BanRemoveByObject(pBan);
	}

	return 0;
}

/*
	TODO: chopp up this function into smaller working parts
*/
int CNetServer::Recv(CNetChunk *pChunk)
{
	unsigned Now = time_timestamp();

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
			CBan *pBan = 0;
			NETADDR BanAddr = Addr;
			int IpHash = (BanAddr.ip[0]+BanAddr.ip[1]+BanAddr.ip[2]+BanAddr.ip[3]+BanAddr.ip[4]+BanAddr.ip[5]+BanAddr.ip[6]+BanAddr.ip[7]+
							BanAddr.ip[8]+BanAddr.ip[9]+BanAddr.ip[10]+BanAddr.ip[11]+BanAddr.ip[12]+BanAddr.ip[13]+BanAddr.ip[14]+BanAddr.ip[15])&0xff;
			int Found = 0;
			BanAddr.port = 0;

			// search a ban
			for(pBan = m_aBans[IpHash]; pBan; pBan = pBan->m_pHashNext)
			{
				if(net_addr_comp(&pBan->m_Info.m_Addr, &BanAddr) == 0)
					break;
			}

			// check if we just should drop the packet
			if(pBan)
			{
				// banned, reply with a message
				char BanStr[128];
				if(pBan->m_Info.m_Expires)
				{
					int Mins = ((pBan->m_Info.m_Expires - Now)+59)/60;
					if(Mins == 1)
						str_format(BanStr, sizeof(BanStr), "Banned for 1 minute (%s)", pBan->m_Info.m_Reason);
					else
						str_format(BanStr, sizeof(BanStr), "Banned for %d minutes (%s)", Mins, pBan->m_Info.m_Reason);
				}
				else
					str_format(BanStr, sizeof(BanStr), "Banned for life (%s)", pBan->m_Info.m_Reason);
				CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, BanStr, str_length(BanStr)+1);
				continue;
			}

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
				// TODO: check size here
				if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL && m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_CONNECT)
				{
					Found = 0;

					// check if we already got this client
					for(int i = 0; i < MaxClients(); i++)
					{
						NETADDR PeerAddr = m_aSlots[i].m_Connection.PeerAddress();
						if(m_aSlots[i].m_Connection.State() != NET_CONNSTATE_OFFLINE &&
							net_addr_comp(&PeerAddr, &Addr) == 0)
						{
							Found = 1; // silent ignore.. we got this client already
							break;
						}
					}

					// client that wants to connect
					if(!Found)
					{
						// only allow a specific number of players with the same ip
						NETADDR ThisAddr = Addr, OtherAddr;
						int FoundAddr = 1;
						ThisAddr.port = 0;
						for(int i = 0; i < MaxClients(); ++i)
						{
							if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
								continue;

							OtherAddr = m_aSlots[i].m_Connection.PeerAddress();
							OtherAddr.port = 0;
							if(!net_addr_comp(&ThisAddr, &OtherAddr))
							{
								if(FoundAddr++ >= m_MaxClientsPerIP)
								{
									char aBuf[128];
									str_format(aBuf, sizeof(aBuf), "Only %d players with the same IP are allowed", m_MaxClientsPerIP);
									CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aBuf, sizeof(aBuf));
									return 0;
								}
							}
						}

						for(int i = 0; i < MaxClients(); i++)
						{
							if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
							{
								Found = 1;
								m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr);
								if(m_pfnNewClient)
									m_pfnNewClient(i, m_UserPtr);
								break;
							}
						}

						if(!Found)
						{
							const char FullMsg[] = "This server is full";
							CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, FullMsg, sizeof(FullMsg));
						}
					}
				}
				else
				{
					// normal packet, find matching slot
					for(int i = 0; i < MaxClients(); i++)
					{
						NETADDR PeerAddr = m_aSlots[i].m_Connection.PeerAddress();
						if(net_addr_comp(&PeerAddr, &Addr) == 0)
						{
							if(m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr))
							{
								if(m_RecvUnpacker.m_Data.m_DataSize)
									m_RecvUnpacker.Start(&Addr, &m_aSlots[i].m_Connection, i);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int CNetServer::Send(CNetChunk *pChunk)
{
	if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netserver", "packet payload too big. %d. dropping packet", pChunk->m_DataSize);
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
