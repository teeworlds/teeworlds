/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h> //rand
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <mastersrv/mastersrv.h>

CNetServer *pNet;

int Progression = 50;
int GameType = 0;
int Flags = 0;

const char *pVersion = "trunk";
const char *pMap = "somemap";
const char *pServerName = "unnamed server";

NETADDR aMasterServers[16] = {{0,{0},0}};
int NumMasters = 0;

const char *PlayerNames[16] = {0};
int PlayerScores[16] = {0};
int NumPlayers = 0;
int MaxPlayers = 0;

char aInfoMsg[1024];
int aInfoMsgSize;

static void SendHeartBeats()
{
	static unsigned char aData[sizeof(SERVERBROWSE_HEARTBEAT) + 2];
	CNetChunk Packet;

	mem_copy(aData, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT));

	Packet.m_ClientID = -1;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_HEARTBEAT) + 2;
	Packet.m_pData = &aData;

	/* supply the set port that the master can use if it has problems */
	aData[sizeof(SERVERBROWSE_HEARTBEAT)] = 0;
	aData[sizeof(SERVERBROWSE_HEARTBEAT)+1] = 0;

	for(int i = 0; i < NumMasters; i++)
	{
		Packet.m_Address = aMasterServers[i];
		pNet->Send(&Packet);
	}
}

static void WriteStr(const char *pStr)
{
	int l = str_length(pStr)+1;
	mem_copy(&aInfoMsg[aInfoMsgSize], pStr, l);
	aInfoMsgSize += l;
}

static void WriteInt(int i)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%d", i);
	WriteStr(aBuf);
}

static void BuildInfoMsg()
{
	aInfoMsgSize = sizeof(SERVERBROWSE_INFO);
	mem_copy(aInfoMsg, SERVERBROWSE_INFO, aInfoMsgSize);
	WriteInt(-1);

	WriteStr(pVersion);
	WriteStr(pServerName);
	WriteStr(pMap);
	WriteInt(GameType);
	WriteInt(Flags);
	WriteInt(Progression);
	WriteInt(NumPlayers);
	WriteInt(MaxPlayers);

	for(int i = 0; i < NumPlayers; i++)
	{
		WriteStr(PlayerNames[i]);
		WriteInt(PlayerScores[i]);
	}
}

static void SendServerInfo(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = aInfoMsgSize;
	p.m_pData = aInfoMsg;
	pNet->Send(&p);
}

static void SendFWCheckResponse(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWRESPONSE);
	p.m_pData = SERVERBROWSE_FWRESPONSE;
	pNet->Send(&p);
}

static int Run()
{
	int64 NextHeartBeat = 0;
	NETADDR BindAddr = {NETTYPE_IPV4, {0},0};

	if(!pNet->Open(BindAddr, 0, 0, 0, 0))
		return 0;

	while(1)
	{
		CNetChunk p;
		pNet->Update();
		while(pNet->Recv(&p))
		{
			if(p.m_ClientID == -1)
			{
				if(p.m_DataSize == sizeof(SERVERBROWSE_GETINFO) &&
					mem_comp(p.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					SendServerInfo(&p.m_Address);
				}
				else if(p.m_DataSize == sizeof(SERVERBROWSE_FWCHECK) &&
					mem_comp(p.m_pData, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
				{
					SendFWCheckResponse(&p.m_Address);
				}
			}
		}

		/* send heartbeats if needed */
		if(NextHeartBeat < time_get())
		{
			NextHeartBeat = time_get()+time_freq()*(15+(rand()%15));
			SendHeartBeats();
		}

		thread_sleep(100);
	}
}

int main(int argc, char **argv)
{
	pNet = new CNetServer;

	while(argc)
	{
		// ?
		/*if(str_comp(*argv, "-m") == 0)
		{
			argc--; argv++;
			net_host_lookup(*argv, &aMasterServers[NumMasters], NETTYPE_IPV4);
			argc--; argv++;
			aMasterServers[NumMasters].port = str_toint(*argv);
			NumMasters++;
		}
		else */if(str_comp(*argv, "-p") == 0)
		{
			argc--; argv++;
			PlayerNames[NumPlayers++] = *argv;
			argc--; argv++;
			PlayerScores[NumPlayers] = str_toint(*argv);
		}
		else if(str_comp(*argv, "-a") == 0)
		{
			argc--; argv++;
			pMap = *argv;
		}
		else if(str_comp(*argv, "-x") == 0)
		{
			argc--; argv++;
			MaxPlayers = str_toint(*argv);
		}
		else if(str_comp(*argv, "-t") == 0)
		{
			argc--; argv++;
			GameType = str_toint(*argv);
		}
		else if(str_comp(*argv, "-g") == 0)
		{
			argc--; argv++;
			Progression = str_toint(*argv);
		}
		else if(str_comp(*argv, "-f") == 0)
		{
			argc--; argv++;
			Flags = str_toint(*argv);
		}
		else if(str_comp(*argv, "-n") == 0)
		{
			argc--; argv++;
			pServerName = *argv;
		}

		argc--; argv++;
	}

	BuildInfoMsg();
	int RunReturn = Run();

	delete pNet;
	return RunReturn;
}

