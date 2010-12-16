/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/engine.h>

#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <engine/shared/compression.h>

#include <engine/shared/network.h>
#include <engine/shared/config.h>
#include <engine/shared/packer.h>
#include <engine/shared/datafile.h>
#include <engine/shared/demo.h>

#include <engine/server.h>
#include <engine/map.h>
#include <engine/console.h>
#include <engine/storage.h>
#include <engine/masterserver.h>
#include <engine/config.h>

#include <mastersrv/mastersrv.h>

#include <string.h>

#include "register.h"
#include "server.h"
#include "../shared/linereader.h"
#include <vector>

#if defined(CONF_FAMILY_WINDOWS) 
	#define _WIN32_WINNT 0x0500
	#define NOGDI
	#include <windows.h>
#endif

static const char *StrLtrim(const char *pStr)
{
	while(*pStr && *pStr >= 0 && *pStr <= 32)
		pStr++;
	return pStr;
}

static void StrRtrim(char *pStr)
{
	int i = str_length(pStr);
	while(i >= 0)
	{
		if(pStr[i] < 0 || pStr[i] > 32)
			break;
		pStr[i] = 0;
		i--;
	}
}


static int StrAllnum(const char *pStr)
{
	while(*pStr)
	{
		if(!(*pStr >= '0' && *pStr <= '9'))
			return 0;
		pStr++;
	}
	return 1;
}

CSnapIDPool::CSnapIDPool()
{
	Reset();
}

void CSnapIDPool::Reset()
{
	for(int i = 0; i < MAX_IDS; i++)
	{
		m_aIDs[i].m_Next = i+1;
		m_aIDs[i].m_State = 0;
	}
		
	m_aIDs[MAX_IDS-1].m_Next = -1;
	m_FirstFree = 0;
	m_FirstTimed = -1;
	m_LastTimed = -1;
	m_Usage = 0;
	m_InUsage = 0;
}


void CSnapIDPool::RemoveFirstTimeout()
{
	int NextTimed = m_aIDs[m_FirstTimed].m_Next;
	
	// add it to the free list
	m_aIDs[m_FirstTimed].m_Next = m_FirstFree;
	m_aIDs[m_FirstTimed].m_State = 0;
	m_FirstFree = m_FirstTimed;
	
	// remove it from the timed list
	m_FirstTimed = NextTimed;
	if(m_FirstTimed == -1)
		m_LastTimed = -1;
		
	m_Usage--;
}

int CSnapIDPool::NewID()
{
	int64 Now = time_get();

	// process timed ids
	while(m_FirstTimed != -1 && m_aIDs[m_FirstTimed].m_Timeout < Now)
		RemoveFirstTimeout();
	
	int Id = m_FirstFree;
	dbg_assert(Id != -1, "id error");
	m_FirstFree = m_aIDs[m_FirstFree].m_Next;
	m_aIDs[Id].m_State = 1;
	m_Usage++;
	m_InUsage++;
	return Id;
}

void CSnapIDPool::TimeoutIDs()
{
	// process timed ids
	while(m_FirstTimed != -1)
		RemoveFirstTimeout();
}

void CSnapIDPool::FreeID(int Id)
{
	dbg_assert(m_aIDs[Id].m_State == 1, "id is not alloced");

	m_InUsage--;
	m_aIDs[Id].m_State = 2;
	m_aIDs[Id].m_Timeout = time_get()+time_freq()*5;
	m_aIDs[Id].m_Next = -1;
	
	if(m_LastTimed != -1)
	{
		m_aIDs[m_LastTimed].m_Next = Id;
		m_LastTimed = Id;
	}
	else
	{
		m_FirstTimed = Id;
		m_LastTimed = Id;
	}
}
	
void CServer::CClient::Reset()
{
	// reset input
	for(int i = 0; i < 200; i++)
		m_aInputs[i].m_GameTick = -1;
	m_CurrentInput = 0;
	mem_zero(&m_LatestInput, sizeof(m_LatestInput));

	m_Snapshots.PurgeAll();
	m_LastAckedSnapshot = -1;
	m_LastInputTick = -1;
	m_SnapRate = CClient::SNAPRATE_INIT;
	m_Score = 0;
}

CServer::CServer() : m_DemoRecorder(&m_SnapshotDelta)
{
	m_TickSpeed = SERVER_TICK_SPEED;
	
	m_pGameServer = 0;
	
	m_CurrentGameTick = 0;
	m_RunServer = 1;

	mem_zero(m_aBrowseinfoGametype, sizeof(m_aBrowseinfoGametype));
	m_BrowseinfoProgression = -1;

	m_pCurrentMapData = 0;
	m_CurrentMapSize = 0;
	
	m_MapReload = 0;

	m_RconClientId = -1;

	Init();
}


int CServer::TrySetClientName(int ClientID, const char *pName)
{
	char aTrimmedName[64];

	// trim the name
	str_copy(aTrimmedName, StrLtrim(pName), sizeof(aTrimmedName));
	StrRtrim(aTrimmedName);
	/*char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "'%s' -> '%s'", pName, aTrimmedName);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);*/
	pName = aTrimmedName;
	
	
	// check for empty names
	if(!pName[0])
		return -1;
	
	// make sure that two clients doesn't have the same name
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(i != ClientID && m_aClients[i].m_State >= CClient::STATE_READY)
		{
			if(str_comp(pName, m_aClients[i].m_aName) == 0)
				return -1;
		}

	// set the client name
	str_copy(m_aClients[ClientID].m_aName, pName, MAX_NAME_LENGTH);
	return 0;
}



void CServer::SetClientName(int ClientID, const char *pName)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;
		
	if(!pName)
		return;
		
	char aNameTry[MAX_NAME_LENGTH];
	str_copy(aNameTry, pName, MAX_NAME_LENGTH);
	if(TrySetClientName(ClientID, aNameTry))
	{
		// auto rename
		for(int i = 1;; i++)
		{
			str_format(aNameTry, MAX_NAME_LENGTH, "(%d)%s", i, pName);
			if(TrySetClientName(ClientID, aNameTry) == 0)
				break;
		}
	}
}

void CServer::SetClientScore(int ClientID, int Score)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;
	m_aClients[ClientID].m_Score = Score;
}

void CServer::SetBrowseInfo(const char *pGameType, int Progression)
{
	str_copy(m_aBrowseinfoGametype, pGameType, sizeof(m_aBrowseinfoGametype));
	m_BrowseinfoProgression = Progression;
	if(m_BrowseinfoProgression > 100)
		m_BrowseinfoProgression = 100;
	if(m_BrowseinfoProgression < -1)
		m_BrowseinfoProgression = -1;
}

void CServer::Kick(int ClientID, const char *pReason)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CClient::STATE_EMPTY)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "invalid client id to kick");
		return;
	}
	else if(m_RconClientId == ClientID)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "you can't kick yourself");
 		return;
	}
		
	m_NetServer.Drop(ClientID, pReason);
}

/*int CServer::Tick()
{
	return m_CurrentGameTick;
}*/

int64 CServer::TickStartTime(int Tick)
{
	return m_GameStartTime + (time_freq()*Tick)/SERVER_TICK_SPEED;
}

/*int CServer::TickSpeed()
{
	return SERVER_TICK_SPEED;
}*/

int CServer::Init()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aClients[i].m_State = CClient::STATE_EMPTY;
		m_aClients[i].m_aName[0] = 0;
		m_aClients[i].m_aClan[0] = 0;
		m_aClients[i].m_Snapshots.Init();
	}

	m_CurrentGameTick = 0;
	m_AnnouncementLastLine = 0;

	return 0;
}

int CServer::IsAuthed(int ClientID)
{
	return m_aClients[ClientID].m_Authed;
}

int CServer::GetClientInfo(int ClientID, CClientInfo *pInfo)
{
	dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "client_id is not valid");
	dbg_assert(pInfo != 0, "info can not be null");

	if(m_aClients[ClientID].m_State == CClient::STATE_INGAME)
	{
		pInfo->m_pName = m_aClients[ClientID].m_aName;
		pInfo->m_Latency = m_aClients[ClientID].m_Latency;
		return 1;
	}
	return 0;
}

void CServer::GetClientIP(int ClientID, char *pIPString, int Size)
{
	if(ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State == CClient::STATE_INGAME)
	{
		NETADDR Addr = m_NetServer.ClientAddr(ClientID);
		str_format(pIPString, Size, "%d.%d.%d.%d", Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
	}
}
	

void CServer::SetClientAuthed(int ClientID, int Level) {
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CClient::STATE_READY)
	{
		return;
	}
	m_aClients[ClientID].m_Authed = Level;
}

int *CServer::LatestInput(int ClientId, int *size)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || m_aClients[ClientId].m_State < CServer::CClient::STATE_READY)
		return 0;
	return m_aClients[ClientId].m_LatestInput.m_aData;
}

const char *CServer::ClientName(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || m_aClients[ClientId].m_State == CServer::CClient::STATE_EMPTY)
		return "(invalid client)";
	else if(m_aClients[ClientId].m_State < CServer::CClient::STATE_READY)
		return "(connecting client)";
	return m_aClients[ClientId].m_aName;
}	

bool CServer::ClientIngame(int ClientID)
{
	return ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME;
}

int CServer::SendMsg(CMsgPacker *pMsg, int Flags, int ClientId)
{
	return SendMsgEx(pMsg, Flags, ClientId, false);
}

int CServer::SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System)
{
	CNetChunk Packet;
	if(!pMsg)
		return -1;
		
	mem_zero(&Packet, sizeof(CNetChunk));
	
	Packet.m_ClientID = ClientID;
	Packet.m_pData = pMsg->Data();
	Packet.m_DataSize = pMsg->Size();
	
	// HACK: modify the message id in the packet and store the system flag
	*((unsigned char*)Packet.m_pData) <<= 1;
	if(System)
		*((unsigned char*)Packet.m_pData) |= 1;

	if(Flags&MSGFLAG_VITAL)
		Packet.m_Flags |= NETSENDFLAG_VITAL;
	if(Flags&MSGFLAG_FLUSH)
		Packet.m_Flags |= NETSENDFLAG_FLUSH;
	
	// write message to demo recorder
	if(!(Flags&MSGFLAG_NORECORD))
		m_DemoRecorder.RecordMessage(pMsg->Data(), pMsg->Size());

	if(!(Flags&MSGFLAG_NOSEND))
	{
		if(ClientID == -1)
		{
			// broadcast
			int i;
			for(i = 0; i < MAX_CLIENTS; i++)
				if(m_aClients[i].m_State == CClient::STATE_INGAME)
				{
					Packet.m_ClientID = i;
					m_NetServer.Send(&Packet);
				}
		}
		else
			m_NetServer.Send(&Packet);
	}
	return 0;
}

void CServer::DoSnapshot()
{
	GameServer()->OnPreSnap();
	
	// create snapshot for demo recording
	if(m_DemoRecorder.IsRecording())
	{
		char aData[CSnapshot::MAX_SIZE];
		int SnapshotSize;

		// build snap and possibly add some messages
		m_SnapshotBuilder.Init();
		GameServer()->OnSnap(-1);
		SnapshotSize = m_SnapshotBuilder.Finish(aData);
		
		// write snapshot
		m_DemoRecorder.RecordSnapshot(Tick(), aData, SnapshotSize);
	}

	// create snapshots for all clients
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// client must be ingame to recive snapshots
		if(m_aClients[i].m_State != CClient::STATE_INGAME)
			continue;
			
		// this client is trying to recover, don't spam snapshots
		if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_RECOVER && (Tick()%50) != 0)
			continue;
			
		// this client is trying to recover, don't spam snapshots
		if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_INIT && (Tick()%10) != 0)
			continue;
			
		{
			char aData[CSnapshot::MAX_SIZE];
			CSnapshot *pData = (CSnapshot*)aData;	// Fix compiler warning for strict-aliasing
			char aDeltaData[CSnapshot::MAX_SIZE];
			char aCompData[CSnapshot::MAX_SIZE];
			int SnapshotSize;
			int Crc;
			static CSnapshot EmptySnap;
			CSnapshot *pDeltashot = &EmptySnap;
			int DeltashotSize;
			int DeltaTick = -1;
			int DeltaSize;

			m_SnapshotBuilder.Init();

			GameServer()->OnSnap(i);

			// finish snapshot
			SnapshotSize = m_SnapshotBuilder.Finish(pData);
			Crc = pData->Crc();

			// remove old snapshos
			// keep 3 seconds worth of snapshots
			m_aClients[i].m_Snapshots.PurgeUntil(m_CurrentGameTick-SERVER_TICK_SPEED*3);
			
			// save it the snapshot
			m_aClients[i].m_Snapshots.Add(m_CurrentGameTick, time_get(), SnapshotSize, pData, 0);
			
			// find snapshot that we can preform delta against
			EmptySnap.Clear();
			
			{
				DeltashotSize = m_aClients[i].m_Snapshots.Get(m_aClients[i].m_LastAckedSnapshot, 0, &pDeltashot, 0);
				if(DeltashotSize >= 0)
					DeltaTick = m_aClients[i].m_LastAckedSnapshot;
				else
				{
					// no acked package found, force client to recover rate
					if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_FULL)
						m_aClients[i].m_SnapRate = CClient::SNAPRATE_RECOVER;
				}
			}
			
			// create delta
			DeltaSize = m_SnapshotDelta.CreateDelta(pDeltashot, pData, aDeltaData);
			
			if(DeltaSize)
			{
				// compress it
				int SnapshotSize;
				const int MaxSize = MAX_SNAPSHOT_PACKSIZE;
				int NumPackets;

				SnapshotSize = CVariableInt::Compress(aDeltaData, DeltaSize, aCompData);
				NumPackets = (SnapshotSize+MaxSize-1)/MaxSize;
				
				for(int n = 0, Left = SnapshotSize; Left; n++)
				{
					int Chunk = Left < MaxSize ? Left : MaxSize;
					Left -= Chunk;

					if(NumPackets == 1)
					{
						CMsgPacker Msg(NETMSG_SNAPSINGLE);
						Msg.AddInt(m_CurrentGameTick);
						Msg.AddInt(m_CurrentGameTick-DeltaTick);
						Msg.AddInt(Crc);
						Msg.AddInt(Chunk);
						Msg.AddRaw(&aCompData[n*MaxSize], Chunk);
						SendMsgEx(&Msg, MSGFLAG_FLUSH, i, true);
					}
					else
					{
						CMsgPacker Msg(NETMSG_SNAP);
						Msg.AddInt(m_CurrentGameTick);
						Msg.AddInt(m_CurrentGameTick-DeltaTick);
						Msg.AddInt(NumPackets);
						Msg.AddInt(n);							
						Msg.AddInt(Crc);
						Msg.AddInt(Chunk);
						Msg.AddRaw(&aCompData[n*MaxSize], Chunk);
						SendMsgEx(&Msg, MSGFLAG_FLUSH, i, true);
					}
				}
			}
			else
			{
				CMsgPacker Msg(NETMSG_SNAPEMPTY);
				Msg.AddInt(m_CurrentGameTick);
				Msg.AddInt(m_CurrentGameTick-DeltaTick);
				SendMsgEx(&Msg, MSGFLAG_FLUSH, i, true);
			}
		}
	}

	GameServer()->OnPostSnap();
}


int CServer::NewClientCallback(int ClientId, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	pThis->m_aClients[ClientId].m_State = CClient::STATE_AUTH;
	pThis->m_aClients[ClientId].m_aName[0] = 0;
	pThis->m_aClients[ClientId].m_aClan[0] = 0;
	pThis->m_aClients[ClientId].m_Authed = 0;
	pThis->m_aClients[ClientId].m_AuthTries = 0;
	memset(&pThis->m_aClients[ClientId].m_Addr, 0, sizeof(NETADDR));
	pThis->m_aClients[ClientId].Reset();
	return 0;
}

int CServer::DelClientCallback(int ClientId, const char *pReason, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	
	NETADDR Addr = pThis->m_NetServer.ClientAddr(ClientId);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "client dropped. cid=%d ip=%d.%d.%d.%d reason=\"%s\"",
		ClientId,
		Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3],
		pReason
	);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);

	// notify the mod about the drop
	if(pThis->m_aClients[ClientId].m_State >= CClient::STATE_READY)
		pThis->GameServer()->OnClientDrop(ClientId);
	
	pThis->m_aClients[ClientId].m_State = CClient::STATE_EMPTY;
	pThis->m_aClients[ClientId].m_aName[0] = 0;
	pThis->m_aClients[ClientId].m_aClan[0] = 0;
	pThis->m_aClients[ClientId].m_Authed = 0;
	pThis->m_aClients[ClientId].m_AuthTries = 0;
	memset(&pThis->m_aClients[ClientId].m_Addr, 0, sizeof(NETADDR));
	pThis->m_aClients[ClientId].m_Snapshots.PurgeAll();
	return 0;
}

void CServer::SendMap(int ClientId)
{
	CMsgPacker Msg(NETMSG_MAP_CHANGE);
	Msg.AddString(GetMapName(), 0);
	Msg.AddInt(m_CurrentMapCrc);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientId, true);
}

void CServer::SendRconLine(int ClientId, const char *pLine)
{
	CMsgPacker Msg(NETMSG_RCON_LINE);
	Msg.AddString(pLine, 512);
	SendMsgEx(&Msg, MSGFLAG_VITAL, ClientId, true);
}

void CServer::SendRconLineAuthed(const char *pLine, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	static volatile int ReentryGuard = 0;
	int i;
	
	if(ReentryGuard) return;
	ReentryGuard++;
	
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(pThis->m_aClients[i].m_State != CClient::STATE_EMPTY && pThis->m_aClients[i].m_Authed)
			pThis->SendRconLine(i, pLine);
	}
	
	ReentryGuard--;
}

void CServer::SendRconResponse(const char *pLine, void *pUser)
{
  RconResponseInfo *pInfo = (RconResponseInfo *)pUser;
  CServer *pThis = pInfo->m_Server;
	static volatile int ReentryGuard = 0;

	if(ReentryGuard)
	  return;

  ReentryGuard++;

	if(pThis->m_aClients[pInfo->m_ClientId].m_State != CClient::STATE_EMPTY)
			pThis->SendRconLine(pInfo->m_ClientId, pLine);

	ReentryGuard--;
}

void CServer::ProcessClientPacket(CNetChunk *pPacket)
{
	int ClientId = pPacket->m_ClientID;
	NETADDR Addr;
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	
	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;
	
	if(Unpacker.Error())
		return;
	
	if(m_aClients[ClientId].m_State == CClient::STATE_AUTH)
	{
		if(Sys && Msg == NETMSG_INFO)
		{
			char aVersion[64];
			const char *pPassword;
			str_copy(aVersion, Unpacker.GetString(CUnpacker::SANITIZE_CC), 64);
			if(str_comp(aVersion, GameServer()->NetVersion()) != 0)
			{
				// OH FUCK! wrong version, drop him
				char aReason[256];
				str_format(aReason, sizeof(aReason), "Wrong version. Server is running '%s' and client '%s'", GameServer()->NetVersion(), aVersion);
				m_NetServer.Drop(ClientId, aReason);
				return;
			}
			
			str_copy(m_aClients[ClientId].m_aName, Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), MAX_NAME_LENGTH);
			str_copy(m_aClients[ClientId].m_aClan, Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), MAX_CLANNAME_LENGTH);
			pPassword = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			
			if(g_Config.m_Password[0] != 0 && str_comp(g_Config.m_Password, pPassword) != 0)
			{
				// wrong password
				m_NetServer.Drop(ClientId, "Wrong password");
				return;
			}
			
			// reserved slot
			if(ClientId >= (g_Config.m_SvMaxClients-g_Config.m_SvReservedSlots) && g_Config.m_SvReservedSlotsPass[0] != 0 && strcmp(g_Config.m_SvReservedSlotsPass, pPassword) != 0)
			{
				m_NetServer.Drop(ClientId, "This server is full");
				return;
			}

			m_aClients[ClientId].m_State = CClient::STATE_CONNECTING;
			SendMap(ClientId);
		}
	}
	else
	{
		if(Sys)
		{
			// system message
			if(Msg == NETMSG_REQUEST_MAP_DATA)
			{
				int Chunk = Unpacker.GetInt();
				int ChunkSize = 1024-128;
				int Offset = Chunk * ChunkSize;
				int Last = 0;
				
				// drop faulty map data requests
				if(Chunk < 0 || Offset > m_CurrentMapSize)
					return;
				
				if(Offset+ChunkSize >= m_CurrentMapSize)
				{
					ChunkSize = m_CurrentMapSize-Offset;
					if(ChunkSize < 0)
						ChunkSize = 0;
					Last = 1;
				}
				
				CMsgPacker Msg(NETMSG_MAP_DATA);
				Msg.AddInt(Last);
				Msg.AddInt(m_CurrentMapSize);
				Msg.AddInt(ChunkSize);
				Msg.AddRaw(&m_pCurrentMapData[Offset], ChunkSize);
				SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientId, true);
				
				if(g_Config.m_Debug)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "sending chunk %d with size %d", Chunk, ChunkSize);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
				}
			}
			else if(Msg == NETMSG_READY)
			{
				if(m_aClients[ClientId].m_State == CClient::STATE_CONNECTING)
				{
					Addr = m_NetServer.ClientAddr(ClientId);
					
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "player is ready. ClientId=%x ip=%d.%d.%d.%d",
						ClientId, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
					Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
					m_aClients[ClientId].m_State = CClient::STATE_READY;
					GameServer()->OnClientConnected(ClientId);
					GameServer()->OnSetAuthed(ClientId, m_aClients[ClientId].m_Authed);
				}
			}
			else if(Msg == NETMSG_ENTERGAME)
			{
				if(m_aClients[ClientId].m_State == CClient::STATE_READY)
				{
					Addr = m_NetServer.ClientAddr(ClientId);
					m_aClients[ClientId].m_Addr = Addr;
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "player has entered the game. ClientId=%x ip=%d.%d.%d.%d",
						ClientId, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
					m_aClients[ClientId].m_State = CClient::STATE_INGAME;
					GameServer()->OnClientEnter(ClientId);
				}
			}
			else if(Msg == NETMSG_INPUT)
			{
				CClient::CInput *pInput;
				int64 TagTime;
				
				m_aClients[ClientId].m_LastAckedSnapshot = Unpacker.GetInt();
				int IntendedTick = Unpacker.GetInt();
				int Size = Unpacker.GetInt();
				
				// check for errors
				if(Unpacker.Error() || Size/4 > MAX_INPUT_SIZE)
					return;

				if(m_aClients[ClientId].m_LastAckedSnapshot > 0)
					m_aClients[ClientId].m_SnapRate = CClient::SNAPRATE_FULL;
					
				if(m_aClients[ClientId].m_Snapshots.Get(m_aClients[ClientId].m_LastAckedSnapshot, &TagTime, 0, 0) >= 0)
					m_aClients[ClientId].m_Latency = (int)(((time_get()-TagTime)*1000)/time_freq());

				// add message to report the input timing
				// skip packets that are old
				if(IntendedTick > m_aClients[ClientId].m_LastInputTick)
				{
					int TimeLeft = ((TickStartTime(IntendedTick)-time_get())*1000) / time_freq();
					
					CMsgPacker Msg(NETMSG_INPUTTIMING);
					Msg.AddInt(IntendedTick);
					Msg.AddInt(TimeLeft);
					SendMsgEx(&Msg, 0, ClientId, true);
				}

				m_aClients[ClientId].m_LastInputTick = IntendedTick;

				pInput = &m_aClients[ClientId].m_aInputs[m_aClients[ClientId].m_CurrentInput];
				
				if(IntendedTick <= Tick())
					IntendedTick = Tick()+1;

				pInput->m_GameTick = IntendedTick;
				
				for(int i = 0; i < Size/4; i++)
					pInput->m_aData[i] = Unpacker.GetInt();
				
				mem_copy(m_aClients[ClientId].m_LatestInput.m_aData, pInput->m_aData, MAX_INPUT_SIZE*sizeof(int));
				
				m_aClients[ClientId].m_CurrentInput++;
				m_aClients[ClientId].m_CurrentInput %= 200;
			
				// call the mod with the fresh input data
				if(m_aClients[ClientId].m_State == CClient::STATE_INGAME)
					GameServer()->OnClientDirectInput(ClientId, m_aClients[ClientId].m_LatestInput.m_aData);
			}
			else if(Msg == NETMSG_RCON_CMD)
			{
				const char *pCmd = Unpacker.GetString();
				
				if(Unpacker.Error() == 0/* && m_aClients[ClientId].m_Authed*/)
				{

					char aBuf[256];
					if(m_aClients[ClientId].m_Authed >= 0)
					{
						Console()->RegisterAlternativePrintCallback(0, 0);

						str_format(aBuf, sizeof(aBuf), "'%s' ClientId=%d Level=%d Rcon='%s'", ClientName(ClientId), ClientId, m_aClients[ClientId].m_Authed, pCmd);
						Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);

						Console()->ReleaseAlternativePrintCallback();

						m_RconClientId = ClientId;

						RconResponseInfo Info;
						Info.m_Server = this;
						Info.m_ClientId = ClientId;

						Console()->ExecuteLine(pCmd, m_aClients[ClientId].m_Authed, ClientId, SendRconLineAuthed, this, SendRconResponse, &Info);
						m_RconClientId = -1;
					}
				}
			}
			else if(Msg == NETMSG_RCON_AUTH)
			{
				const char *pPw;
				Unpacker.GetString(); // login name, not used
				pPw = Unpacker.GetString(CUnpacker::SANITIZE_CC);
				
				if(Unpacker.Error() == 0)
					CheckPass(ClientId,pPw);

				/*if(Unpacker.Error() == 0)
				{
					if(g_Config.m_SvRconPassword[0] == 0)
					{
						SendRconLine(ClientId, "No rcon password set on server. Set sv_rcon_password to enable the remote console.");
					}
					else if(str_comp(pPw, g_Config.m_SvRconPassword) == 0)
					{
						CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
						Msg.AddInt(1);
						SendMsgEx(&Msg, MSGFLAG_VITAL, ClientId, true);
						
						m_aClients[ClientId].m_Authed = 1;
						SendRconLine(ClientId, "Authentication successful. Remote console access granted.");
						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "ClientId=%d authed", ClientId);
						Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
					}
					else if(g_Config.m_SvRconMaxTries)
					{
						m_aClients[ClientId].m_AuthTries++;
						char aBuf[128];
						str_format(aBuf, sizeof(aBuf), "Wrong password %d/%d.", m_aClients[ClientId].m_AuthTries, g_Config.m_SvRconMaxTries);
						SendRconLine(ClientId, aBuf);
						if(m_aClients[ClientId].m_AuthTries >= g_Config.m_SvRconMaxTries)
						{
							if(!g_Config.m_SvRconBantime)
								m_NetServer.Drop(ClientId, "Too many remote console authentication tries");
							else
							{
								NETADDR Addr = m_NetServer.ClientAddr(ClientId);
								BanAdd(Addr, g_Config.m_SvRconBantime*60, "Too many remote console authentication tries");
							}
						}
					}
					else
					{
						SendRconLine(ClientId, "Wrong password.");
					}
				}*/
			}
			else if(Msg == NETMSG_PING)
			{
				CMsgPacker Msg(NETMSG_PING_REPLY);
				SendMsgEx(&Msg, 0, ClientId, true);
			}
			else
			{
				char aHex[] = "0123456789ABCDEF";
				char aBuf[512];

				for(int b = 0; b < pPacket->m_DataSize && b < 32; b++)
				{
					aBuf[b*3] = aHex[((const unsigned char *)pPacket->m_pData)[b]>>4];
					aBuf[b*3+1] = aHex[((const unsigned char *)pPacket->m_pData)[b]&0xf];
					aBuf[b*3+2] = ' ';
					aBuf[b*3+3] = 0;
				}

				char aBufMsg[256];
				str_format(aBufMsg, sizeof(aBufMsg), "strange message ClientId=%d msg=%d data_size=%d", ClientId, Msg, pPacket->m_DataSize);
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBufMsg);
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
				
			}
		}
		else
		{
			// game message
			if(m_aClients[ClientId].m_State >= CClient::STATE_READY)
				GameServer()->OnMessage(Msg, &Unpacker, ClientId);
		}
	}
}
	
void CServer::SendServerInfo(NETADDR *pAddr, int Token)
{
	CNetChunk Packet;
	CPacker p;
	char aBuf[128];

	// count the players
	int PlayerCount = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
			PlayerCount++;
	}
	
	p.Reset();

	if(Token >= 0)
	{
		// new token based format
		p.AddRaw(SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));
		str_format(aBuf, sizeof(aBuf), "%d", Token);
		p.AddString(aBuf, 6);
	}
	else
	{
		// old format
		p.AddRaw(SERVERBROWSE_OLD_INFO, sizeof(SERVERBROWSE_OLD_INFO));
	}
	
	p.AddString(GameServer()->Version(), 32);
	p.AddString(g_Config.m_SvName, 64);
	p.AddString(GetMapName(), 32);

	// gametype
	p.AddString(m_aBrowseinfoGametype, 16);

	// flags
	int i = 0;
	if(g_Config.m_Password[0])   // password set
		i |= SERVER_FLAG_PASSWORD;
	str_format(aBuf, sizeof(aBuf), "%d", i);
	p.AddString(aBuf, 2);

	// progression
	str_format(aBuf, sizeof(aBuf), "%d", m_BrowseinfoProgression);
	p.AddString(aBuf, 4);
	
	str_format(aBuf, sizeof(aBuf), "%d", PlayerCount); p.AddString(aBuf, 3);  // num players
	str_format(aBuf, sizeof(aBuf), "%d", max(m_NetServer.MaxClients() - g_Config.m_SvReservedSlots, PlayerCount)); p.AddString(aBuf, 3); // max players

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			p.AddString(ClientName(i), 48);  // player name
			str_format(aBuf, sizeof(aBuf), "%d", m_aClients[i].m_Score); p.AddString(aBuf, 6);  // player score
		}
	}
	
	
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = p.Size();
	Packet.m_pData = p.Data();
	m_NetServer.Send(&Packet);
}

void CServer::UpdateServerInfo()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			NETADDR Addr = m_NetServer.ClientAddr(i);
			SendServerInfo(&Addr, -1); 	// SERVERBROWSE_OLD_INFO
		}
	}
}

int CServer::BanAdd(NETADDR Addr, int Seconds, const char *pReason)
{
	Addr.port = 0;
	char aAddrStr[128];
	net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr));
	char aBuf[256];
	if(Seconds)
		str_format(aBuf, sizeof(aBuf), "banned %s for %d minutes", aAddrStr, Seconds/60);
	else
		str_format(aBuf, sizeof(aBuf), "banned %s for life", aAddrStr);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	return m_NetServer.BanAdd(Addr, Seconds, pReason);	
}

int CServer::BanRemove(NETADDR Addr)
{
	return m_NetServer.BanRemove(Addr);
}
	

void CServer::PumpNetwork()
{
	CNetChunk Packet;

	m_NetServer.Update();
	
	// process packets
	while(m_NetServer.Recv(&Packet))
	{
		if(Packet.m_ClientID == -1)
		{
			// stateless
			if(!m_Register.RegisterProcessPacket(&Packet))
			{
				if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO)+1 &&
					mem_comp(Packet.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					SendServerInfo(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO)]);
				}
				
				
				if(Packet.m_DataSize == sizeof(SERVERBROWSE_OLD_GETINFO) &&
					mem_comp(Packet.m_pData, SERVERBROWSE_OLD_GETINFO, sizeof(SERVERBROWSE_OLD_GETINFO)) == 0)
				{
					SendServerInfo(&Packet.m_Address, -1);
				}
			}
		}
		else
			ProcessClientPacket(&Packet);
	}
}

char *CServer::GetMapName()
{
	// get the name of the map without his path
	char *pMapShortName = &g_Config.m_SvMap[0];
	for(int i = 0; i < str_length(g_Config.m_SvMap)-1; i++)
	{
		if(g_Config.m_SvMap[i] == '/' || g_Config.m_SvMap[i] == '\\')
			pMapShortName = &g_Config.m_SvMap[i+1];
	}
	return pMapShortName;
}

int CServer::LoadMap(const char *pMapName)
{
	//DATAFILE *df;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "maps/%s.map", pMapName);
	
	/*df = datafile_load(buf);
	if(!df)
		return 0;*/
		
	if(!m_pMap->Load(aBuf))
		return 0;
	
	// stop recording when we change map
	m_DemoRecorder.Stop();
	
	// reinit snapshot ids
	m_IDPool.TimeoutIDs();
	
	// get the crc of the map
	m_CurrentMapCrc = m_pMap->Crc();
	char aBufMsg[256];
	str_format(aBufMsg, sizeof(aBufMsg), "%s crc is %08x", aBuf, m_CurrentMapCrc);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBufMsg);
		
	str_copy(m_aCurrentMap, pMapName, sizeof(m_aCurrentMap));
	//map_set(df);
	
	// load compelate map into memory for download
	{
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
		m_CurrentMapSize = (int)io_length(File);
		if(m_pCurrentMapData)
			mem_free(m_pCurrentMapData);
		m_pCurrentMapData = (unsigned char *)mem_alloc(m_CurrentMapSize, 1);
		io_read(File, m_pCurrentMapData, m_CurrentMapSize);
		io_close(File);
	}
	return 1;
}

void CServer::InitEngine(const char *pAppname)
{
	m_Engine.Init(pAppname);
}

void CServer::InitRegister(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole)
{
	m_Register.Init(pNetServer, pMasterServer, pConsole);
}

int CServer::Run()
{
	m_pGameServer = Kernel()->RequestInterface<IGameServer>();
	m_pMap = Kernel()->RequestInterface<IEngineMap>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();

	//snap_init_id();
	net_init();
	
	//
	Console()->RegisterPrintCallback(SendRconLineAuthed, this);
	Console()->RegisterPrintResponseCallback(SendRconLineAuthed, this);
	Console()->RegisterClientOnlineCallback(ClientOnline, this);
	Console()->RegisterCompareClientsCallback(CompareClients, this);

	// load map
	if(!LoadMap(g_Config.m_SvMap))
	{
		dbg_msg("server", "failed to load map. mapname='%s'", g_Config.m_SvMap);
		return -1;
	}
	
	// start server
	// TODO: IPv6 support
	NETADDR BindAddr;
	if(g_Config.m_SvBindaddr[0] && net_host_lookup(g_Config.m_SvBindaddr, &BindAddr, NETTYPE_IPV4) == 0)
	{
		// sweet!
		BindAddr.port = g_Config.m_SvPort;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.port = g_Config.m_SvPort;
	}
	
	
	if(!m_NetServer.Open(BindAddr, g_Config.m_SvMaxClients, g_Config.m_SvMaxClientsPerIP, 0))
	{
		dbg_msg("server", "couldn't open socket. port might already be in use");
		return -1;
	}

	m_NetServer.SetCallbacks(NewClientCallback, DelClientCallback, this);
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "server name is '%s'", g_Config.m_SvName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	
	GameServer()->OnInit();
	str_format(aBuf, sizeof(aBuf), "version %s", GameServer()->NetVersion());
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// process pending commands
	m_pConsole->StoreCommands(false,-1);

	// start game
	{
		int64 ReportTime = time_get();
		int ReportInterval = 3;
	
		m_Lastheartbeat = 0;
		m_GameStartTime = time_get();
	
		if(g_Config.m_Debug)
		{
			str_format(aBuf, sizeof(aBuf), "baseline memory usage %dk", mem_stats()->allocated/1024);
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}

		while(m_RunServer)
		{
			int64 t = time_get();
			int NewTicks = 0;
			
			// load new map TODO: don't poll this
			if(str_comp(g_Config.m_SvMap, m_aCurrentMap) != 0 || m_MapReload)
			{
				m_MapReload = 0;
				
				// load map
				if(LoadMap(g_Config.m_SvMap))
				{
					Console()->ExecuteLine("tune_reset", 4, -1);
					Console()->ExecuteLine("tune gun_speed 1400", 4, -1);
					Console()->ExecuteLine("tune shotgun_curvature 0", 4, -1);
					Console()->ExecuteLine("tune shotgun_speed 500", 4, -1);
					Console()->ExecuteLine("tune shotgun_speeddiff 0", 4, -1);
					Console()->ExecuteLine("tune gun_curvature 0", 4, -1);
					Console()->ExecuteLine("sv_hit 1",4,-1);
					Console()->ExecuteLine("sv_npc 0",4,-1);
					Console()->ExecuteLine("sv_phook 1",4,-1);
					Console()->ExecuteLine("sv_endless_drag 0",4,-1);
					Console()->ExecuteLine("sv_old_laser 0",4,-1);
					// new map loaded
					GameServer()->OnShutdown();
					
					for(int c = 0; c < MAX_CLIENTS; c++)
					{
						if(m_aClients[c].m_State <= CClient::STATE_AUTH)
							continue;
						
						SendMap(c);
						m_aClients[c].Reset();
						m_aClients[c].m_State = CClient::STATE_CONNECTING;
					}
					
					m_GameStartTime = time_get();
					m_CurrentGameTick = 0;
					Kernel()->ReregisterInterface(GameServer());
					GameServer()->OnInit();
					UpdateServerInfo();
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "failed to load map. mapname='%s'", g_Config.m_SvMap);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
					str_copy(g_Config.m_SvMap, m_aCurrentMap, sizeof(g_Config.m_SvMap));
				}
			}
			
			while(t > TickStartTime(m_CurrentGameTick+1))
			{
				m_CurrentGameTick++;
				NewTicks++;
				
				// apply new input
				for(int c = 0; c < MAX_CLIENTS; c++)
				{
					if(m_aClients[c].m_State == CClient::STATE_EMPTY)
						continue;
					for(int i = 0; i < 200; i++)
					{
						if(m_aClients[c].m_aInputs[i].m_GameTick == Tick())
						{
							if(m_aClients[c].m_State == CClient::STATE_INGAME)
								GameServer()->OnClientPredictedInput(c, m_aClients[c].m_aInputs[i].m_aData);
							break;
						}
					}
				}

				GameServer()->OnTick();
			}
			
			// snap game
			if(NewTicks)
			{
				if(g_Config.m_SvHighBandwidth || (m_CurrentGameTick%2) == 0)
					DoSnapshot();
			}
			
			// master server stuff
			m_Register.RegisterUpdate();
	
			PumpNetwork();
	
			if(ReportTime < time_get())
			{
				if(g_Config.m_Debug)
				{
					/*
					static NETSTATS prev_stats;
					NETSTATS stats;
					netserver_stats(net, &stats);
					
					perf_next();
					
					if(config.dbg_pref)
						perf_dump(&rootscope);

					dbg_msg("server", "send=%8d recv=%8d",
						(stats.send_bytes - prev_stats.send_bytes)/reportinterval,
						(stats.recv_bytes - prev_stats.recv_bytes)/reportinterval);
						
					prev_stats = stats;
					*/
				}
	
				ReportTime += time_freq()*ReportInterval;
			}
			
			// wait for incomming data
			net_socket_read_wait(m_NetServer.Socket(), 5);
		}
	}
	// disconnect all clients on shutdown
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
			m_NetServer.Drop(i, "Server shutdown");
	}

	GameServer()->OnShutdown();
	m_pMap->Unload();

	if(m_pCurrentMapData)
		mem_free(m_pCurrentMapData);
	return 0;
}

void CServer::ConKick(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	int Victim = pResult->GetVictim();
	if(pResult->NumArguments() >= 1)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Kicked by (%s)", pResult->GetString(0));
		((CServer *)pUser)->Kick(Victim, aBuf);
	}
	else
		((CServer *)pUser)->Kick(Victim, "Kicked by console");
}

void CServer::ConBan(IConsole::IResult *pResult, void *pUser, int ClientId1)
{
	NETADDR Addr;
	CServer *pServer = (CServer *)pUser;
	const char *pStr = pResult->GetString(0);
	int Minutes = 30;
  	const char *pReason = "No reason given";
	
	if(pResult->NumArguments() > 1)
		Minutes = pResult->GetInteger(1);
	
	if(pResult->NumArguments() > 2)
		pReason = pResult->GetString(2);
	
	if(net_addr_from_str(&Addr, pStr) == 0)
	{
		if(pServer->m_RconClientId >= 0 && pServer->m_RconClientId < MAX_CLIENTS && pServer->m_aClients[pServer->m_RconClientId].m_State != CClient::STATE_EMPTY)
		{
			NETADDR AddrCheck = pServer->m_NetServer.ClientAddr(pServer->m_RconClientId);
			Addr.port = AddrCheck.port = 0;
			if(net_addr_comp(&Addr, &AddrCheck) == 0)
			{
				pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "server", "you can't ban yourself");
				return;
			}
		}
		for(int i=0;i<MAX_CLIENTS;i++)
		{
			NETADDR Temp = pServer->m_NetServer.ClientAddr(i);
			Addr.port = Temp.port = 0;
			if(net_addr_comp(&Addr, &Temp) == 0)
			{
				if ((((CServer *)pUser)->m_aClients[ClientId1].m_Authed > 0) && ((CServer *)pUser)->m_aClients[ClientId1].m_Authed <= ((CServer *)pUser)->m_aClients[i].m_Authed)
				{
					pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "you can't ban an a player with the higher or same rank!");
					return;
				}
			}
		}
		pServer->BanAdd(Addr, Minutes*60, pReason);
	}
	else if(StrAllnum(pStr))
	{
		int ClientId = str_toint(pStr);

		if(ClientId < 0 || ClientId >= MAX_CLIENTS || pServer->m_aClients[ClientId].m_State == CClient::STATE_EMPTY)
		{
			pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "server", "invalid client id");
			return;
		}

		if (ClientId1 != -1 && ((CServer *)pUser)->m_aClients[ClientId1].m_Authed <= ((CServer *)pUser)->m_aClients[ClientId].m_Authed)
			return;

		else if(pServer->m_RconClientId == ClientId)
		{
			pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "server", "you can't ban yourself");
			return;
		}

		Addr = pServer->m_NetServer.ClientAddr(ClientId);
		pServer->BanAdd(Addr, Minutes*60, pReason);
	}
	else
	{
		pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "server", "invalid network address to ban");
		return;
 	}
}

void CServer::ConUnban(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	NETADDR Addr;
	CServer *pServer = (CServer *)pUser;
	const char *pStr = pResult->GetString(0);
	
	if(net_addr_from_str(&Addr, pStr) == 0 && !pServer->BanRemove(Addr))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "unbanned %d.%d.%d.%d", Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
		pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
	else if(StrAllnum(pStr))
	{
		int BanIndex = str_toint(pStr);
		CNetServer::CBanInfo Info;
		if(BanIndex < 0 || !pServer->m_NetServer.BanGet(BanIndex, &Info))
			pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "server", "invalid ban index");
		else if(!pServer->BanRemove(Info.m_Addr))
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "unbanned %d.%d.%d.%d", Info.m_Addr.ip[0], Info.m_Addr.ip[1], Info.m_Addr.ip[2], Info.m_Addr.ip[3]);
			pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		}
	}
	else
		pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "server", "invalid network address");
}

void CServer::ConBans(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	unsigned Now = time_timestamp();
	char aBuf[1024];
	CServer* pServer = (CServer *)pUser;
	
	int Num = pServer->m_NetServer.BanNum();
	for(int i = 0; i < Num; i++)
	{
		CNetServer::CBanInfo Info;
		pServer->m_NetServer.BanGet(i, &Info);
		NETADDR Addr = Info.m_Addr;
		
		if(Info.m_Expires == -1)
		{
			str_format(aBuf, sizeof(aBuf), "#%d %d.%d.%d.%d for life", i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
		}
		else
		{
			unsigned t = Info.m_Expires - Now;
			str_format(aBuf, sizeof(aBuf), "#%d %d.%d.%d.%d for %d minutes and %d seconds", i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], t/60, t%60);
		}
		pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Server", aBuf);
	}
	str_format(aBuf, sizeof(aBuf), "%d ban(s)", Num);
	pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Server", aBuf);
}

void CServer::ConStatus(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	int i;
	NETADDR Addr;
	char aBuf[1024];
	CServer* pServer = (CServer *)pUser;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(pServer->m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			Addr = pServer->m_NetServer.ClientAddr(i);
			if(pServer->m_aClients[i].m_State == CClient::STATE_INGAME)
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%d.%d.%d.%d:%d name='%s' level=%d",
					i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], Addr.port,
					pServer->m_aClients[i].m_aName, pServer->m_aClients[i].m_Authed);
			else
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%d.%d.%d.%d:%d connecting",
					i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], Addr.port);
			pServer->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Server", aBuf);
		}
	}
}

void CServer::ConShutdown(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	((CServer *)pUser)->m_RunServer = 0;
}

void CServer::ConRecord(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	CServer* pServer = (CServer *)pUser;
	char aFilename[128];

	if(pResult->NumArguments())
		str_format(aFilename, sizeof(aFilename), "demos/%s.demo", pResult->GetString(0));
	else
	{
		char aDate[20];
		str_timestamp(aDate, sizeof(aDate));
		str_format(aFilename, sizeof(aFilename), "demos/demo_%s.demo", aDate);
	}
	pServer->m_DemoRecorder.Start(pServer->Storage(), pServer->Console(), aFilename, pServer->GameServer()->NetVersion(), pServer->m_aCurrentMap, pServer->m_CurrentMapCrc, "server");
}

void CServer::ConStopRecord(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	((CServer *)pUser)->m_DemoRecorder.Stop();
}

void CServer::ConMapReload(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	((CServer *)pUser)->m_MapReload = 1;
}

void CServer::ConCmdList(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CServer *pSelf = (CServer *)pUserData;

	if(pResult->NumArguments() == 0)
		pSelf->Console()->List((pSelf->m_aClients[ClientId].m_Authed != 0) ? pSelf->m_aClients[ClientId].m_Authed : -1, CFGFLAG_SERVER);
	else if (pResult->GetInteger(0) == 0)
		pSelf->Console()->List(-1, CFGFLAG_SERVER);
	else
		pSelf->Console()->List(pResult->GetInteger(0), CFGFLAG_SERVER);
}



void CServer::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData, -1);
	if(pResult->NumArguments())
		((CServer *)pUserData)->UpdateServerInfo();
}

void CServer::ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData, -1);
	if(pResult->NumArguments())
		((CServer *)pUserData)->m_NetServer.SetMaxClientsPerIP(pResult->GetInteger(0));
}

void CServer::ConLogin(IConsole::IResult *pResult, void *pUser, int ClientId)
{
	if(pResult->NumArguments())
		((CServer *)pUser)->CheckPass(ClientId, pResult->GetString(0));
	else
		((CServer *)pUser)->SetRconLevel(ClientId, 0);
}

bool CServer::CompareClients(int ClientLevel, int Victim, void *pUser)
{
	CServer* pSelf = (CServer *)pUser;

	if(!ClientOnline(Victim, pSelf))
		return false;

	return clamp(ClientLevel, 0, 4) > clamp(pSelf->m_aClients[Victim].m_Authed, 0, 4);
}

bool CServer::ClientOnline(int ClientId, void *pUser)
{
	CServer* pSelf = (CServer *)pUser;
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return false;
	
	return pSelf->m_aClients[ClientId].m_State != CClient::STATE_EMPTY;
}

void CServer::RegisterCommands()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	
	Console()->Register("kick", "v?t", CFGFLAG_SERVER, ConKick, this, "", 2);
	Console()->Register("ban", "s?ir", CFGFLAG_SERVER|CFGFLAG_STORE, ConBan, this, "", 2);
	Console()->Register("unban", "s", CFGFLAG_SERVER|CFGFLAG_STORE, ConUnban, this, "", 2);
	Console()->Register("bans", "", CFGFLAG_SERVER|CFGFLAG_STORE, ConBans, this, "", 2);
	Console()->Register("status", "", CFGFLAG_SERVER, ConStatus, this, "", 1);
	Console()->Register("shutdown", "", CFGFLAG_SERVER, ConShutdown, this, "", 3);

	Console()->Register("record", "?s", CFGFLAG_SERVER|CFGFLAG_STORE, ConRecord, this, "", 3);
	Console()->Register("stoprecord", "", CFGFLAG_SERVER, ConStopRecord, this, "", 3);
	
	Console()->Register("reload", "", CFGFLAG_SERVER, ConMapReload, this, "", 3);

	Console()->Chain("sv_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("password", ConchainSpecialInfoupdate, this);

	Console()->Chain("sv_max_clients_per_ip", ConchainMaxclientsperipUpdate, this);

	Console()->Register("login", "?s", CFGFLAG_SERVER, ConLogin, this, "Allows you access to rcon if no password is given, or changes your level if a password is given", -1);
	Console()->Register("auth", "?s", CFGFLAG_SERVER, ConLogin, this, "Allows you access to rcon if no password is given, or changes your level if a password is given", -1);

	Console()->Register("cmdlist", "?i", CFGFLAG_SERVER, ConCmdList, this, "Shows you the commands available for your remote console access. Specify the level if you want to see other level's commands", -1);
}	


int CServer::SnapNewID()
{
	return m_IDPool.NewID();
}

void CServer::SnapFreeID(int ID)
{
	m_IDPool.FreeID(ID);
}


void *CServer::SnapNewItem(int Type, int Id, int Size)
{
	dbg_assert(Type >= 0 && Type <=0xffff, "incorrect type");
	dbg_assert(Id >= 0 && Id <=0xffff, "incorrect id");
	return m_SnapshotBuilder.NewItem(Type, Id, Size);		
}

void CServer::SnapSetStaticsize(int ItemType, int Size)
{
	m_SnapshotDelta.SetStaticsize(ItemType, Size);
}

static CServer *CreateServer() { return new CServer(); }

int main(int argc, const char **argv) // ignore_convention
{
#if defined(CONF_FAMILY_WINDOWS)
	for(int i = 1; i < argc; i++) // ignore_convention
	{
		if(str_comp("-s", argv[i]) == 0 || str_comp("--silent", argv[i]) == 0) // ignore_convention
		{
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			break;
		}
	}
#endif

	// init the engine
	dbg_msg("server", "starting...");
	CServer *pServer = CreateServer();
	pServer->InitEngine("Teeworlds");
	
	IKernel *pKernel = IKernel::Create();

	// create the components
	IEngineMap *pEngineMap = CreateEngineMap();
	IGameServer *pGameServer = CreateGameServer();
	IConsole *pConsole = CreateConsole(CFGFLAG_SERVER);
	IEngineMasterServer *pEngineMasterServer = CreateEngineMasterServer();
	IStorage *pStorage = CreateStorage("Teeworlds", argc, argv); // ignore_convention
	IConfig *pConfig = CreateConfig();
	
	pServer->InitRegister(&pServer->m_NetServer, pEngineMasterServer, pConsole);

	{
		bool RegisterFail = false;

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pServer); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMap*>(pEngineMap)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMap*>(pEngineMap));
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pGameServer);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConsole);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pStorage);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConfig);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMasterServer*>(pEngineMasterServer)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMasterServer*>(pEngineMasterServer));
		
		if(RegisterFail)
			return -1;
	}
	
	pConfig->Init();
	pEngineMasterServer->Init(pServer->Engine());
	pEngineMasterServer->Load();
		
	// register all console commands
	pServer->RegisterCommands();
	pGameServer->OnConsoleInit();
	
	pConsole->ExecuteLine("tune_reset", 4, -1);
	pConsole->ExecuteLine("tune gun_speed 1400", 4, -1);
	pConsole->ExecuteLine("tune shotgun_curvature 0", 4, -1);
	pConsole->ExecuteLine("tune shotgun_speed 500", 4, -1);
	pConsole->ExecuteLine("tune shotgun_speeddiff 0", 4, -1);
	pConsole->ExecuteLine("tune gun_curvature 0", 4, -1);
	pConsole->ExecuteLine("sv_hit 1",4,-1);
	pConsole->ExecuteLine("sv_npc 0",4,-1);
	pConsole->ExecuteLine("sv_phook 1",4,-1);
	pConsole->ExecuteLine("sv_endless_drag 0",4,-1);
	pConsole->ExecuteLine("sv_old_laser 0",4,-1);
	// execute autoexec file
	pConsole->ExecuteFile("autoexec.cfg", 0, 0, 0, 0, 4);

	// parse the command line arguments
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	// restore empty config strings to their defaults
	pConfig->RestoreStrings();
	
	pServer->Engine()->InitLogfile();

	// run the server
	pServer->Run();
	
	// free
	delete pServer;
	delete pKernel;
	delete pEngineMap;
	delete pGameServer;
	delete pConsole;
	delete pEngineMasterServer;
	delete pStorage;
	delete pConfig;
	return 0;
}

void CServer::SetRconLevel(int ClientId, int Level)
{
	if(Level < 0)
	{
		dbg_msg("server", "%s set to level 0. ClientId=%x ip=%d.%d.%d.%d",ClientName(ClientId),	ClientId, m_aClients[ClientId].m_Addr.ip[0], m_aClients[ClientId].m_Addr.ip[1], m_aClients[ClientId].m_Addr.ip[2], m_aClients[ClientId].m_Addr.ip[3]);
		CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
		Msg.AddInt(0);
		SendMsgEx(&Msg, MSGFLAG_VITAL, ClientId, true);
		m_aClients[ClientId].m_Authed = Level;
	}
	else
	{
		dbg_msg("server", "%s set to level %d. ClientId=%x ip=%d.%d.%d.%d",ClientName(ClientId), Level, ClientId, m_aClients[ClientId].m_Addr.ip[0], m_aClients[ClientId].m_Addr.ip[1], m_aClients[ClientId].m_Addr.ip[2], m_aClients[ClientId].m_Addr.ip[3]);
		CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
		Msg.AddInt(1);
		SendMsgEx(&Msg, MSGFLAG_VITAL, ClientId, true);
		m_aClients[ClientId].m_Authed = Level;
		GameServer()->OnSetAuthed(ClientId, m_aClients[ClientId].m_Authed);
	}
}

void CServer::CheckPass(int ClientId, const char *pPw)
{

	if(pPw[0] != 0)
	{
		if(g_Config.m_SvRconPasswordHelper[0] == 0 &&
			g_Config.m_SvRconPasswordModer[0] == 0 &&
			g_Config.m_SvRconPasswordAdmin[0] == 0)
		{
			SendRconLine(ClientId, "No rcon password set on server. Set sv_admin_pass/sv_mod_pass/sv_helper_pass to enable the remote console.");
		}
		else
		{
			/*else if(str_comp(pPw, g_Config.m_SvRconPassword) == 0)
			{
				CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
				Msg.AddInt(1);
				SendMsgEx(&Msg, MSGFLAG_VITAL, ClientId, true);

				m_aClients[ClientId].m_Authed = 1;
				SendRconLine(ClientId, "Authentication successful. Remote console access granted.");
				dbg_msg("server", "ClientId=%d authed", ClientId);
			}*/
			int level = -1;
			if(str_comp(pPw, g_Config.m_SvRconPasswordHelper) == 0)
			{
				level = 1;
			}
			else if(str_comp(pPw, g_Config.m_SvRconPasswordModer) == 0)
			{
				level = 2;
			}
			else if(str_comp(pPw, g_Config.m_SvRconPasswordAdmin) == 0)
			{
				level = 3;
			}
			if(level != -1)
			{
				char buf[128]="Authentication successful. Remote console access granted for ClientId=%d with level=%d";
				SetRconLevel(ClientId,level);
				str_format(buf,sizeof(buf),buf,ClientId,level);
				SendRconLine(ClientId, buf);
				dbg_msg("server", "'%s' ClientId=%d authed with Level=%d", ClientName(ClientId), ClientId, level);
			}
			else if(g_Config.m_SvRconMaxTries)
			{
				m_aClients[ClientId].m_AuthTries++;
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Wrong password %d/%d.", m_aClients[ClientId].m_AuthTries, g_Config.m_SvRconMaxTries);
				SendRconLine(ClientId, aBuf);
				if(m_aClients[ClientId].m_AuthTries >= g_Config.m_SvRconMaxTries)
				{
					if(!g_Config.m_SvRconBantime)
						m_NetServer.Drop(ClientId, "Too many remote console authentication tries");
					else
					{
						NETADDR Addr = m_NetServer.ClientAddr(ClientId);
						BanAdd(Addr, g_Config.m_SvRconBantime, "Too many remote console authentication tries");
					}
				}
			}
		}
	}
	/*else if(str_comp(pPw, g_Config.m_SvRconPassword) == 0)
	{
		CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
		Msg.AddInt(1);
		SendMsgEx(&Msg, MSGFLAG_VITAL, ClientId, true);

		m_aClients[ClientId].m_Authed = 1;
		SendRconLine(ClientId, "Authentication successful. Remote console access granted.");
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "ClientId=%d authed", ClientId);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}*/
	else
	{
		char buf[128]="Authentication successful. Remote console access granted for ClientId=%d with level=%d";
		SetRconLevel(ClientId,0);
		str_format(buf,sizeof(buf),buf,ClientId,0);
		SendRconLine(ClientId, buf);
		dbg_msg("server", "'%s' ClientId=%d authed with Level=%d", ClientName(ClientId), ClientId, 0);
	}
}


char *CServer::GetAnnouncementLine(char const *FileName)
{
	IOHANDLE File = m_pStorage->OpenFile(FileName, IOFLAG_READ, IStorage::TYPE_ALL);
	if(File)
	{
		std::vector<char*> v;
		char *pLine;
		CLineReader *lr = new CLineReader();
		lr->Init(File);
		while((pLine = lr->Get()))
			if(str_length(pLine))
				if(pLine[0]!='#')
					v.push_back(pLine);
		if(v.size() == 1)
		{
			m_AnnouncementLastLine = 0;
		}
		else if(!g_Config.m_SvAnnouncementRandom)
		{
			if(m_AnnouncementLastLine >= v.size())
				m_AnnouncementLastLine %= v.size();
		}
		else
		{
			unsigned Rand;
			do
				Rand = rand() % v.size();
			while(Rand == m_AnnouncementLastLine);
				
			m_AnnouncementLastLine = Rand;
		}
		return v[m_AnnouncementLastLine];
	}
	return 0;
}
