// copyright (c) 2007 magnus auvinen, see licence.txt for more info

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
#include <engine/shared/demorec.h>

#include <engine/server.h>
#include <engine/map.h>
#include <engine/console.h>
#include <engine/storage.h>
#include <engine/masterserver.h>
#include <engine/config.h>

#include <mastersrv/mastersrv.h>

#include "register.h"
#include "server.h"

#if defined(CONF_FAMILY_WINDOWS) 
	#define _WIN32_WINNT 0x0500
	#define NOGDI
	#include <windows.h>
#endif

static const char *StrLtrim(const char *pStr)
{
	while(*pStr && *pStr <= 32)
		pStr++;
	return pStr;
}

static void StrRtrim(char *pStr)
{
	int i = str_length(pStr);
	while(i >= 0)
	{
		if(pStr[i] > 32)
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

	Init();
}


int CServer::TrySetClientName(int ClientID, const char *pName)
{
	char aTrimmedName[64];

	// trim the name
	str_copy(aTrimmedName, StrLtrim(pName), sizeof(aTrimmedName));
	StrRtrim(aTrimmedName);
	dbg_msg("", "'%s' -> '%s'", pName, aTrimmedName);
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
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;
		
	if(m_aClients[ClientID].m_State != CClient::STATE_EMPTY)
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

	return 0;
}

bool CServer::IsAuthed(int ClientID)
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
	

int *CServer::LatestInput(int ClientId, int *size)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || m_aClients[ClientId].m_State < CServer::CClient::STATE_READY)
		return 0;
	return m_aClients[ClientId].m_LatestInput.m_aData;
}

const char *CServer::ClientName(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || m_aClients[ClientId].m_State < CServer::CClient::STATE_READY)
		return "(invalid client)";
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
	pThis->m_aClients[ClientId].Reset();
	return 0;
}

int CServer::DelClientCallback(int ClientId, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	
	// notify the mod about the drop
	if(pThis->m_aClients[ClientId].m_State >= CClient::STATE_READY)
		pThis->GameServer()->OnClientDrop(ClientId);
	
	pThis->m_aClients[ClientId].m_State = CClient::STATE_EMPTY;
	pThis->m_aClients[ClientId].m_aName[0] = 0;
	pThis->m_aClients[ClientId].m_aClan[0] = 0;
	pThis->m_aClients[ClientId].m_Authed = 0;
	pThis->m_aClients[ClientId].m_Snapshots.PurgeAll();
	return 0;
}

void CServer::SendMap(int ClientId)
{
	//get the name of the map without his path
	char * pMapShortName = &g_Config.m_SvMap[0];
	for(int i = 0; i < str_length(g_Config.m_SvMap)-1; i++)
	{
		if(g_Config.m_SvMap[i] == '/' || g_Config.m_SvMap[i] == '\\')
			pMapShortName = &g_Config.m_SvMap[i+1];
	}
	
	CMsgPacker Msg(NETMSG_MAP_CHANGE);
	Msg.AddString(pMapShortName, 0);
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
			str_copy(aVersion, Unpacker.GetString(), 64);
			if(str_comp(aVersion, GameServer()->NetVersion()) != 0)
			{
				// OH FUCK! wrong version, drop him
				char aReason[256];
				str_format(aReason, sizeof(aReason), "wrong version. server is running '%s' and client '%s'.", GameServer()->NetVersion(), aVersion);
				m_NetServer.Drop(ClientId, aReason);
				return;
			}
			
			str_copy(m_aClients[ClientId].m_aName, Unpacker.GetString(), MAX_NAME_LENGTH);
			str_copy(m_aClients[ClientId].m_aClan, Unpacker.GetString(), MAX_CLANNAME_LENGTH);
			pPassword = Unpacker.GetString();
			
			if(g_Config.m_Password[0] != 0 && str_comp(g_Config.m_Password, pPassword) != 0)
			{
				// wrong password
				m_NetServer.Drop(ClientId, "wrong password");
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
					dbg_msg("server", "sending chunk %d with size %d", Chunk, ChunkSize);
			}
			else if(Msg == NETMSG_READY)
			{
				if(m_aClients[ClientId].m_State == CClient::STATE_CONNECTING)
				{
					Addr = m_NetServer.ClientAddr(ClientId);
					
					dbg_msg("server", "player is ready. ClientId=%x ip=%d.%d.%d.%d",
						ClientId, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
					m_aClients[ClientId].m_State = CClient::STATE_READY;
					GameServer()->OnClientConnected(ClientId);
				}
			}
			else if(Msg == NETMSG_ENTERGAME)
			{
				if(m_aClients[ClientId].m_State == CClient::STATE_READY)
				{
					Addr = m_NetServer.ClientAddr(ClientId);
					
					dbg_msg("server", "player has entered the game. ClientId=%x ip=%d.%d.%d.%d",
						ClientId, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
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
				
				if(Unpacker.Error() == 0 && m_aClients[ClientId].m_Authed)
				{
					dbg_msg("server", "ClientId=%d rcon='%s'", ClientId, pCmd);
					Console()->ExecuteLine(pCmd);
				}
			}
			else if(Msg == NETMSG_RCON_AUTH)
			{
				const char *pPw;
				Unpacker.GetString(); // login name, not used
				pPw = Unpacker.GetString();
				
				if(Unpacker.Error() == 0)
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
						dbg_msg("server", "ClientId=%d authed", ClientId);
					}
					else
					{
						SendRconLine(ClientId, "Wrong password.");
					}
				}
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

				dbg_msg("server", "strange message ClientId=%d msg=%d data_size=%d", ClientId, Msg, pPacket->m_DataSize);
				dbg_msg("server", "%s", aBuf);
				
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
	p.AddString(g_Config.m_SvMap, 32);

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
	str_format(aBuf, sizeof(aBuf), "%d", m_NetServer.MaxClients()); p.AddString(aBuf, 3); // max players

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			p.AddString(m_aClients[i].m_aName, 48);  // player name
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

int CServer::BanAdd(NETADDR Addr, int Seconds)
{
	return m_NetServer.BanAdd(Addr, Seconds);	
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
	dbg_msg("server", "%s crc is %08x", aBuf, m_CurrentMapCrc);
		
	str_copy(m_aCurrentMap, pMapName, sizeof(m_aCurrentMap));
	//map_set(df);
	
	// load compelate map into memory for download
	{
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_READ);
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

void CServer::InitRegister(CNetServer *pNetServer, IEngineMasterServer *pMasterServer)
{
	m_Register.Init(pNetServer, pMasterServer);
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
	
	dbg_msg("server", "server name is '%s'", g_Config.m_SvName);
	
	GameServer()->OnInit();
	dbg_msg("server", "version %s", GameServer()->NetVersion());

	// start game
	{
		int64 ReportTime = time_get();
		int ReportInterval = 3;
	
		m_Lastheartbeat = 0;
		m_GameStartTime = time_get();
	
		if(g_Config.m_Debug)
			dbg_msg("server", "baseline memory usage %dk", mem_stats()->allocated/1024);

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
					dbg_msg("server", "failed to load map. mapname='%s'", g_Config.m_SvMap);
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
			m_NetServer.Drop(i, "server shutdown");
	}

	GameServer()->OnShutdown();
	m_pMap->Unload();

	if(m_pCurrentMapData)
		mem_free(m_pCurrentMapData);
	return 0;
}

void CServer::ConKick(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->Kick(pResult->GetInteger(0), "kicked by console");
}

void CServer::ConBan(IConsole::IResult *pResult, void *pUser)
{
	NETADDR Addr;
	char aAddrStr[128];
	const char *pStr = pResult->GetString(0);
	int Minutes = 30;
	
	if(pResult->NumArguments() > 1)
		Minutes = pResult->GetInteger(1);
	
	if(net_addr_from_str(&Addr, pStr) == 0)
		((CServer *)pUser)->BanAdd(Addr, Minutes*60);
	else if(StrAllnum(pStr))
	{
		int ClientId = str_toint(pStr);

		if(ClientId < 0 || ClientId >= MAX_CLIENTS || ((CServer *)pUser)->m_aClients[ClientId].m_State == CClient::STATE_EMPTY)
		{
			dbg_msg("server", "invalid client id");
			return;
		}

		NETADDR Addr = ((CServer *)pUser)->m_NetServer.ClientAddr(ClientId);
		((CServer *)pUser)->BanAdd(Addr, Minutes*60);
	}
	
	Addr.port = 0;
	net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr));
	
	if(Minutes)
		dbg_msg("server", "banned %s for %d minutes", aAddrStr, Minutes);
	else
		dbg_msg("server", "banned %s for life", aAddrStr);
}

void CServer::ConUnban(IConsole::IResult *pResult, void *pUser)
{
	NETADDR Addr;
	CServer *pServer = (CServer *)pUser;
	const char *pStr = pResult->GetString(0);
	
	if(net_addr_from_str(&Addr, pStr) == 0)
		pServer->BanRemove(Addr);
	else if(StrAllnum(pStr))
	{
		int BanIndex = str_toint(pStr);
		CNetServer::CBanInfo Info;
		if(BanIndex < 0 || !pServer->m_NetServer.BanGet(BanIndex, &Info))
			dbg_msg("server", "invalid ban index");
		else
			pServer->BanRemove(Info.m_Addr);
	}
	else
		dbg_msg("server", "invalid network address");
}

void CServer::ConBans(IConsole::IResult *pResult, void *pUser)
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
		pServer->Console()->Print(aBuf);
		dbg_msg("server", "%s", aBuf);
	}
	str_format(aBuf, sizeof(aBuf), "%d ban(s)", Num);
	pServer->Console()->Print(aBuf);
	dbg_msg("server", "%s", aBuf);
}

void CServer::ConStatus(IConsole::IResult *pResult, void *pUser)
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
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%d.%d.%d.%d:%d name='%s' score=%d",
					i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], Addr.port,
					pServer->m_aClients[i].m_aName, pServer->m_aClients[i].m_Score);
			else
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%d.%d.%d.%d:%d connecting",
					i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], Addr.port);
			pServer->Console()->Print(aBuf);
			dbg_msg("server", "%s", aBuf);
		}
	}
}

void CServer::ConShutdown(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_RunServer = 0;
}

void CServer::ConRecord(IConsole::IResult *pResult, void *pUser)
{
	char aFilename[512];
	str_format(aFilename, sizeof(aFilename), "demos/%s.demo", pResult->GetString(0));
	((CServer *)pUser)->m_DemoRecorder.Start(((CServer *)pUser)->Storage(), aFilename, ((CServer *)pUser)->GameServer()->NetVersion(), ((CServer *)pUser)->m_aCurrentMap, ((CServer *)pUser)->m_CurrentMapCrc, "server");
}

void CServer::ConStopRecord(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_DemoRecorder.Stop();
}

void CServer::ConMapReload(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_MapReload = 1;
}

void CServer::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->UpdateServerInfo();
}

void CServer::ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->m_NetServer.SetMaxClientsPerIP(pResult->GetInteger(0));
}

void CServer::RegisterCommands()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	
	Console()->Register("kick", "i", CFGFLAG_SERVER, ConKick, this, "");
	Console()->Register("ban", "s?i", CFGFLAG_SERVER, ConBan, this, "");
	Console()->Register("unban", "s", CFGFLAG_SERVER, ConUnban, this, "");
	Console()->Register("bans", "", CFGFLAG_SERVER, ConBans, this, "");
	Console()->Register("status", "", CFGFLAG_SERVER, ConStatus, this, "");
	Console()->Register("shutdown", "", CFGFLAG_SERVER, ConShutdown, this, "");

	Console()->Register("record", "s", CFGFLAG_SERVER, ConRecord, this, "");
	Console()->Register("stoprecord", "", CFGFLAG_SERVER, ConStopRecord, this, "");
	
	Console()->Register("reload", "", CFGFLAG_SERVER, ConMapReload, this, "");

	Console()->Chain("sv_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("password", ConchainSpecialInfoupdate, this);

	Console()->Chain("sv_max_clients_per_ip", ConchainMaxclientsperipUpdate, this);
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
	IStorage *pStorage = CreateStorage("Teeworlds", argv[0]); // ignore_convention
	IConfig *pConfig = CreateConfig();
	
	pServer->InitRegister(&pServer->m_NetServer, pEngineMasterServer);

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
	
	// execute autoexec file
	pConsole->ExecuteFile("autoexec.cfg");

	// parse the command line arguments
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention
	
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

