/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <stdlib.h> // rand

#include <base/system.h>

#include <engine/external/md5/md5.h>

#include "network.h"

static unsigned int Hash(char *pData, int Size)
{
	md5_state_t State;
	unsigned int aDigest[4]; // make sure this is 16 byte

	md5_init(&State);
	md5_append(&State, (const md5_byte_t *)pData, Size);
	md5_finish(&State, (md5_byte_t *)aDigest);

	return (aDigest[0] ^ aDigest[1] ^ aDigest[2] ^ aDigest[3]);
}

void CNetTokenManager::Init(NETSOCKET Socket, int SeedTime)
{
	m_Socket = Socket;
	m_SeedTime = SeedTime;
	GenerateSeed();
}

void CNetTokenManager::Update()
{
	if(time_get() > m_NextSeedTime)
		GenerateSeed();
}

int CNetTokenManager::ProcessMessage(const NETADDR *pAddr, const CNetPacketConstruct *pPacket, bool Notify)
{
	if(pPacket->m_Token != NET_TOKEN_NONE
		&& !CheckToken(pAddr, pPacket->m_Token, pPacket->m_ResponseToken, Notify))
		return 0; // wrong token, silent ignore

	bool Verified = pPacket->m_Token != NET_TOKEN_NONE;
	bool TokenMessage = (pPacket->m_Flags & NET_PACKETFLAG_CONTROL)
		&& pPacket->m_aChunkData[0] == NET_CTRLMSG_TOKEN;

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		return (Verified) ? 1 : -1; // connless packets without token are allowed

	if(!TokenMessage)
	{
		if(Verified)
			return 1; // verified packet
		else
			// the only allowed not connless packet
			// without token is NET_CTRLMSG_TOKEN
			return 0;
	}

	if(Verified && TokenMessage)
		return 1; // everything is fine, token exchange complete

	// client requesting token
	CNetBase::SendControlMsgWithToken(m_Socket, (NETADDR *)pAddr,
		pPacket->m_ResponseToken, 0, NET_CTRLMSG_TOKEN,
		GenerateToken(pAddr));
	return 0; // no need to process NET_CTRLMSG_TOKEN further
}

void CNetTokenManager::GenerateSeed()
{
	static const NETADDR NullAddr = { 0 };
	m_PrevSeed = m_Seed;

	for(int i = 0; i < 4; i++)
	{
		// rand() returns a random integer between 0 and RAND_MAX
		// RAND_MAX is at least 1<<16 according to the standard
		m_Seed <<= 16;
		m_Seed ^= rand();
	}

	m_PrevGlobalToken = m_GlobalToken;
	m_GlobalToken = GenerateToken(&NullAddr);

	m_NextSeedTime = time_get() + time_freq() * m_SeedTime;
}

TOKEN CNetTokenManager::GenerateToken(const NETADDR *pAddr) const
{
	return GenerateToken(pAddr, m_Seed);
}

TOKEN CNetTokenManager::GenerateToken(const NETADDR *pAddr, int64 Seed)
{
	static const NETADDR NullAddr = { 0 };
	NETADDR Addr;
	char aBuf[sizeof(NETADDR) + sizeof(int64)];
	int Result;

	if(pAddr->type & NETTYPE_LINK_BROADCAST)
		return GenerateToken(&NullAddr, Seed);

	Addr = *pAddr;
	Addr.port = 0;
	mem_copy(aBuf, &Addr, sizeof(NETADDR));
	mem_copy(aBuf + sizeof(NETADDR), &Seed, sizeof(int64));

	Result = Hash(aBuf, sizeof(aBuf)) & NET_TOKEN_MASK;
	if(Result == NET_TOKEN_NONE)
		Result--;

	return Result;
}

bool CNetTokenManager::CheckToken(const NETADDR *pAddr, TOKEN Token, TOKEN ResponseToken, bool Notify)
{
	TOKEN CurrentToken = GenerateToken(pAddr, m_Seed);
	if(CurrentToken == Token)
		return true;

	if(GenerateToken(pAddr, m_PrevSeed) == Token)
	{
		if(Notify)
			CNetBase::SendControlMsgWithToken(m_Socket, (NETADDR *)pAddr,
				ResponseToken, 0, NET_CTRLMSG_TOKEN, CurrentToken);
				// notify the peer about the new token
		return true;
	}
	else if(Token == m_GlobalToken)
		return true;
	else if(Token == m_PrevGlobalToken)
	{
		if(Notify)
			CNetBase::SendControlMsgWithToken(m_Socket, (NETADDR *)pAddr,
				ResponseToken, 0, NET_CTRLMSG_TOKEN, m_GlobalToken);
				// notify the peer about the new token
		return true;
	}

	return false;
}


CNetTokenCache::CNetTokenCache()
{
	m_pTokenManager = 0;
	m_pConnlessPacketList = 0;
}

CNetTokenCache::~CNetTokenCache()
{
	// delete the linked list
	while(m_pConnlessPacketList)
	{
		CConnlessPacketInfo *pTemp = m_pConnlessPacketList->m_pNext;
		delete m_pConnlessPacketList;
		m_pConnlessPacketList = pTemp;
	}
}

void CNetTokenCache::Init(NETSOCKET Socket, const CNetTokenManager *pTokenManager)
{
	// call the destructor to clear the linked list
	this->~CNetTokenCache();

	m_TokenCache.Init();
	m_Socket = Socket;
	m_pTokenManager = pTokenManager;
}

void CNetTokenCache::SendPacketConnless(const NETADDR *pAddr, const void *pData, int DataSize)
{
	TOKEN Token = GetToken(pAddr);
	if(Token != NET_TOKEN_NONE)
	{
		CNetBase::SendPacketConnless(m_Socket, pAddr, Token,
			m_pTokenManager->GenerateToken(pAddr), pData, DataSize);
	}
	else
	{
		FetchToken(pAddr);

		// store the packet for future sending
		CConnlessPacketInfo **ppInfo = &m_pConnlessPacketList;
		while(*ppInfo)
			ppInfo = &(*ppInfo)->m_pNext;
		*ppInfo = new CConnlessPacketInfo();
		mem_copy((*ppInfo)->m_aData, pData, DataSize);
		(*ppInfo)->m_Addr = *pAddr;
		(*ppInfo)->m_DataSize = DataSize;
		(*ppInfo)->m_Expiry = time_get() + time_freq() * NET_TOKENCACHE_PACKETEXPIRY;
	}
}

TOKEN CNetTokenCache::GetToken(const NETADDR *pAddr)
{
	// traverse the list in the reverse direction
	// newest caches are the best
	CAddressInfo *pInfo = m_TokenCache.Last();
	while(pInfo)
	{
		if(net_addr_comp(&pInfo->m_Addr, pAddr) == 0)
			return pInfo->m_Token;
		pInfo = m_TokenCache.Prev(pInfo);
	}
	return NET_TOKEN_NONE;
}

void CNetTokenCache::FetchToken(const NETADDR *pAddr)
{
	CNetBase::SendControlMsgWithToken(m_Socket, pAddr, NET_TOKEN_NONE, 0, 
		NET_CTRLMSG_TOKEN, m_pTokenManager->GenerateToken(pAddr));
}

void CNetTokenCache::AddToken(const NETADDR *pAddr, TOKEN Token)
{
	if(Token == NET_TOKEN_NONE)
		return;

	CAddressInfo Info;
	Info.m_Addr = *pAddr;
	Info.m_Token = Token;
	Info.m_Expiry = time_get() + time_freq() * NET_TOKENCACHE_ADDRESSEXPIRY;

	(*m_TokenCache.Allocate(sizeof(Info))) = Info;

	// search the list of packets to be sent
	// for this address
	CConnlessPacketInfo **ppPrevNext = &m_pConnlessPacketList;
		// pointer to the next element pointer of the previous element
	CConnlessPacketInfo *pInfo = m_pConnlessPacketList;
	while(pInfo)
	{
		if(net_addr_comp(&pInfo->m_Addr, pAddr) == 0)
		{
			CNetBase::SendPacketConnless(m_Socket, pAddr, Token,
				m_pTokenManager->GenerateToken(pAddr),
				pInfo->m_aData, pInfo->m_DataSize);
			*ppPrevNext = pInfo->m_pNext;
			delete pInfo;
			pInfo = *ppPrevNext;
		}
		else
			pInfo = pInfo->m_pNext;
	}
}

void CNetTokenCache::Update()
{
	int64 Now = time_get();

	// drop expired address info
	CAddressInfo *pAddrInfo;
	while((pAddrInfo = m_TokenCache.First()) && (pAddrInfo->m_Expiry <= Now))
		m_TokenCache.PopFirst();


	// drop expired packets
	while(m_pConnlessPacketList && m_pConnlessPacketList->m_Expiry <= Now)
	{
		CConnlessPacketInfo *pNewList;
		pNewList = m_pConnlessPacketList->m_pNext;
		delete m_pConnlessPacketList;
		m_pConnlessPacketList = pNewList;
	}
}

