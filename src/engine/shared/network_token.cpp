/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/hash_ctxt.h>
#include <base/math.h>
#include <base/system.h>

#include "network.h"

static unsigned int Hash(char *pData, int Size)
{
	unsigned aDigest[4];
	MD5_DIGEST Digest = md5(pData, Size);
	for(int i = 0; i < 4; i++)
		aDigest[i] = bytes_be_to_uint(&Digest.data[i * 4]);

	return (aDigest[0] ^ aDigest[1] ^ aDigest[2] ^ aDigest[3]);
}

int CNetTokenCache::CConnlessPacketInfo::m_UniqueID = 0;

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

int CNetTokenManager::ProcessMessage(const NETADDR *pAddr, const CNetPacketConstruct *pPacket)
{
	bool BroadcastResponse = false;
	if(pPacket->m_Token != NET_TOKEN_NONE
		&& !CheckToken(pAddr, pPacket->m_Token, pPacket->m_ResponseToken, &BroadcastResponse))
		return 0; // wrong token, silent ignore

	bool Verified = pPacket->m_Token != NET_TOKEN_NONE;
	bool TokenMessage = (pPacket->m_Flags & NET_PACKETFLAG_CONTROL)
		&& pPacket->m_aChunkData[0] == NET_CTRLMSG_TOKEN;

	if(pPacket->m_Flags&NET_PACKETFLAG_CONNLESS)
		return (Verified && !BroadcastResponse) ? 1 : 0; // connless packets without token are not allowed

	if(!TokenMessage)
	{
		if(Verified && !BroadcastResponse)
			return 1; // verified packet
		else
			// the only allowed not connless packet
			// without token is NET_CTRLMSG_TOKEN
			return 0;
	}

	if(Verified && TokenMessage)
		return BroadcastResponse ? -1 : 1; // everything is fine, token exchange complete

	// client requesting token
	if(pPacket->m_DataSize >= NET_TOKENREQUEST_DATASIZE)
	{
		CNetBase::SendControlMsgWithToken(m_Socket, (NETADDR *)pAddr,
			pPacket->m_ResponseToken, 0, NET_CTRLMSG_TOKEN,
			GenerateToken(pAddr), false);
	}
	return 0; // no need to process NET_CTRLMSG_TOKEN further
}

void CNetTokenManager::GenerateSeed()
{
	static const NETADDR NullAddr = { 0 };
	m_PrevSeed = m_Seed;

	secure_random_fill(&m_Seed, sizeof(m_Seed));

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
	unsigned int Result;

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

bool CNetTokenManager::CheckToken(const NETADDR *pAddr, TOKEN Token, TOKEN ResponseToken, bool *BroadcastResponse)
{
	TOKEN CurrentToken = GenerateToken(pAddr, m_Seed);
	if(CurrentToken == Token)
		return true;

	if(GenerateToken(pAddr, m_PrevSeed) == Token)
	{
		// no need to notify the peer, just a one time thing
		return true;
	}
	else if(Token == m_GlobalToken)
	{
		*BroadcastResponse = true;
		return true;
	}
	else if(Token == m_PrevGlobalToken)
	{
		// no need to notify the peer, just a broadcast token response
		*BroadcastResponse = true;
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
		m_pConnlessPacketList = 0;
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

void CNetTokenCache::SendPacketConnless(const NETADDR *pAddr, const void *pData, int DataSize, CSendCBData *pCallbackData)
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
		int64 Now = time_get();
		(*ppInfo)->m_Expiry = Now + time_freq() * NET_TOKENCACHE_PACKETEXPIRY;
		(*ppInfo)->m_LastTokenRequest = Now;
		(*ppInfo)->m_pNext = 0;
		if(pCallbackData)
		{
			(*ppInfo)->m_pfnCallback = pCallbackData->m_pfnCallback;
			(*ppInfo)->m_pCallbackUser = pCallbackData->m_pCallbackUser;
			pCallbackData->m_TrackID = (*ppInfo)->m_TrackID;
		}
		else
		{
			(*ppInfo)->m_pfnCallback = 0;
			(*ppInfo)->m_pCallbackUser = 0;
		}
	}
}

void CNetTokenCache::PurgeStoredPacket(int TrackID)
{
	CConnlessPacketInfo *pPrevInfo = 0;
	CConnlessPacketInfo *pInfo = m_pConnlessPacketList;
	while(pInfo)
	{
		if(pInfo->m_TrackID == TrackID)
		{
			// purge desired packet
			CConnlessPacketInfo *pNext = pInfo->m_pNext;
			if(pPrevInfo)
				pPrevInfo->m_pNext = pNext;
			if(pInfo == m_pConnlessPacketList)
				m_pConnlessPacketList = pNext;
			delete pInfo;

			break;
		}
		else
		{
			if(pPrevInfo)
				pPrevInfo = pPrevInfo->m_pNext;
			else
				pPrevInfo = pInfo;
			pInfo = pInfo->m_pNext;
		}
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
		NET_CTRLMSG_TOKEN, m_pTokenManager->GenerateToken(pAddr), true);
}

void CNetTokenCache::AddToken(const NETADDR *pAddr, TOKEN Token, int TokenFLag)
{
	if(Token == NET_TOKEN_NONE)
		return;

	// search the list of packets to be sent
	// for this address
	CConnlessPacketInfo *pPrevInfo = 0;
	CConnlessPacketInfo *pInfo = m_pConnlessPacketList;
	bool Found = false;
	while(pInfo)
	{
		static NETADDR NullAddr = { 0 };
		NullAddr.type = 7;	// cover broadcasts
		NullAddr.port = pAddr->port;
		if(net_addr_comp(&pInfo->m_Addr, pAddr) == 0 || ((TokenFLag&NET_TOKENFLAG_ALLOWBROADCAST) && net_addr_comp(&pInfo->m_Addr, &NullAddr) == 0))
		{
			// notify the user that the packet gets delivered
			if(pInfo->m_pfnCallback)
				pInfo->m_pfnCallback(pInfo->m_TrackID, pInfo->m_pCallbackUser);
			CNetBase::SendPacketConnless(m_Socket, &(pInfo->m_Addr), Token,
				m_pTokenManager->GenerateToken(pAddr),
				pInfo->m_aData, pInfo->m_DataSize);
			CConnlessPacketInfo *pNext = pInfo->m_pNext;
			if(pPrevInfo)
				pPrevInfo->m_pNext = pNext;
			if(pInfo == m_pConnlessPacketList)
				m_pConnlessPacketList = pNext;
			delete pInfo;
			pInfo = pNext;
		}
		else
		{
			if(pPrevInfo)
				pPrevInfo = pPrevInfo->m_pNext;
			else
				pPrevInfo = pInfo;
			pInfo = pInfo->m_pNext;
		}
	}

	// add the token
	if(Found || !(TokenFLag&NET_TOKENFLAG_RESPONSEONLY))
	{
		CAddressInfo Info;
		Info.m_Addr = *pAddr;
		Info.m_Token = Token;
		Info.m_Expiry = time_get() + time_freq() * NET_TOKENCACHE_ADDRESSEXPIRY;
		(*m_TokenCache.Allocate(sizeof(Info))) = Info;
	}
}

void CNetTokenCache::Update()
{
	int64 Now = time_get();

	// drop expired address info
	CAddressInfo *pAddrInfo;
	while((pAddrInfo = m_TokenCache.First()) && (pAddrInfo->m_Expiry <= Now))
		m_TokenCache.PopFirst();

	// try to fetch the token again for stored packets
	CConnlessPacketInfo * pEntry = m_pConnlessPacketList;
	while(pEntry)
	{
		if(pEntry->m_LastTokenRequest + 2*time_freq() <= Now)
		{
			FetchToken(&pEntry->m_Addr);
			pEntry->m_LastTokenRequest = Now;
		}
		pEntry = pEntry->m_pNext;
	}

	// drop expired packets
	while(m_pConnlessPacketList && m_pConnlessPacketList->m_Expiry <= Now)
	{
		CConnlessPacketInfo *pNewList;
		pNewList = m_pConnlessPacketList->m_pNext;
		delete m_pConnlessPacketList;
		m_pConnlessPacketList = pNewList;
	}
}

