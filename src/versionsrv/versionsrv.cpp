/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/kernel.h>
#include <engine/storage.h>

#include <engine/shared/network.h>

#include <game/version.h>

#include "versionsrv.h"
#include "mapversions.h"

enum
{
	MAX_MAPS_PER_PACKET = 48,
	MAX_PACKETS = 16,
	MAX_MAPS = MAX_MAPS_PER_PACKET * MAX_PACKETS,
};

struct CMapversionPacketData
{
	int m_Size;
	struct
	{
		unsigned char m_aHeader[sizeof(VERSIONSRV_MAPLIST)];
		CMapVersion m_aMaplist[MAX_MAPS_PER_PACKET];
	} m_Data;
};

static CNetClient s_NetClient;

static unsigned char s_aVersionPacket[sizeof(VERSIONSRV_VERSION) + sizeof(GAME_RELEASE_VERSION)];
static CMapversionPacketData s_aMapversionPackets07[MAX_PACKETS];
static unsigned s_NumMapversionPackets07 = 0;

static void BuildVersionPacket()
{
	mem_copy(s_aVersionPacket, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	mem_copy(s_aVersionPacket + sizeof(VERSIONSRV_VERSION), GAME_RELEASE_VERSION, sizeof(GAME_RELEASE_VERSION));
}

static bool GetMapversionPackets(unsigned ClientVersion, CMapversionPacketData **pPacketData, unsigned **pNumPackets)
{
	switch(ClientVersion & 0xFFFFFF00u) // ignore minor version
	{
		case 0x0700: // 0.7.x
			*pPacketData = s_aMapversionPackets07;
			*pNumPackets = &s_NumMapversionPackets07;
			return true;
	}
	*pPacketData = 0x0;
	*pNumPackets = 0x0;
	return false;
}

static void BuildMapversionPacket(unsigned ClientVersion, const CMapVersion *pMapVersionList, unsigned NumMapVersionItems)
{
	dbg_assert(NumMapVersionItems <= MAX_MAPS, "too many maps");
	CMapversionPacketData *pPacketData;
	unsigned *pNumPackets;
	dbg_assert(GetMapversionPackets(ClientVersion, &pPacketData, &pNumPackets), "unhandled ClientVersion");

	unsigned MapsLeft = NumMapVersionItems;
	*pNumPackets = 0;
	while(MapsLeft > 0 && *pNumPackets < MAX_PACKETS)
	{
		const unsigned Chunk = minimum<unsigned>(MapsLeft, MAX_MAPS_PER_PACKET);
		MapsLeft -= Chunk;

		// copy header
		mem_copy(pPacketData[*pNumPackets].m_Data.m_aHeader, VERSIONSRV_MAPLIST, sizeof(VERSIONSRV_MAPLIST));

		// copy map versions
		for(unsigned i = 0; i < Chunk; i++)
		{
			pPacketData[*pNumPackets].m_Data.m_aMaplist[i] = *pMapVersionList;
			pMapVersionList++;
		}

		pPacketData[*pNumPackets].m_Size = sizeof(VERSIONSRV_MAPLIST) + sizeof(CMapVersion) * Chunk;

		(*pNumPackets)++;
	}
}

static void SendVersion(NETADDR *pAddr, TOKEN ResponseToken)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_pData = s_aVersionPacket;
	Packet.m_DataSize = sizeof(s_aVersionPacket);
	s_NetClient.Send(&Packet, ResponseToken);
}

static void SendMapversions(NETADDR *pAddr, TOKEN ResponseToken, unsigned ClientVersion)
{
	CMapversionPacketData *pPacketData;
	unsigned *pNumPackets;
	if(!GetMapversionPackets(ClientVersion, &pPacketData, &pNumPackets))
		return;

	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;

	for(unsigned i = 0; i < *pNumPackets; i++)
	{
		Packet.m_DataSize = pPacketData[i].m_Size;
		Packet.m_pData = &pPacketData[i].m_Data;
		s_NetClient.Send(&Packet, ResponseToken);
	}
}

int main(int argc, const char **argv)
{
	dbg_logger_stdout();
	cmdline_fix(&argc, &argv);

	const int FlagMask = 0;
	IKernel *pKernel = IKernel::Create();
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	IConfigManager *pConfigManager = CreateConfigManager();
	IConsole *pConsole = CreateConsole(FlagMask);

	bool RegisterFail = !pKernel->RegisterInterface(pStorage);
	RegisterFail |= !pKernel->RegisterInterface(pConsole);
	RegisterFail |= !pKernel->RegisterInterface(pConfigManager);
	if(RegisterFail)
		return -1;

	pConfigManager->Init(FlagMask);
	pConsole->Init();

	if(secure_random_init() != 0)
	{
		dbg_msg("versionsrv", "could not initialize secure RNG");
		return -1;
	}

	NETADDR BindAddr;
	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_ALL;
	BindAddr.port = VERSIONSRV_PORT;
	if(!s_NetClient.Open(BindAddr, pConfigManager->Values(), pConsole, 0, 0))
	{
		dbg_msg("versionsrv", "could not start network");
		return -1;
	}

	dbg_msg("versionsrv", "building packets");

	BuildVersionPacket();
	BuildMapversionPacket(0x0700, s_aMapVersionList, s_NumMapVersionItems);

	dbg_msg("versionsrv", "started");

	while(true)
	{
		s_NetClient.Update();

		// process packets
		CNetChunk Packet;
		TOKEN ResponseToken;
		while(s_NetClient.Recv(&Packet, &ResponseToken))
		{
			if(Packet.m_DataSize == sizeof(VERSIONSRV_GETVERSION)
				&& mem_comp(Packet.m_pData, VERSIONSRV_GETVERSION, sizeof(VERSIONSRV_GETVERSION)) == 0)
			{
				SendVersion(&Packet.m_Address, ResponseToken);
			}

			if((Packet.m_DataSize == sizeof(VERSIONSRV_GETMAPLIST)
					|| Packet.m_DataSize == sizeof(VERSIONSRV_GETMAPLIST) + sizeof(unsigned))
				&& mem_comp(Packet.m_pData, VERSIONSRV_GETMAPLIST, sizeof(VERSIONSRV_GETMAPLIST)) == 0)
			{
				unsigned ClientVersion = 0x0700;
				if(Packet.m_DataSize == sizeof(VERSIONSRV_GETMAPLIST) + sizeof(unsigned))
					ClientVersion = bytes_be_to_uint((unsigned char *)Packet.m_pData + sizeof(VERSIONSRV_GETMAPLIST));
				SendMapversions(&Packet.m_Address, ResponseToken, ClientVersion);
			}
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	cmdline_free(argc, argv);
	return 0;
}
