/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef MASTERSRV_MASTERSRV_H
#define MASTERSRV_MASTERSRV_H

#include <engine/kernel.h>
#include <engine/shared/network.h>

struct CMastersrvAddr
{
	unsigned char m_aIp[16];
	unsigned char m_aPort[2];
};


static const unsigned char SERVERBROWSE_HEARTBEAT[] = {255, 255, 255, 255, 'b', 'e', 'a', '2'};

static const unsigned char SERVERBROWSE_GETLIST[] = {255, 255, 255, 255, 'r', 'e', 'q', '2'};
static const unsigned char SERVERBROWSE_LIST[] = {255, 255, 255, 255, 'l', 'i', 's', '2'};

static const unsigned char SERVERBROWSE_GETCOUNT[] = {255, 255, 255, 255, 'c', 'o', 'u', '2'};
static const unsigned char SERVERBROWSE_COUNT[] = {255, 255, 255, 255, 's', 'i', 'z', '2'};

static const unsigned char SERVERBROWSE_GETINFO[] = {255, 255, 255, 255, 'g', 'i', 'e', '3'};
static const unsigned char SERVERBROWSE_INFO[] = {255, 255, 255, 255, 'i', 'n', 'f', '3'};

static const unsigned char SERVERBROWSE_FWCHECK[] = {255, 255, 255, 255, 'f', 'w', '?', '?'};
static const unsigned char SERVERBROWSE_FWRESPONSE[] = {255, 255, 255, 255, 'f', 'w', '!', '!'};
static const unsigned char SERVERBROWSE_FWOK[] = {255, 255, 255, 255, 'f', 'w', 'o', 'k'};
static const unsigned char SERVERBROWSE_FWERROR[] = {255, 255, 255, 255, 'f', 'w', 'e', 'r'};

class CNetChunk;
class IMastersrv;

enum
{
	MASTERSERVER_PORT=8300,
};

class IMastersrv : public IInterface
{
	MACRO_INTERFACE("mastersrv", 0)
public:
	enum
	{
		MASTERSRV_0_7=0,
		MASTERSRV_0_6,
		MASTERSRV_0_5,
		NUM_MASTERSRV,

		SOCKET_OP=0,
		SOCKET_CHECKER,
		NUM_SOCKETS,
	};

	IMastersrv() {}
	virtual ~IMastersrv() {}

	virtual int Init() = 0;
	virtual int Run() = 0;

	virtual void AddServer(const NETADDR *pAddr, void *pUserData, int Version) = 0;
	virtual void AddCheckserver(const NETADDR *pAddr, const NETADDR *pAltAddr, void *pUserData, int Version) = 0;
	virtual void SendList(const NETADDR *pAddr, void *pUserData, int Version) = 0;
	virtual int GetCount() const = 0;

	virtual int Send(int Socket, const CNetChunk *pPacket, TOKEN PacketToken, int PacketVersion) = 0;
};

class IMastersrvSlave
{
public:
	IMastersrvSlave(IMastersrv *pOwner, int Version) : m_pOwner(pOwner), m_Version(Version) {}
	virtual ~IMastersrvSlave() {}

	// hooks
	void AddServer(const NETADDR *pAddr, void *pUserData) { m_pOwner->AddServer(pAddr, pUserData, m_Version); }
	void AddCheckserver(const NETADDR *pAddr, const NETADDR *pAltAddr, void *pUserData) { m_pOwner->AddCheckserver(pAddr, pAltAddr, pUserData, m_Version); }
	void SendList(const NETADDR *pAddr, void *pUserData) { m_pOwner->SendList(pAddr, pUserData, m_Version); }

	// interface for packet building
	//    these functions return number of bytes written
	//    if that number smaller than 0, an error is assumed
	virtual int BuildPacketStart(void *pData, int MaxLength) { return 0; };
	virtual int BuildPacketAdd(void *pData, int MaxLength, const NETADDR *pAddr, void *pUserData) = 0;
	virtual int BuildPacketFinalize(void *pData, int MaxLength) { return 0; };

	// interface for packet receiving
	//   this function shall return 0 if the packet should be furtherly processed
	virtual int ProcessMessage(int Socket, const CNetChunk *pPacket, TOKEN PacketToken, int PacketVersion) = 0;

	// interface for network
	virtual void SendCheck(const NETADDR *pAddr, void *pUserData) = 0;
	virtual void SendOk(const NETADDR *pAddr, void *pUserData) = 0;
	virtual void SendError(const NETADDR *pAddr, void *pUserData) = 0;
	virtual void SendList(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData) = 0;

	static void NetaddrToMastersrv(CMastersrvAddr *pOut, const NETADDR *pIn);

protected:
	IMastersrv *m_pOwner;
	const int m_Version;
};

class IMastersrvSlaveAdv : public IMastersrvSlave
{
public:
	IMastersrvSlaveAdv(IMastersrv *pOwner, int Version) : IMastersrvSlave(pOwner, Version) {}
	virtual ~IMastersrvSlaveAdv() {}

	virtual void SendCount(const NETADDR *pAddr, void *pUserData) { SendPacket(PACKET_COUNT, pAddr, pUserData); }
	virtual void SendCheck(const NETADDR *pAddr, void *pUserData) { SendPacket(PACKET_CHECK, pAddr, pUserData); }
	virtual void SendOk(const NETADDR *pAddr, void *pUserData)    { SendPacket(PACKET_OK,    pAddr, pUserData); }
	virtual void SendError(const NETADDR *pAddr, void *pUserData) { SendPacket(PACKET_ERROR, pAddr, pUserData); }
	virtual void SendList(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData)
	{
		m_aPackets[PACKET_LIST].m_DataSize = DataSize;
		m_aPackets[PACKET_LIST].m_pData = pData;
		SendPacket(PACKET_LIST, pAddr, pUserData);
	}

protected:
	virtual void SendPacket(int PacketType, const NETADDR *pAddr, void *pUserData) = 0;

	enum
	{
		PACKET_COUNT=0,
		PACKET_CHECK,
		PACKET_OK,
		PACKET_ERROR,
		PACKET_LIST,
		NUM_PACKETS,
	};
	CNetChunk m_aPackets[NUM_PACKETS];
};

IMastersrv *CreateMastersrv();

IMastersrvSlave *CreateSlave_0_5(IMastersrv *pOwner);
IMastersrvSlave *CreateSlave_0_6(IMastersrv *pOwner);
IMastersrvSlave *CreateSlave_0_7(IMastersrv *pOwner);


#endif
