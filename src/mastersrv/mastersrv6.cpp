/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "mastersrv.h"

static const unsigned char SERVERBROWSE6_HEARTBEAT[]  = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'b', 'e', 'a', '2'};

static const unsigned char SERVERBROWSE6_GETLIST[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'r', 'e', 'q', '2'};
static const unsigned char SERVERBROWSE6_LIST[]       = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'l', 'i', 's', '2'};

static const unsigned char SERVERBROWSE6_GETCOUNT[]   = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'c', 'o', 'u', '2'};
static const unsigned char SERVERBROWSE6_COUNT[]      = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 's', 'i', 'z', '2'};

static const unsigned char SERVERBROWSE6_FWCHECK[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', '?', '?'};
static const unsigned char SERVERBROWSE6_FWRESPONSE[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', '!', '!'};
static const unsigned char SERVERBROWSE6_FWOK[]       = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', 'o', 'k'};
static const unsigned char SERVERBROWSE6_FWERROR[]    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'f', 'w', 'e', 'r'};
//                                                       0    1    2    3    4    5    6    7    8    9   10   11   12   13

class CMastersrvSlave_0_6 : public IMastersrvSlaveAdv
{
public:
	CMastersrvSlave_0_6(IMastersrv *pOwner);
	virtual ~CMastersrvSlave_0_6() {}

	virtual int BuildPacketStart(void *pData, int MaxLength);
	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData);
	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken) { return 0; }
	virtual int ProcessMessageRaw(int Socket, const NETADDR *pAddress, const void *pData, int DataSize);

	virtual void SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData);

	struct CCountPacketData
	{
		unsigned char m_Header[sizeof(SERVERBROWSE6_COUNT)];
		unsigned char m_High;
		unsigned char m_Low;
	} m_CountData;
};

CMastersrvSlave_0_6::CMastersrvSlave_0_6(IMastersrv *pOwner)
 : IMastersrvSlaveAdv(pOwner, IMastersrv::MASTERSRV_0_6)
{
	mem_copy(m_CountData.m_Header, SERVERBROWSE6_COUNT, sizeof(m_CountData.m_Header));

	for(int i = 0; i < NUM_PACKETS; i++)
	{
		m_aPackets[i].m_ClientID = -1;
		m_aPackets[i].m_Flags = NETSENDFLAG_CONNLESS|NETSENDFLAG_STATELESS;
	}

	m_aPackets[PACKET_COUNT].m_DataSize = sizeof(m_CountData);
	m_aPackets[PACKET_COUNT].m_pData = &m_CountData;

	m_aPackets[PACKET_CHECK].m_DataSize = sizeof(SERVERBROWSE6_FWCHECK);
	m_aPackets[PACKET_CHECK].m_pData = SERVERBROWSE6_FWCHECK;

	m_aPackets[PACKET_OK].m_DataSize = sizeof(SERVERBROWSE6_FWOK);
	m_aPackets[PACKET_OK].m_pData = SERVERBROWSE6_FWOK;

	m_aPackets[PACKET_ERROR].m_DataSize = sizeof(SERVERBROWSE6_FWERROR);
	m_aPackets[PACKET_ERROR].m_pData = SERVERBROWSE6_FWERROR;

	dbg_msg("mastersrv", "started mastersrv 0.6");
}

int CMastersrvSlave_0_6::BuildPacketStart(void *pData, int MaxLength)
{
	if(MaxLength < (int)sizeof(SERVERBROWSE6_LIST))
		return -1;
	mem_copy(pData, SERVERBROWSE6_LIST, sizeof(SERVERBROWSE6_LIST));
	return sizeof(SERVERBROWSE6_LIST);
}

int CMastersrvSlave_0_6::BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData)
{
	if(MaxLength < (int)sizeof(CMastersrvAddr))
		return -1;
	NetaddrToMastersrv((CMastersrvAddr *)pData, pAddr);
	return sizeof(CMastersrvAddr);
}

int CMastersrvSlave_0_6::ProcessMessageRaw(int Socket, const NETADDR *pAddress, const void *pData, int DataSize)
{
	if(Socket == IMastersrv::SOCKET_OP)
	{
		if(DataSize == sizeof(SERVERBROWSE6_HEARTBEAT)+2 &&
			mem_comp(pData, SERVERBROWSE6_HEARTBEAT, sizeof(SERVERBROWSE6_HEARTBEAT)) == 0)
		{
			NETADDR Alt;
			unsigned char *d = (unsigned char *)pData;
			Alt = *pAddress;
			Alt.port = (d[sizeof(SERVERBROWSE6_HEARTBEAT)]<<8)
				| d[sizeof(SERVERBROWSE6_HEARTBEAT)+1];
			AddCheckserver(pAddress, &Alt, 0);
			return 1;
		}
		else if(DataSize == sizeof(SERVERBROWSE6_GETCOUNT) &&
			mem_comp(pData, SERVERBROWSE6_GETCOUNT, sizeof(SERVERBROWSE6_GETCOUNT)) == 0)
		{
			int Count = m_pOwner->GetCount();
			m_CountData.m_High = (Count>>8)&0xff;
			m_CountData.m_Low = Count&0xff;
			SendCount(pAddress, 0);
			return 1;
		}
		else if(DataSize == sizeof(SERVERBROWSE6_GETLIST) &&
			mem_comp(pData, SERVERBROWSE6_GETLIST, sizeof(SERVERBROWSE6_GETLIST)) == 0)
		{
			IMastersrvSlave::SendList(pAddress, 0);
			return 1;
		}
	}
	else if(Socket == IMastersrv::SOCKET_CHECKER)
	{
		if(DataSize == sizeof(SERVERBROWSE6_FWRESPONSE) &&
			mem_comp(pData, SERVERBROWSE6_FWRESPONSE, sizeof(SERVERBROWSE6_FWRESPONSE)) == 0)
		{
			AddServer(pAddress, 0);
			return 0; // for 0.5 compatiblity
		}
	}
	else
		dbg_assert(0, "invalid socket type");

	return 0;
}

void CMastersrvSlave_0_6::SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData)
{
	dbg_assert(PacketType >= 0 && PacketType < NUM_PACKETS, "invalid packet type");

	m_pOwner->SendRaw((PacketType != PACKET_CHECK) ? IMastersrv::SOCKET_OP : IMastersrv::SOCKET_CHECKER,
		pAddr, m_aPackets[PacketType].m_pData, m_aPackets[PacketType].m_DataSize);
}

IMastersrvSlave *CreateSlave_0_6(IMastersrv *pOwner)
{
	return new CMastersrvSlave_0_6(pOwner);
}

