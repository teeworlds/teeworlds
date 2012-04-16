/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <stdlib.h> // rand

#include <base/system.h>

#include <engine/external/md5/md5.h>

#include "network.h"

static TOKEN Hash(char *pData, int Size)
{
	md5_state_t State;
	unsigned int aDigest[4]; // make sure this is 16 byte

	md5_init(&State);
	md5_append(&State, (const md5_byte_t *)pData, Size);
	md5_finish(&State, (md5_byte_t *)aDigest);

	return aDigest[0] ^ aDigest[1] ^ aDigest[2] ^ aDigest[3];
}

void CNetTokenManager::Init(NETSOCKET Socket)
{
	m_Socket = Socket;
	GenerateSeed();
}

int CNetTokenManager::ProcessMessage(const NETADDR *pAddr, const CNetPacketConstruct *pPacket, bool Notify)
{
	if(pPacket->m_Token != NET_TOKEN_NONE && !CheckToken(pAddr, pPacket->m_Token, pPacket->m_ResponseToken, Notify))
		return 0; // wrong token, silent ignore

	bool Verified = pPacket->m_Token != NET_TOKEN_NONE;
	bool TokenMessage = (pPacket->m_Flags & NET_PACKETFLAG_CONTROL)
		&& pPacket->m_aChunkData[0] == NET_CTRLMSG_TOKEN;

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		return (Verified) ? 1 : -1; // connless packets without token are allowed

	if(!TokenMessage)
		if(Verified)
			return 1; // verified packet
		else
			// the only allowed not connless packet
			// without token is NET_CTRLMSG_TOKEN
			return 0;

	if(Verified && TokenMessage)
		return 0; // everything is fine, token exchange complete

	// client requesting token
	CNetBase::SendToken(m_Socket, (NETADDR *)pAddr, GenerateToken(pAddr, m_Seed), pPacket->m_ResponseToken);
	return 0; // no need to process NET_CTRLMSG_TOKEN further
}

void CNetTokenManager::GenerateSeed()
{
	m_PrevSeed = m_Seed;

	for(int i = 0; i < 4; i++)
	{
		// rand() returns a random integer between 0 and RAND_MAX
		// RAND_MAX is at least 1<<16 according to the standard
		m_Seed <<= 16;
		m_Seed ^= rand();
	}
}

TOKEN CNetTokenManager::GenerateToken(const NETADDR *pAddr)
{
	return GenerateToken(pAddr, m_Seed);
}

TOKEN CNetTokenManager::GenerateToken(const NETADDR *pAddr, int64 Seed)
{
	char aBuf[sizeof(NETADDR) + sizeof(int64)];
	int Result;

	mem_copy(aBuf, pAddr, sizeof(NETADDR));
	mem_copy(aBuf + sizeof(NETADDR), &Seed, sizeof(int64));

	Result = Hash(aBuf, sizeof(aBuf));
	if(Result == NET_TOKEN_NONE)
		Result++;
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
			CNetBase::SendToken(m_Socket, (NETADDR *)pAddr, CurrentToken, ResponseToken); // notify the peer about the new token
		return true;
	}

	return false;
}

