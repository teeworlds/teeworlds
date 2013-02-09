/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/version.h>

#include "mastersrv.h"
#include "versionsrv.h"
#include "mapversions.h"


static const unsigned char VERSIONSRV_GETVERSION_LEGACY[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'v', 'e', 'r', 'g'};
static const unsigned char VERSIONSRV_VERSION_LEGACY[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'v', 'e', 'r', 's'};

static const unsigned char VERSIONSRV_GETMAPLIST_LEGACY[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'v', 'm', 'l', 'g'};
static const unsigned char VERSIONSRV_MAPLIST_LEGACY[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'v', 'm', 'l', 's'};

static const unsigned char VERSION_DATA_LEGACY[] = {0x00, 0, 1, 2};

class CMastersrvSlave_VerLegacy : public IMastersrvSlave
{
public:
	CMastersrvSlave_VerLegacy(IMastersrv *pOwner);
	virtual ~CMastersrvSlave_VerLegacy() {}

	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); return -1;}
	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken) { return 0; }
	virtual int ProcessMessageRaw(int Socket, const NETADDR *pAddress, const void *pData, int DataSize);

	void SendVersion(const NETADDR *pAddr);
	void SendMaplist(const NETADDR *pAddr);

	// stubs, not needed
	virtual void SendCheck(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendOk(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendError(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendList(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData) { dbg_assert(0, "stub"); }

	static NETADDR s_NullAddr;

	char m_aVersionPacket[sizeof(VERSIONSRV_VERSION_LEGACY) + sizeof(VERSION_DATA_LEGACY)];
	char m_aMaplistPacket[sizeof(VERSIONSRV_MAPLIST_LEGACY) + sizeof(s_aMapVersionList)];
};

NETADDR CMastersrvSlave_VerLegacy::s_NullAddr = { 0 };

CMastersrvSlave_VerLegacy::CMastersrvSlave_VerLegacy(IMastersrv *pOwner)
 : IMastersrvSlave(pOwner, IMastersrv::MASTERSRV_VER)
{
	for(int i = 0; i < s_NumMapVersionItems; i++)
		AddServer(&s_NullAddr, &s_aMapVersionList[i]);
	mem_copy(m_aVersionPacket, VERSIONSRV_VERSION_LEGACY, sizeof(VERSIONSRV_VERSION_LEGACY));

	(m_aVersionPacket + sizeof(VERSIONSRV_VERSION_LEGACY))[0] = 0;
	(m_aVersionPacket + sizeof(VERSIONSRV_VERSION_LEGACY))[1] = GAME_RELEASE_VERSION[0] - '0';
	(m_aVersionPacket + sizeof(VERSIONSRV_VERSION_LEGACY))[2] = GAME_RELEASE_VERSION[2] - '0';
	(m_aVersionPacket + sizeof(VERSIONSRV_VERSION_LEGACY))[3] = GAME_RELEASE_VERSION[4] - '0';

	mem_copy(m_aMaplistPacket, VERSIONSRV_MAPLIST_LEGACY, sizeof(VERSIONSRV_MAPLIST_LEGACY));
	mem_copy(m_aMaplistPacket + sizeof(VERSIONSRV_MAPLIST_LEGACY),
		s_aMapVersionList, sizeof(s_aMapVersionList));

	dbg_msg("mastersrv", "started versionsrv_legacy");
}

void CMastersrvSlave_VerLegacy::SendVersion(const NETADDR *pAddr)
{
	m_pOwner->SendRaw(IMastersrv::SOCKET_VERSION, pAddr, m_aVersionPacket, sizeof(m_aVersionPacket));
}

void CMastersrvSlave_VerLegacy::SendMaplist(const NETADDR *pAddr)
{
	m_pOwner->SendRaw(IMastersrv::SOCKET_VERSION, pAddr, m_aMaplistPacket, sizeof(m_aMaplistPacket));
}

int CMastersrvSlave_VerLegacy::ProcessMessageRaw(int Socket, const NETADDR *pAddress, const void *pData, int DataSize)
{
	if(Socket != IMastersrv::SOCKET_VERSION)
		return 0;

	if(DataSize == sizeof(VERSIONSRV_GETVERSION_LEGACY) &&
		mem_comp(pData, VERSIONSRV_GETVERSION_LEGACY, sizeof(VERSIONSRV_GETVERSION_LEGACY)) == 0)
	{
		SendVersion(pAddress);
		return 1;
	}
	else if(DataSize == sizeof(VERSIONSRV_GETMAPLIST_LEGACY) &&
		mem_comp(pData, VERSIONSRV_GETMAPLIST_LEGACY, sizeof(VERSIONSRV_GETMAPLIST_LEGACY)) == 0)
	{
		SendMaplist(pAddress);
		return 1;
	}
	return 0;
}

IMastersrvSlave *CreateSlave_VerLegacy(IMastersrv *pOwner)
{
	return new CMastersrvSlave_VerLegacy(pOwner);
}

