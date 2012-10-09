/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/network.h>

#include <game/version.h>

#include "versionsrv.h"
#include "mapversions.h"

enum {
	MAX_MAPS_PER_PACKET=48,
	MAX_PACKETS=16,
	MAX_MAPS=MAX_MAPS_PER_PACKET*MAX_PACKETS,
};

struct CPacketData
{
	int m_Size;
	struct {
		unsigned char m_aHeader[sizeof(VERSIONSRV_MAPLIST)];
		CMapVersion m_aMaplist[MAX_MAPS_PER_PACKET];
	} m_Data;
};

CPacketData m_aPackets[MAX_PACKETS];
static int m_NumPackets = 0;

static CNetClient g_NetOp; // main

void BuildPackets()
{
	CMapVersion *pCurrent = &s_aMapVersionList[0];
	int ServersLeft = s_NumMapVersionItems;
	m_NumPackets = 0;
	while(ServersLeft && m_NumPackets < MAX_PACKETS)
	{
		int Chunk = ServersLeft;
		if(Chunk > MAX_MAPS_PER_PACKET)
			Chunk = MAX_MAPS_PER_PACKET;
		ServersLeft -= Chunk;

		// copy header
		mem_copy(m_aPackets[m_NumPackets].m_Data.m_aHeader, VERSIONSRV_MAPLIST, sizeof(VERSIONSRV_MAPLIST));

		// copy map versions
		for(int i = 0; i < Chunk; i++)
		{
			m_aPackets[m_NumPackets].m_Data.m_aMaplist[i] = *pCurrent;
			pCurrent++;
		}

		m_aPackets[m_NumPackets].m_Size = sizeof(VERSIONSRV_MAPLIST) + sizeof(CMapVersion)*Chunk;

		m_NumPackets++;
	}
}

void SendVer(NETADDR *pAddr)
{
	CNetChunk p;
	unsigned char aData[sizeof(VERSIONSRV_VERSION) + sizeof(GAME_RELEASE_VERSION)];

	mem_copy(aData, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	mem_copy(aData + sizeof(VERSIONSRV_VERSION), GAME_RELEASE_VERSION, sizeof(GAME_RELEASE_VERSION));

	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_pData = aData;
	p.m_DataSize = sizeof(aData);

	g_NetOp.Send(&p);
}

int main(int argc, char **argv) // ignore_convention
{
	NETADDR BindAddr;

	dbg_logger_stdout();
	net_init();

	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_ALL;
	BindAddr.port = VERSIONSRV_PORT;
	if(!g_NetOp.Open(BindAddr, 0))
	{
		dbg_msg("mastersrv", "couldn't start network");
		return -1;
	}

	BuildPackets();

	dbg_msg("versionsrv", "started");

	while(1)
	{
		g_NetOp.Update();

		// process packets
		CNetChunk Packet;
		while(g_NetOp.Recv(&Packet))
		{
			if(Packet.m_DataSize == sizeof(VERSIONSRV_GETVERSION) &&
				mem_comp(Packet.m_pData, VERSIONSRV_GETVERSION, sizeof(VERSIONSRV_GETVERSION)) == 0)
			{
				SendVer(&Packet.m_Address);
			}

			if(Packet.m_DataSize == sizeof(VERSIONSRV_GETMAPLIST) &&
				mem_comp(Packet.m_pData, VERSIONSRV_GETMAPLIST, sizeof(VERSIONSRV_GETMAPLIST)) == 0)
			{
				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;

				for(int i = 0; i < m_NumPackets; i++)
				{
					p.m_DataSize = m_aPackets[i].m_Size;
					p.m_pData = &m_aPackets[i].m_Data;
					g_NetOp.Send(&p);
				}
			}
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	return 0;
}
