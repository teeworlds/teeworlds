/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/network.h>

#include "versionsrv.h"

static CNetClient g_NetOp; // main

void SendVer(NETADDR *pAddr)
{
	CNetChunk p;
	unsigned char aData[sizeof(VERSIONSRV_VERSION) + sizeof(VERSION_DATA)];
	
	mem_copy(aData, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION));
	mem_copy(aData + sizeof(VERSIONSRV_VERSION), VERSION_DATA, sizeof(VERSION_DATA));
	
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
	BindAddr.port = VERSIONSRV_PORT;
	g_NetOp.Open(BindAddr, 0);
	
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
		}
		
		// be nice to the CPU
		thread_sleep(1);
	}
	
	return 0;
}
