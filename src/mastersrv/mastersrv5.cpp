/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "mastersrv.h"

static const unsigned char SERVERBROWSE5_HEARTBEAT[]  = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'b', 'e', 'a', 't'};

static const unsigned char SERVERBROWSE5_GETLIST[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'r', 'e', 'q', 't'};
static const unsigned char SERVERBROWSE5_LIST[]       = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'l', 'i', 's', 't'};

static const unsigned char SERVERBROWSE5_GETCOUNT[]   = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'c', 'o', 'u', 'n'};
static const unsigned char SERVERBROWSE5_COUNT[]      = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 's', 'i', 'z', 'e'};

static const unsigned char SERVERBROWSE5_FWCHECK[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', '?', '?'};
static const unsigned char SERVERBROWSE5_FWRESPONSE[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', '!', '!'};
static const unsigned char SERVERBROWSE5_FWOK[]       = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', 'o', 'k'};
static const unsigned char SERVERBROWSE5_FWERROR[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', 'e', 'r'};
//                                                       0    1    2    3    4    5    6    7    8    9   10   11   12   13

class CMastersrvSlave_0_5 : public IMastersrvSlaveAdv
{
public:
	CMastersrvSlave_0_5(IMastersrv *pOwner);
	virtual ~CMastersrvSlave_0_5() {}

	virtual int BuildPacketStart(void *pData, int MaxLength);
	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData);
	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken) { return 0; }
	virtual int ProcessMessageRaw(int Socket, const NETADDR *pAddress, const void *pData, int DataSize);

	virtual void SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData);

	struct CCountPacketData
	{
		unsigned char m_Header[sizeof(SERVERBROWSE5_COUNT)];
		unsigned char m_High;
		unsigned char m_Low;
	} m_CountData;

	struct CMastersrvAddrLegacy
	{
		unsigned char m_aIp[4];
		unsigned char m_aPort[2];
	};

	static void NetaddrToMastersrv(CMastersrvAddrLegacy *pOut, const NETADDR *pIn);
};

CMastersrvSlave_0_5::CMastersrvSlave_0_5(IMastersrv *pOwner)
 : IMastersrvSlaveAdv(pOwner, IMastersrv::MASTERSRV_0_5)
{
	mem_copy(m_CountData.m_Header, SERVERBROWSE5_COUNT, sizeof(m_CountData.m_Header));

	for(int i = 0; i < NUM_PACKETS; i++)
	{
		m_aPackets[i].m_ClientID = -1;
		m_aPackets[i].m_Flags = NETSENDFLAG_CONNLESS|NETSENDFLAG_STATELESS;
	}

	m_aPackets[PACKET_COUNT].m_DataSize = sizeof(m_CountData);
	m_aPackets[PACKET_COUNT].m_pData = &m_CountData;

	m_aPackets[PACKET_CHECK].m_DataSize = sizeof(SERVERBROWSE5_FWCHECK);
	m_aPackets[PACKET_CHECK].m_pData = SERVERBROWSE5_FWCHECK;

	m_aPackets[PACKET_OK].m_DataSize = sizeof(SERVERBROWSE5_FWOK);
	m_aPackets[PACKET_OK].m_pData = SERVERBROWSE5_FWOK;

	m_aPackets[PACKET_ERROR].m_DataSize = sizeof(SERVERBROWSE5_FWERROR);
	m_aPackets[PACKET_ERROR].m_pData = SERVERBROWSE5_FWERROR;

	dbg_msg("mastersrv", "started mastersrv 0.5");
}

int CMastersrvSlave_0_5::BuildPacketStart(void *pData, int MaxLength)
{
	if(MaxLength < (int)sizeof(SERVERBROWSE5_LIST))
		return -1;
	mem_copy(pData, SERVERBROWSE5_LIST, sizeof(SERVERBROWSE5_LIST));
	return sizeof(SERVERBROWSE5_LIST);
}

int CMastersrvSlave_0_5::BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData)
{
	if(MaxLength < (int)sizeof(CMastersrvAddrLegacy))
		return -1;
	NetaddrToMastersrv((CMastersrvAddrLegacy *)pData, pAddr);
	return sizeof(CMastersrvAddrLegacy);
}

int CMastersrvSlave_0_5::ProcessMessageRaw(int Socket, const NETADDR *pAddress, const void *pData, int DataSize)
{
	if(Socket == IMastersrv::SOCKET_OP)
	{
		if(DataSize == sizeof(SERVERBROWSE5_HEARTBEAT)+2 &&
			mem_comp(pData, SERVERBROWSE5_HEARTBEAT, sizeof(SERVERBROWSE5_HEARTBEAT)) == 0)
		{
			NETADDR Alt;
			unsigned char *d = (unsigned char *)pData;
			Alt = *pAddress;
			Alt.port = (d[sizeof(SERVERBROWSE5_HEARTBEAT)]<<8)
				| d[sizeof(SERVERBROWSE5_HEARTBEAT)+1];
			AddCheckserver(pAddress, &Alt, 0);
			return 1;
		}
		else if(DataSize == sizeof(SERVERBROWSE5_GETCOUNT) &&
			mem_comp(pData, SERVERBROWSE5_GETCOUNT, sizeof(SERVERBROWSE5_GETCOUNT)) == 0)
		{
			int Count = m_pOwner->GetCount();
			m_CountData.m_High = (Count>>8)&0xff;
			m_CountData.m_Low = Count&0xff;
			SendPacket(PACKET_COUNT, pAddress, 0);
			return 1;
		}
		else if(DataSize == sizeof(SERVERBROWSE5_GETLIST) &&
			mem_comp(pData, SERVERBROWSE5_GETLIST, sizeof(SERVERBROWSE5_GETLIST)) == 0)
		{
			IMastersrvSlave::SendList(pAddress, 0);
			return 1;
		}
	}
	else if(Socket == IMastersrv::SOCKET_CHECKER)
	{
		if(DataSize == sizeof(SERVERBROWSE5_FWRESPONSE) &&
			mem_comp(pData, SERVERBROWSE5_FWRESPONSE, sizeof(SERVERBROWSE5_FWRESPONSE)) == 0)
		{
			AddServer(pAddress, 0);
			return 0; // for 0.6 compatiblity
		}
	}
	else
		dbg_assert(0, "invalid socket type");

	return 0;
}

void CMastersrvSlave_0_5::SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData)
{
	dbg_assert(PacketType >= 0 && PacketType < NUM_PACKETS, "invalid packet type");

	m_pOwner->SendRaw((PacketType != PACKET_CHECK) ? IMastersrv::SOCKET_OP : IMastersrv::SOCKET_CHECKER,
		pAddr, m_aPackets[PacketType].m_pData, m_aPackets[PacketType].m_DataSize);
}

void CMastersrvSlave_0_5::NetaddrToMastersrv(CMastersrvAddrLegacy *pOut, const NETADDR *pIn)
{
	dbg_assert(pIn->type == NETTYPE_IPV4, "legacy mastersrv addresses only support ipv4");

	mem_copy(pOut->m_aIp, pIn->ip, 4);
	pOut->m_aPort[0] = (pIn->port>>0)&0xff; // little endian
	pOut->m_aPort[1] = (pIn->port>>8)&0xff;
}

IMastersrvSlave *CreateSlave_0_5(IMastersrv *pOwner)
{
	return new CMastersrvSlave_0_5(pOwner);
}

