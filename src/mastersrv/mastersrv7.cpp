/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "mastersrv.h"

class CMastersrvSlave_0_7 : public IMastersrvSlaveAdv
{
public:
	CMastersrvSlave_0_7(IMastersrv *pOwner);
	virtual ~CMastersrvSlave_0_7() {}

	virtual int BuildPacketStart(void *pData, int MaxLength);
	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData);
	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken, int PacketVersion);

	virtual void SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData);

	struct CCountPacketData
	{
		unsigned char m_Header[sizeof(SERVERBROWSE_COUNT)];
		unsigned char m_High;
		unsigned char m_Low;
	} m_CountData;
};

CMastersrvSlave_0_7::CMastersrvSlave_0_7(IMastersrv *pOwner)
 : IMastersrvSlaveAdv(pOwner, IMastersrv::MASTERSRV_0_7)
{
	mem_copy(m_CountData.m_Header, SERVERBROWSE_COUNT, sizeof(m_CountData.m_Header));

	for(int i = 0; i < NUM_PACKETS; i++)
	{
		m_aPackets[i].m_ClientID = -1;
		m_aPackets[i].m_Flags = NETSENDFLAG_CONNLESS;
	}

	m_aPackets[PACKET_COUNT].m_DataSize = sizeof(m_CountData);
	m_aPackets[PACKET_COUNT].m_pData = &m_CountData;

	m_aPackets[PACKET_CHECK].m_DataSize = sizeof(SERVERBROWSE_FWCHECK);
	m_aPackets[PACKET_CHECK].m_pData = SERVERBROWSE_FWCHECK;

	m_aPackets[PACKET_OK].m_DataSize = sizeof(SERVERBROWSE_FWOK);
	m_aPackets[PACKET_OK].m_pData = SERVERBROWSE_FWOK;

	m_aPackets[PACKET_ERROR].m_DataSize = sizeof(SERVERBROWSE_FWERROR);
	m_aPackets[PACKET_ERROR].m_pData = SERVERBROWSE_FWERROR;
}

int CMastersrvSlave_0_7::BuildPacketStart(void *pData, int MaxLength)
{
	if(MaxLength < sizeof(SERVERBROWSE_GETLIST))
		return -1;
	mem_copy(pData, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST));
	return sizeof(SERVERBROWSE_GETLIST);
}

int CMastersrvSlave_0_7::BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData)
{
	if(MaxLength < sizeof(CMastersrvAddr))
		return -1;
	NetaddrToMastersrv((CMastersrvAddr *)pData, pAddr);
	return sizeof(CMastersrvAddr);
}

int CMastersrvSlave_0_7::ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken, int PacketVersion)
{
	if(PacketVersion != NET_PACKETVERSION || !(pPacket->m_Flags&NETSENDFLAG_CONNLESS))
		return 0;

	if(Socket == IMastersrv::SOCKET_OP)
	{
		if(pPacket->m_DataSize == sizeof(SERVERBROWSE_HEARTBEAT)+2 &&
			mem_comp(pPacket->m_pData, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT)) == 0)
		{
			if(pPacket->m_Flags&NETSENDFLAG_STATELESS)
				return -1;
			NETADDR Alt;
			unsigned char *d = (unsigned char *)pPacket->m_pData;
			Alt = pPacket->m_Address;
			Alt.port = (d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8)
				| d[sizeof(SERVERBROWSE_HEARTBEAT)+1];
			AddCheckserver(&pPacket->m_Address, &Alt, (void *)PacketToken);
			return 1;
		}
		else if(pPacket->m_DataSize == sizeof(SERVERBROWSE_GETCOUNT) &&
			mem_comp(pPacket->m_pData, SERVERBROWSE_GETCOUNT, sizeof(SERVERBROWSE_GETCOUNT)) == 0)
		{
			int Count = m_pOwner->GetCount();
			m_CountData.m_High = (Count>>8)&0xff;
			m_CountData.m_Low = Count&0xff;
			SendPacket(PACKET_COUNT, &pPacket->m_Address, (void *)PacketToken);
			return 1;
		}
		else if(pPacket->m_DataSize == sizeof(SERVERBROWSE_GETLIST) &&
			mem_comp(pPacket->m_pData, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST)) == 0)
		{
			if(pPacket->m_Flags&NETSENDFLAG_STATELESS)
				return -1;
			IMastersrvSlave::SendList(&pPacket->m_Address, (void *)PacketToken);
			return 1;
		}
	}
	else if(Socket == IMastersrv::SOCKET_CHECKER)
	{
		if(pPacket->m_DataSize == sizeof(SERVERBROWSE_FWRESPONSE) &&
			mem_comp(pPacket->m_pData, SERVERBROWSE_FWRESPONSE, sizeof(SERVERBROWSE_FWRESPONSE)) == 0)
		{
			AddServer(&pPacket->m_Address, (void *)PacketToken);
			return 1;
		}
	}
	else
		dbg_assert(0, "invalid socket type");

	return 0;
}

void CMastersrvSlave_0_7::SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData)
{
	dbg_assert(PacketType >= 0 && PacketType < NUM_PACKETS, "invalid packet type");

	m_aPackets[PacketType].m_Address = *pAddr;
	m_pOwner->Send((PacketType != PACKET_CHECK) ? IMastersrv::SOCKET_OP : IMastersrv::SOCKET_CHECKER,
		&m_aPackets[PacketType], (TOKEN) (long int) pUserData, NET_PACKETVERSION);
		
}

IMastersrvSlave *CreateSlave_0_7(IMastersrv *pOwner)
{
	return new CMastersrvSlave_0_7(pOwner);
}

