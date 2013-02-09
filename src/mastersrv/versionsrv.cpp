/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/version.h>

#include "mastersrv.h"
#include "versionsrv.h"
#include "mapversions.h"

class CMastersrvSlave_Ver : public IMastersrvSlave
{
public:
	CMastersrvSlave_Ver(IMastersrv *pOwner);
	virtual ~CMastersrvSlave_Ver() {}

	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); return -1; }

	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken);

	void SendVersion(const NETADDR *pAddr, TOKEN PacketToken);
	void SendMaplist(const NETADDR *pAddr, TOKEN PacketToken);

	// stubs, not needed
	virtual void SendCheck(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendOk(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendError(const NETADDR *pAddr, void *pUserData) { dbg_assert(0, "stub"); }
	virtual void SendList(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData) { dbg_assert(0, "stub"); }

	static NETADDR s_NullAddr;

	char m_aVersionPacket[sizeof(VERSIONSRV_VERSION) + sizeof(GAME_RELEASE_VERSION)];
	char m_aMaplistPacket[sizeof(VERSIONSRV_MAPLIST) + sizeof(s_aMapVersionList)];
};

NETADDR CMastersrvSlave_Ver::s_NullAddr = { 0 };

CMastersrvSlave_Ver::CMastersrvSlave_Ver(IMastersrv *pOwner)
 : IMastersrvSlave(pOwner, IMastersrv::MASTERSRV_VER_LEGACY)
{
	for(int i = 0; i < s_NumMapVersionItems; i++)
		AddServer(&s_NullAddr, &s_aMapVersionList[i]);
	mem_copy(m_aVersionPacket, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	mem_copy(m_aVersionPacket + sizeof(VERSIONSRV_VERSION),
		GAME_RELEASE_VERSION, sizeof(GAME_RELEASE_VERSION));

	mem_copy(m_aMaplistPacket, VERSIONSRV_MAPLIST, sizeof(VERSIONSRV_MAPLIST));
	mem_copy(m_aMaplistPacket + sizeof(VERSIONSRV_MAPLIST),
		s_aMapVersionList, sizeof(s_aMapVersionList));

	dbg_msg("mastersrv", "started versionsrv");
}

void CMastersrvSlave_Ver::SendVersion(const NETADDR *pAddr, TOKEN PacketToken)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_pData = m_aVersionPacket;
	Packet.m_DataSize = sizeof(m_aVersionPacket);
	m_pOwner->Send(IMastersrv::SOCKET_VERSION, &Packet, PacketToken);
}

void CMastersrvSlave_Ver::SendMaplist(const NETADDR *pAddr, TOKEN PacketToken)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_pData = m_aMaplistPacket;
	Packet.m_DataSize = sizeof(m_aMaplistPacket);
	m_pOwner->Send(IMastersrv::SOCKET_VERSION, &Packet, PacketToken);
}

int CMastersrvSlave_Ver::ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken)
{
	if(Socket != IMastersrv::SOCKET_VERSION)
		return 0;

	if(pPacket->m_DataSize == sizeof(VERSIONSRV_GETVERSION) &&
		mem_comp(pPacket->m_pData, VERSIONSRV_GETVERSION, sizeof(VERSIONSRV_GETVERSION)) == 0)
	{
		SendVersion(&pPacket->m_Address, PacketToken);
		return 1;
	}
	else if(pPacket->m_DataSize == sizeof(VERSIONSRV_GETMAPLIST) &&
		mem_comp(pPacket->m_pData, VERSIONSRV_GETMAPLIST, sizeof(VERSIONSRV_GETMAPLIST)) == 0)
	{
		if(pPacket->m_Flags&NETSENDFLAG_STATELESS)
			return 1; // new-style packet without token -> drop
		SendMaplist(&pPacket->m_Address, PacketToken);
		return 1;
	}
	return 0;
}

IMastersrvSlave *CreateSlave_Ver(IMastersrv *pOwner)
{
	return new CMastersrvSlave_Ver(pOwner);
}

