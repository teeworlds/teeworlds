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
static CMapversionPacketData s_aMapversionPackets[MAX_PACKETS];
static int s_NumMapversionPackets = 0;

static void BuildVersionPacket()
{
	mem_copy(s_aVersionPacket, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	mem_copy(s_aVersionPacket + sizeof(VERSIONSRV_VERSION), GAME_RELEASE_VERSION, sizeof(GAME_RELEASE_VERSION));
}

static void BuildMapversionPacket()
{
	dbg_assert(s_NumMapVersionItems <= MAX_MAPS, "too many maps");
	const CMapVersion *pCurrent = &s_aMapVersionList[0];
	int MapsLeft = s_NumMapVersionItems;
	s_NumMapversionPackets = 0;
	while(MapsLeft > 0 && s_NumMapversionPackets < MAX_PACKETS)
	{
		const int Chunk = minimum<int>(MapsLeft, MAX_MAPS_PER_PACKET);
		MapsLeft -= Chunk;

		// copy header
		mem_copy(s_aMapversionPackets[s_NumMapversionPackets].m_Data.m_aHeader, VERSIONSRV_MAPLIST, sizeof(VERSIONSRV_MAPLIST));

		// copy map versions
		for(int i = 0; i < Chunk; i++)
		{
			s_aMapversionPackets[s_NumMapversionPackets].m_Data.m_aMaplist[i] = *pCurrent;
			pCurrent++;
		}

		s_aMapversionPackets[s_NumMapversionPackets].m_Size = sizeof(VERSIONSRV_MAPLIST) + sizeof(CMapVersion) * Chunk;

		s_NumMapversionPackets++;
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

static void SendMapversions(NETADDR *pAddr, TOKEN ResponseToken)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;

	for(int i = 0; i < s_NumMapversionPackets; i++)
	{
		Packet.m_DataSize = s_aMapversionPackets[i].m_Size;
		Packet.m_pData = &s_aMapversionPackets[i].m_Data;
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
	BuildMapversionPacket();

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

			if(Packet.m_DataSize == sizeof(VERSIONSRV_GETMAPLIST)
				&& mem_comp(Packet.m_pData, VERSIONSRV_GETMAPLIST, sizeof(VERSIONSRV_GETMAPLIST)) == 0)
			{
				SendMapversions(&Packet.m_Address, ResponseToken);
			}
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	cmdline_free(argc, argv);
	return 0;
}
