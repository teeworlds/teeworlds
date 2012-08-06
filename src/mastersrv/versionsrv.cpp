/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "mastersrv.h"

#include <game/version.h>

struct CMapVersion
{
	char m_aName[8];
	unsigned char m_aCrc[4];
	unsigned char m_aSize[4];
};

static CMapVersion s_aMapVersionList[] = {
	{"ctf1", {0x06, 0xb5, 0xf1, 0x17}, {0x00, 0x00, 0x12, 0x38}},
	{"ctf2", {0x27, 0xbc, 0x5e, 0xac}, {0x00, 0x00, 0x64, 0x1a}},
	{"ctf3", {0xa3, 0x73, 0x9d, 0x41}, {0x00, 0x00, 0x17, 0x0f}},
	{"ctf4", {0xbe, 0x7c, 0x4d, 0xb9}, {0x00, 0x00, 0x2e, 0xfe}},
	{"ctf5", {0xd9, 0x21, 0x29, 0xa0}, {0x00, 0x00, 0x2f, 0x4c}},
	{"ctf6", {0x28, 0xc8, 0x43, 0x51}, {0x00, 0x00, 0x69, 0x2f}},
	{"ctf7", {0x1d, 0x35, 0x98, 0x72}, {0x00, 0x00, 0x15, 0x87}},
	{"dm1", {0xf2, 0x15, 0x9e, 0x6e}, {0x00, 0x00, 0x16, 0xad}},
	{"dm2", {0x71, 0x83, 0x98, 0x78}, {0x00, 0x00, 0x21, 0xdf}},
	{"dm6", {0x47, 0x4d, 0xa2, 0x35}, {0x00, 0x00, 0x1e, 0x95}},
	{"dm7", {0x42, 0x6d, 0xa1, 0x67}, {0x00, 0x00, 0x27, 0x2a}},
	{"dm8", {0x85, 0xf1, 0x1e, 0xd6}, {0x00, 0x00, 0x9e, 0xbd}},
	{"dm9", {0x42, 0xd4, 0x77, 0x7e}, {0x00, 0x00, 0x20, 0x11}},
};

static const int s_NumMapVersionItems = sizeof(s_aMapVersionList)/sizeof(CMapVersion);

static const unsigned char VERSIONSRV_GETVERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 'g'};
static const unsigned char VERSIONSRV_VERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 's'};

static const unsigned char VERSIONSRV_GETMAPLIST[] = {255, 255, 255, 255, 'v', 'm', 'l', 'g'};
static const unsigned char VERSIONSRV_MAPLIST[] = {255, 255, 255, 255, 'v', 'm', 'l', 's'};

class CMastersrvSlave_Ver : public IMastersrvSlave
{
public:
	struct CNetConnData
	{
		TOKEN m_Token;
		int m_Version;
	};

	CMastersrvSlave_Ver(IMastersrv *pOwner);
	virtual ~CMastersrvSlave_Ver() {}

	virtual int BuildPacketStart(void *pData, int MaxLength);
	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData);
	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken, int PacketVersion);

	virtual void SendList(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData);
	virtual void SendVersion(const NETADDR *pAddr, TOKEN PacketToken, int PacketVersion);

	// stubs, not needed
	virtual void SendCheck(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendOk(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendError(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }

	static NETADDR s_NullAddr;

	char m_aVersionPacket[sizeof(VERSIONSRV_VERSION) + sizeof(GAME_RELEASE_VERSION)];
};

NETADDR CMastersrvSlave_Ver::s_NullAddr = { 0 };

CMastersrvSlave_Ver::CMastersrvSlave_Ver(IMastersrv *pOwner)
 : IMastersrvSlave(pOwner, IMastersrv::MASTERSRV_VER)
{
	for(int i = 0; i < s_NumMapVersionItems; i++)
		AddServer(&s_NullAddr, &s_aMapVersionList[i]);
	mem_copy(m_aVersionPacket, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	mem_copy(m_aVersionPacket + sizeof(VERSIONSRV_VERSION),
		GAME_RELEASE_VERSION, sizeof(GAME_RELEASE_VERSION));

	dbg_msg("mastersrv", "started versionsrv");
}

void CMastersrvSlave_Ver::SendVersion(const NETADDR *pAddr, TOKEN PacketToken, int PacketVersion)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	if(PacketVersion == NET_PACKETVERSION_LEGACY)
		Packet.m_Flags |= NETSENDFLAG_STATELESS;
	Packet.m_pData = m_aVersionPacket;
	Packet.m_DataSize = sizeof(m_aVersionPacket);
	m_pOwner->Send(IMastersrv::SOCKET_VERSION, &Packet, PacketToken, PacketVersion);
}

void CMastersrvSlave_Ver::SendList(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData)
{
	CNetConnData *pConn = (CNetConnData *)pUserData;
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	if(pConn->m_Version == NET_PACKETVERSION_LEGACY)
		Packet.m_Flags |= NETSENDFLAG_STATELESS;
	Packet.m_pData = pData;
	Packet.m_DataSize = DataSize;
	m_pOwner->Send(IMastersrv::SOCKET_VERSION, &Packet, pConn->m_Token, pConn->m_Version);
}

int CMastersrvSlave_Ver::BuildPacketStart(void *pData, int MaxLength)
{
	if(MaxLength < sizeof(VERSIONSRV_MAPLIST))
		return -1;
	mem_copy(pData, VERSIONSRV_MAPLIST, sizeof(VERSIONSRV_MAPLIST));
	return sizeof(VERSIONSRV_MAPLIST);
}

int CMastersrvSlave_Ver::BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData)
{
	if(MaxLength < sizeof(CMapVersion))
		return -1;
	mem_copy(pData, pUserData, sizeof(CMapVersion));
	return sizeof(CMapVersion);
}

int CMastersrvSlave_Ver::ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken, int PacketVersion)
{
	if(Socket != IMastersrv::SOCKET_VERSION)
		return 0;

	if(pPacket->m_DataSize == sizeof(VERSIONSRV_GETVERSION) &&
		mem_comp(pPacket->m_pData, VERSIONSRV_GETVERSION, sizeof(VERSIONSRV_GETVERSION)) == 0)
	{
		SendVersion(&pPacket->m_Address, PacketToken, PacketVersion);
		return 1;
	}
	else if(pPacket->m_DataSize == sizeof(VERSIONSRV_GETMAPLIST) &&
		mem_comp(pPacket->m_pData, VERSIONSRV_GETMAPLIST, sizeof(VERSIONSRV_GETMAPLIST)) == 0)
	{
		if(PacketVersion == NET_PACKETVERSION &&
			(pPacket->m_Flags&NETSENDFLAG_STATELESS))
			return 1; // new-style packet without token -> drop
		CNetConnData Data;
		Data.m_Token = PacketToken;
		Data.m_Version = PacketVersion;
		IMastersrvSlave::SendList(&pPacket->m_Address, &Data);
		return 1;
	}
	return 0;
}

IMastersrvSlave *CreateSlave_Ver(IMastersrv *pOwner)
{
	return new CMastersrvSlave_Ver(pOwner);
}

