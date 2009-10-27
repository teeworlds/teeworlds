/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <base/system.h>

#include <engine/e_config.h>
#include <engine/e_engine.h>
#include <engine/e_server_interface.h>

#include <engine/e_protocol.h>
#include <engine/e_snapshot.h>

#include <engine/e_compression.h>

#include <engine/e_network.h>
#include <engine/e_config.h>
#include <engine/e_packer.h>
#include <engine/e_datafile.h>
#include <engine/e_demorec.h>

#include <mastersrv/mastersrv.h>

#if defined(CONF_FAMILY_WINDOWS) 
	#define _WIN32_WINNT 0x0500 
	#include <windows.h> 
#endif


extern int register_process_packet(CNetChunk *pPacket);
extern int register_update();

static const char *str_ltrim(const char *str)
{
	while(*str && *str <= 32)
		str++;
	return str;
}

static void str_rtrim(char *str)
{
	int i = str_length(str);
	while(i >= 0)
	{
		if(str[i] > 32)
			break;
		str[i] = 0;
		i--;
	}
}


class CSnapIDPool
{
	enum
	{
		MAX_IDS = 16*1024,
	};

	class CID
	{
	public:
		short m_Next;
		short m_State; /* 0 = free, 1 = alloced, 2 = timed */
		int m_Timeout;
	};

	CID m_aIDs[MAX_IDS];
		
	int m_FirstFree;
	int m_FirstTimed;
	int m_LastTimed;
	int m_Usage;
	int m_InUsage;

public:	

	CSnapIDPool()
	{
		Reset();
	}
	
	void Reset()
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
	

	void RemoveFirstTimeout()
	{
		int NextTimed = m_aIDs[m_FirstTimed].m_Next;
		
		/* add it to the free list */
		m_aIDs[m_FirstTimed].m_Next = m_FirstFree;
		m_aIDs[m_FirstTimed].m_State = 0;
		m_FirstFree = m_FirstTimed;
		
		/* remove it from the timed list */
		m_FirstTimed = NextTimed;
		if(m_FirstTimed == -1)
			m_LastTimed = -1;
			
		m_Usage--;
	}

	int NewID()
	{
		int64 now = time_get();
		
		/* process timed ids */
		while(m_FirstTimed != -1 && m_aIDs[m_FirstTimed].m_Timeout < now)
			RemoveFirstTimeout();
		
		int id = m_FirstFree;
		dbg_assert(id != -1, "id error");
		m_FirstFree = m_aIDs[m_FirstFree].m_Next;
		m_aIDs[id].m_State = 1;
		m_Usage++;
		m_InUsage++;
		return id;
	}

	void TimeoutIDs()
	{
		/* process timed ids */
		while(m_FirstTimed != -1)
			RemoveFirstTimeout();
	}

	void FreeID(int id)
	{
		dbg_assert(m_aIDs[id].m_State == 1, "id is not alloced");

		m_InUsage--;
		m_aIDs[id].m_State = 2;
		m_aIDs[id].m_Timeout = time_get()+time_freq()*5;
		m_aIDs[id].m_Next = -1;
		
		if(m_LastTimed != -1)
		{
			m_aIDs[m_LastTimed].m_Next = id;
			m_LastTimed = id;
		}
		else
		{
			m_FirstTimed = id;
			m_LastTimed = id;
		}
	}
};

#if 0
class IGameServer
{
public:
	/*
		Function: mods_console_init
			TODO
	*/
	virtual void ConsoleInit();

	/*
		Function: Init
			Called when the server is started.
		
		Remarks:
			It's called after the map is loaded so all map items are available.
	*/
	void Init() = 0;

	/*
		Function: Shutdown
			Called when the server quits.
		
		Remarks:
			Should be used to clean up all resources used.
	*/
	void Shutdown() = 0;

	/*
		Function: ClientEnter
			Called when a client has joined the game.
			
		Arguments:
			cid - Client ID. Is 0 - MAX_CLIENTS.
		
		Remarks:
			It's called when the client is finished loading and should enter gameplay.
	*/
	void ClientEnter(int cid) = 0;

	/*
		Function: ClientDrop
			Called when a client drops from the server.

		Arguments:
			cid - Client ID. Is 0 - MAX_CLIENTS
	*/
	void ClientDrop(int cid) = 0;

	/*
		Function: ClientDirectInput
			Called when the server recives new input from a client.

		Arguments:
			cid - Client ID. Is 0 - MAX_CLIENTS.
			input - Pointer to the input data.
			size - Size of the data. (NOT IMPLEMENTED YET)
	*/
	void ClientDirectInput(int cid, void *input) = 0;

	/*
		Function: ClientPredictedInput
			Called when the server applys the predicted input on the client.

		Arguments:
			cid - Client ID. Is 0 - MAX_CLIENTS.
			input - Pointer to the input data.
			size - Size of the data. (NOT IMPLEMENTED YET)
	*/
	void ClientPredictedInput(int cid, void *input) = 0;


	/*
		Function: Tick
			Called with a regular interval to progress the gameplay.
			
		Remarks:
			The SERVER_TICK_SPEED tells the number of ticks per second.
	*/
	void Tick() = 0;

	/*
		Function: Presnap
			Called before the server starts to construct snapshots for the clients.
	*/
	void Presnap() = 0;

	/*
		Function: Snap
			Called to create the snapshot for a client.
		
		Arguments:
			cid - Client ID. Is 0 - MAX_CLIENTS.
		
		Remarks:
			The game should make a series of calls to <snap_new_item> to construct
			the snapshot for the client.
	*/
	void Snap(int cid) = 0;

	/*
		Function: PostSnap
			Called after the server is done sending the snapshots.
	*/
	void PostSnap() = 0;

	/*
		Function: ClientConnected
			TODO
		
		Arguments:
			arg1 - desc
		
		Returns:

		See Also:
			<other_func>
	*/
	void ClientConnected(int client_id) = 0;


	/*
		Function: NetVersion
			TODO
		
		Arguments:
			arg1 - desc
		
		Returns:

		See Also:
			<other_func>
	*/
	const char *NetVersion() = 0;

	/*
		Function: Version
			TODO
		
		Arguments:
			arg1 - desc
		
		Returns:

		See Also:
			<other_func>
	*/
	const char *Version() = 0;

	/*
		Function: Message
			TODO
		
		Arguments:
			arg1 - desc
		
		Returns:

		See Also:
			<other_func>
	*/
	void Message(int msg, int client_id) = 0;
};

#endif
/*
class IServer
{
public:
	BanAdd
	BanRemove
	TickSpeed
	Tick
	Kick
	SetBrowseInfo
	SetClientScore
	SetClientName
	GetClientInfo
	LatestInput
	ClientName
	
	SendMessage()
	
	Map
	
	virtual int NewSnapID() = 0;
	virtual int FreeSnapID(int i) = 0;
};*/

class CServer
{
public:
	/* */
	class CClient
	{
	public:
	
		enum
		{
			STATE_EMPTY = 0,
			STATE_AUTH,
			STATE_CONNECTING,
			STATE_READY,
			STATE_INGAME,
			
			SNAPRATE_INIT=0,
			SNAPRATE_FULL,
			SNAPRATE_RECOVER
		};
	
		class CInput
		{
		public:
			int m_aData[MAX_INPUT_SIZE];
			int m_GameTick; /* the tick that was chosen for the input */
		};
	
		/* connection state info */
		int m_State;
		int m_Latency;
		int m_SnapRate;
		
		int m_LastAckedSnapshot;
		int m_LastInputTick;
		CSnapshotStorage m_Snapshots;
		
		CInput m_LatestInput;
		CInput m_aInputs[200]; /* TODO: handle input better */
		int m_CurrentInput;
		
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLANNAME_LENGTH];
		int m_Score;
		int m_Authed;
		
		void Reset()
		{
			/* reset input */
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
	};
	
	CClient m_aClients[MAX_CLIENTS];

	CSnapshotBuilder m_SnapshotBuilder;
	CSnapIDPool m_IDPool;
	CNetServer m_NetServer;

	int64 m_GameStartTime;
	int m_CurrentTick;
	int m_RunServer;

	char m_aBrowseinfoGametype[16];
	int m_BrowseinfoProgression;

	int64 m_Lastheartbeat;
	/*static NETADDR4 master_server;*/

	char m_aCurrentMap[64];
	int m_CurrentMapCrc;
	unsigned char *m_pCurrentMapData;
	int m_CurrentMapSize;	
	
	CServer()
	{
		m_CurrentTick = 0;
		m_RunServer = 1;

		mem_zero(m_aBrowseinfoGametype, sizeof(m_aBrowseinfoGametype));
		m_BrowseinfoProgression = -1;

		m_pCurrentMapData = 0;
		m_CurrentMapSize = 0;	
	}
	

	int TrySetClientName(int ClientID, const char *pName)
	{
		int i;
		char aTrimmedName[64];

		/* trim the name */
		str_copy(aTrimmedName, str_ltrim(pName), sizeof(aTrimmedName));
		str_rtrim(aTrimmedName);
		dbg_msg("", "'%s' -> '%s'", pName, aTrimmedName);
		pName = aTrimmedName;
		
		
		/* check for empty names */
		if(!pName[0])
			return -1;
		
		/* make sure that two clients doesn't have the same name */
		for(i = 0; i < MAX_CLIENTS; i++)
			if(i != ClientID && m_aClients[i].m_State >= CClient::STATE_READY)
			{
				if(strcmp(pName, m_aClients[i].m_aName) == 0)
					return -1;
			}

		/* set the client name */
		str_copy(m_aClients[ClientID].m_aName, pName, MAX_NAME_LENGTH);
		return 0;
	}



	void SetClientName(int ClientID, const char *pName)
	{
		if(ClientID < 0 || ClientID > MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
			return;
			
		if(!pName)
			return;
			
		char aNameTry[MAX_NAME_LENGTH];
		str_copy(aNameTry, pName, MAX_NAME_LENGTH);
		if(TrySetClientName(ClientID, aNameTry))
		{
			/* auto rename */
			for(int i = 1;; i++)
			{
				str_format(aNameTry, MAX_NAME_LENGTH, "(%d)%s", i, pName);
				if(TrySetClientName(ClientID, aNameTry) == 0)
					break;
			}
		}
	}

	void SetClientScore(int ClientID, int Score)
	{
		if(ClientID < 0 || ClientID > MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
			return;
		m_aClients[ClientID].m_Score = Score;
	}

	void SetBrowseInfo(const char *pGameType, int Progression)
	{
		str_copy(m_aBrowseinfoGametype, pGameType, sizeof(m_aBrowseinfoGametype));
		m_BrowseinfoProgression = Progression;
		if(m_BrowseinfoProgression > 100)
			m_BrowseinfoProgression = 100;
		if(m_BrowseinfoProgression < -1)
			m_BrowseinfoProgression = -1;
	}

	void Kick(int ClientID, const char *pReason)
	{
		if(ClientID < 0 || ClientID > MAX_CLIENTS)
			return;
			
		if(m_aClients[ClientID].m_State != CClient::STATE_EMPTY)
			m_NetServer.Drop(ClientID, pReason);
	}

	int Tick()
	{
		return m_CurrentTick;
	}

	int64 TickStartTime(int Tick)
	{
		return m_GameStartTime + (time_freq()*Tick)/SERVER_TICK_SPEED;
	}

	int TickSpeed()
	{
		return SERVER_TICK_SPEED;
	}

	int Init()
	{
		int i;
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			m_aClients[i].m_State = CClient::STATE_EMPTY;
			m_aClients[i].m_aName[0] = 0;
			m_aClients[i].m_aClan[0] = 0;
			m_aClients[i].m_Snapshots.Init();
		}

		m_CurrentTick = 0;

		return 0;
	}

	int GetClientInfo(int ClientID, CLIENT_INFO *pInfo)
	{
		dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "client_id is not valid");
		dbg_assert(pInfo != 0, "info can not be null");

		if(m_aClients[ClientID].m_State == CClient::STATE_INGAME)
		{
			pInfo->name = m_aClients[ClientID].m_aName;
			pInfo->latency = m_aClients[ClientID].m_Latency;
			return 1;
		}
		return 0;
	}

	int SendMsg(int ClientID)
	{
		const MSG_INFO *pInfo = msg_get_info();
		CNetChunk Packet;
		if(!pInfo)
			return -1;
			
		mem_zero(&Packet, sizeof(CNetChunk));
		
		Packet.m_ClientID = ClientID;
		Packet.m_pData = pInfo->data;
		Packet.m_DataSize = pInfo->size;

		if(pInfo->flags&MSGFLAG_VITAL)
			Packet.m_Flags |= NETSENDFLAG_VITAL;
		if(pInfo->flags&MSGFLAG_FLUSH)
			Packet.m_Flags |= NETSENDFLAG_FLUSH;
		
		/* write message to demo recorder */
		if(!(pInfo->flags&MSGFLAG_NORECORD))
			demorec_record_message(pInfo->data, pInfo->size);

		if(!(pInfo->flags&MSGFLAG_NOSEND))
		{
			if(ClientID == -1)
			{
				/* broadcast */
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

	void DoSnapshot()
	{
		int i;
		
		{
			static PERFORMACE_INFO scope = {"presnap", 0};
			perf_start(&scope);
			mods_presnap();
			perf_end();
		}
		
		/* create snapshot for demo recording */
		if(demorec_isrecording())
		{
			char data[CSnapshot::MAX_SIZE];
			int snapshot_size;

			/* build snap and possibly add some messages */
			m_SnapshotBuilder.Init();
			mods_snap(-1);
			snapshot_size = m_SnapshotBuilder.Finish(data);
			
			/* write snapshot */
			demorec_record_snapshot(Tick(), data, snapshot_size);
		}

		/* create snapshots for all clients */
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			/* client must be ingame to recive snapshots */
			if(m_aClients[i].m_State != CClient::STATE_INGAME)
				continue;
				
			/* this client is trying to recover, don't spam snapshots */
			if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_RECOVER && (Tick()%50) != 0)
				continue;
				
			/* this client is trying to recover, don't spam snapshots */
			if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_INIT && (Tick()%10) != 0)
				continue;
				
			{
				char data[CSnapshot::MAX_SIZE];
				char deltadata[CSnapshot::MAX_SIZE];
				char compdata[CSnapshot::MAX_SIZE];
				int snapshot_size;
				int crc;
				static CSnapshot emptysnap;
				CSnapshot *deltashot = &emptysnap;
				int deltashot_size;
				int delta_tick = -1;
				int deltasize;
				static PERFORMACE_INFO scope = {"build", 0};
				perf_start(&scope);

				m_SnapshotBuilder.Init();

				{
					static PERFORMACE_INFO scope = {"modsnap", 0};
					perf_start(&scope);
					mods_snap(i);
					perf_end();
				}

				/* finish snapshot */
				snapshot_size = m_SnapshotBuilder.Finish(data);
				crc = ((CSnapshot*)data)->Crc();

				/* remove old snapshos */
				/* keep 3 seconds worth of snapshots */
				m_aClients[i].m_Snapshots.PurgeUntil(m_CurrentTick-SERVER_TICK_SPEED*3);
				
				/* save it the snapshot */
				m_aClients[i].m_Snapshots.Add(m_CurrentTick, time_get(), snapshot_size, data, 0);
				
				/* find snapshot that we can preform delta against */
				emptysnap.m_DataSize = 0;
				emptysnap.m_NumItems = 0;
				
				{
					deltashot_size = m_aClients[i].m_Snapshots.Get(m_aClients[i].m_LastAckedSnapshot, 0, &deltashot, 0);
					if(deltashot_size >= 0)
						delta_tick = m_aClients[i].m_LastAckedSnapshot;
					else
					{
						/* no acked package found, force client to recover rate */
						if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_FULL)
							m_aClients[i].m_SnapRate = CClient::SNAPRATE_RECOVER;
					}
				}
				
				/* create delta */
				{
					static PERFORMACE_INFO scope = {"delta", 0};
					perf_start(&scope);
					deltasize = CSnapshot::CreateDelta(deltashot, (CSnapshot*)data, deltadata);
					perf_end();
				}

				
				if(deltasize)
				{
					/* compress it */
					int snapshot_size;
					const int max_size = MAX_SNAPSHOT_PACKSIZE;
					int numpackets;
					int n, left;

					{				
						static PERFORMACE_INFO scope = {"compress", 0};
						/*char buffer[512];*/
						perf_start(&scope);
						snapshot_size = intpack_compress(deltadata, deltasize, compdata);
						
						/*str_hex(buffer, sizeof(buffer), compdata, snapshot_size);
						dbg_msg("", "deltasize=%d -> %d : %s", deltasize, snapshot_size, buffer);*/
						
						perf_end();
					}

					numpackets = (snapshot_size+max_size-1)/max_size;
					
					for(n = 0, left = snapshot_size; left; n++)
					{
						int chunk = left < max_size ? left : max_size;
						left -= chunk;

						if(numpackets == 1)
							msg_pack_start_system(NETMSG_SNAPSINGLE, MSGFLAG_FLUSH);
						else
							msg_pack_start_system(NETMSG_SNAP, MSGFLAG_FLUSH);
							
						msg_pack_int(m_CurrentTick);
						msg_pack_int(m_CurrentTick-delta_tick); /* compressed with */
						
						if(numpackets != 1)
						{
							msg_pack_int(numpackets);
							msg_pack_int(n);
						}
						
						msg_pack_int(crc);
						msg_pack_int(chunk);
						msg_pack_raw(&compdata[n*max_size], chunk);
						msg_pack_end();
						SendMsg(i);
					}
				}
				else
				{
					msg_pack_start_system(NETMSG_SNAPEMPTY, MSGFLAG_FLUSH);
					msg_pack_int(m_CurrentTick);
					msg_pack_int(m_CurrentTick-delta_tick); /* compressed with */
					msg_pack_end();
					SendMsg(i);
				}
				
				perf_end();
			}
		}

		mods_postsnap();
	}


	static int NewClientCallback(int cid, void *pUser)
	{
		CServer *pThis = (CServer *)pUser;
		pThis->m_aClients[cid].m_State = CClient::STATE_AUTH;
		pThis->m_aClients[cid].m_aName[0] = 0;
		pThis->m_aClients[cid].m_aClan[0] = 0;
		pThis->m_aClients[cid].m_Authed = 0;
		pThis->m_aClients[cid].Reset();
		return 0;
	}

	static int DelClientCallback(int cid, void *pUser)
	{
		CServer *pThis = (CServer *)pUser;
		
		/* notify the mod about the drop */
		if(pThis->m_aClients[cid].m_State >= CClient::STATE_READY)
			mods_client_drop(cid);
		
		pThis->m_aClients[cid].m_State = CClient::STATE_EMPTY;
		pThis->m_aClients[cid].m_aName[0] = 0;
		pThis->m_aClients[cid].m_aClan[0] = 0;
		pThis->m_aClients[cid].m_Authed = 0;
		pThis->m_aClients[cid].m_Snapshots.PurgeAll();
		return 0;
	}

	void SendMap(int cid)
	{
		msg_pack_start_system(NETMSG_MAP_CHANGE, MSGFLAG_VITAL|MSGFLAG_FLUSH);
		msg_pack_string(config.sv_map, 0);
		msg_pack_int(m_CurrentMapCrc);
		msg_pack_end();
		server_send_msg(cid);
	}

	void SendRconLine(int cid, const char *pLine)
	{
		msg_pack_start_system(NETMSG_RCON_LINE, MSGFLAG_VITAL);
		msg_pack_string(pLine, 512);
		msg_pack_end();
		server_send_msg(cid);
	}

	static void SendRconLineAuthed(const char *pLine, void *pUser)
	{
		CServer *pThis = (CServer *)pUser;
		static volatile int reentry_guard = 0;
		int i;
		
		if(reentry_guard) return;
		reentry_guard++;
		
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			if(pThis->m_aClients[i].m_State != CClient::STATE_EMPTY && pThis->m_aClients[i].m_Authed)
				pThis->SendRconLine(i, pLine);
		}
		
		reentry_guard--;
	}
	
	void ProcessClientPacket(CNetChunk *pPacket)
	{
		int cid = pPacket->m_ClientID;
		NETADDR addr;
		
		int sys;
		int msg = msg_unpack_start(pPacket->m_pData, pPacket->m_DataSize, &sys);
		
		if(m_aClients[cid].m_State == CClient::STATE_AUTH)
		{
			if(sys && msg == NETMSG_INFO)
			{
				char version[64];
				const char *password;
				str_copy(version, msg_unpack_string(), 64);
				if(strcmp(version, mods_net_version()) != 0)
				{
					/* OH FUCK! wrong version, drop him */
					char reason[256];
					str_format(reason, sizeof(reason), "wrong version. server is running '%s' and client '%s'.", mods_net_version(), version);
					m_NetServer.Drop(cid, reason);
					return;
				}
				
				str_copy(m_aClients[cid].m_aName, msg_unpack_string(), MAX_NAME_LENGTH);
				str_copy(m_aClients[cid].m_aClan, msg_unpack_string(), MAX_CLANNAME_LENGTH);
				password = msg_unpack_string();
				
				if(config.password[0] != 0 && strcmp(config.password, password) != 0)
				{
					/* wrong password */
					m_NetServer.Drop(cid, "wrong password");
					return;
				}
				
				m_aClients[cid].m_State = CClient::STATE_CONNECTING;
				SendMap(cid);
			}
		}
		else
		{
			if(sys)
			{
				/* system message */
				if(msg == NETMSG_REQUEST_MAP_DATA)
				{
					int chunk = msg_unpack_int();
					int chunk_size = 1024-128;
					int offset = chunk * chunk_size;
					int last = 0;
					
					/* drop faulty map data requests */
					if(chunk < 0 || offset > m_CurrentMapSize)
						return;
					
					if(offset+chunk_size >= m_CurrentMapSize)
					{
						chunk_size = m_CurrentMapSize-offset;
						if(chunk_size < 0)
							chunk_size = 0;
						last = 1;
					}
					
					msg_pack_start_system(NETMSG_MAP_DATA, MSGFLAG_VITAL|MSGFLAG_FLUSH);
					msg_pack_int(last);
					msg_pack_int(m_CurrentMapSize);
					msg_pack_int(chunk_size);
					msg_pack_raw(&m_pCurrentMapData[offset], chunk_size);
					msg_pack_end();
					server_send_msg(cid);
					
					if(config.debug)
						dbg_msg("server", "sending chunk %d with size %d", chunk, chunk_size);
				}
				else if(msg == NETMSG_READY)
				{
					if(m_aClients[cid].m_State == CClient::STATE_CONNECTING)
					{
						addr = m_NetServer.ClientAddr(cid);
						
						dbg_msg("server", "player is ready. cid=%x ip=%d.%d.%d.%d",
							cid,
							addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]
							);
						m_aClients[cid].m_State = CClient::STATE_READY;
						mods_connected(cid);
					}
				}
				else if(msg == NETMSG_ENTERGAME)
				{
					if(m_aClients[cid].m_State == CClient::STATE_READY)
					{
						addr = m_NetServer.ClientAddr(cid);
						
						dbg_msg("server", "player has entered the game. cid=%x ip=%d.%d.%d.%d",
							cid,
							addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]
							);
						m_aClients[cid].m_State = CClient::STATE_INGAME;
						mods_client_enter(cid);
					}
				}
				else if(msg == NETMSG_INPUT)
				{
					int tick, size, i;
					CClient::CInput *pInput;
					int64 tagtime;
					
					m_aClients[cid].m_LastAckedSnapshot = msg_unpack_int();
					tick = msg_unpack_int();
					size = msg_unpack_int();
					
					/* check for errors */
					if(msg_unpack_error() || size/4 > MAX_INPUT_SIZE)
						return;

					if(m_aClients[cid].m_LastAckedSnapshot > 0)
						m_aClients[cid].m_SnapRate = CClient::SNAPRATE_FULL;
						
					if(m_aClients[cid].m_Snapshots.Get(m_aClients[cid].m_LastAckedSnapshot, &tagtime, 0, 0) >= 0)
						m_aClients[cid].m_Latency = (int)(((time_get()-tagtime)*1000)/time_freq());

					/* add message to report the input timing */
					/* skip packets that are old */
					if(tick > m_aClients[cid].m_LastInputTick)
					{
						int time_left = ((TickStartTime(tick)-time_get())*1000) / time_freq();
						msg_pack_start_system(NETMSG_INPUTTIMING, 0);
						msg_pack_int(tick);
						msg_pack_int(time_left);
						msg_pack_end();
						server_send_msg(cid);
					}

					m_aClients[cid].m_LastInputTick = tick;

					pInput = &m_aClients[cid].m_aInputs[m_aClients[cid].m_CurrentInput];
					
					if(tick <= server_tick())
						tick = server_tick()+1;

					pInput->m_GameTick = tick;
					
					for(i = 0; i < size/4; i++)
						pInput->m_aData[i] = msg_unpack_int();
					
					mem_copy(m_aClients[cid].m_LatestInput.m_aData, pInput->m_aData, MAX_INPUT_SIZE*sizeof(int));
					
					m_aClients[cid].m_CurrentInput++;
					m_aClients[cid].m_CurrentInput %= 200;
				
					/* call the mod with the fresh input data */
					if(m_aClients[cid].m_State == CClient::STATE_INGAME)
						mods_client_direct_input(cid, m_aClients[cid].m_LatestInput.m_aData);
				}
				else if(msg == NETMSG_RCON_CMD)
				{
					const char *cmd = msg_unpack_string();
					
					if(msg_unpack_error() == 0 && m_aClients[cid].m_Authed)
					{
						dbg_msg("server", "cid=%d rcon='%s'", cid, cmd);
						console_execute_line(cmd);
					}
				}
				else if(msg == NETMSG_RCON_AUTH)
				{
					const char *pw;
					msg_unpack_string(); /* login name, not used */
					pw = msg_unpack_string();
					
					if(msg_unpack_error() == 0)
					{
						if(config.sv_rcon_password[0] == 0)
						{
							SendRconLine(cid, "No rcon password set on server. Set sv_rcon_password to enable the remote console.");
						}
						else if(strcmp(pw, config.sv_rcon_password) == 0)
						{
							msg_pack_start_system(NETMSG_RCON_AUTH_STATUS, MSGFLAG_VITAL);
							msg_pack_int(1);
							msg_pack_end();
							server_send_msg(cid);
							
							m_aClients[cid].m_Authed = 1;
							SendRconLine(cid, "Authentication successful. Remote console access granted.");
							dbg_msg("server", "cid=%d authed", cid);
						}
						else
						{
							SendRconLine(cid, "Wrong password.");
						}
					}
				}
				else if(msg == NETMSG_PING)
				{
					msg_pack_start_system(NETMSG_PING_REPLY, 0);
					msg_pack_end();
					server_send_msg(cid);
				}
				else
				{
					char hex[] = "0123456789ABCDEF";
					char buf[512];
					int b;

					for(b = 0; b < pPacket->m_DataSize && b < 32; b++)
					{
						buf[b*3] = hex[((const unsigned char *)pPacket->m_pData)[b]>>4];
						buf[b*3+1] = hex[((const unsigned char *)pPacket->m_pData)[b]&0xf];
						buf[b*3+2] = ' ';
						buf[b*3+3] = 0;
					}

					dbg_msg("server", "strange message cid=%d msg=%d data_size=%d", cid, msg, pPacket->m_DataSize);
					dbg_msg("server", "%s", buf);
					
				}
			}
			else
			{
				/* game message */
				if(m_aClients[cid].m_State >= CClient::STATE_READY)
					mods_message(msg, cid);
			}
		}
	}
		
	void SendServerInfo(NETADDR *addr, int token)
	{
		CNetChunk Packet;
		CPacker p;
		char buf[128];

		/* count the players */	
		int player_count = 0;
		int i;
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aClients[i].m_State != CClient::STATE_EMPTY)
				player_count++;
		}
		
		p.Reset();

		if(token >= 0)
		{
			/* new token based format */
			p.AddRaw(SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));
			str_format(buf, sizeof(buf), "%d", token);
			p.AddString(buf, 6);
		}
		else
		{
			/* old format */
			p.AddRaw(SERVERBROWSE_OLD_INFO, sizeof(SERVERBROWSE_OLD_INFO));
		}
		
		p.AddString(mods_version(), 32);
		p.AddString(config.sv_name, 64);
		p.AddString(config.sv_map, 32);

		/* gametype */
		p.AddString(m_aBrowseinfoGametype, 16);

		/* flags */
		i = 0;
		if(config.password[0])   /* password set */
			i |= SRVFLAG_PASSWORD;
		str_format(buf, sizeof(buf), "%d", i);
		p.AddString(buf, 2);

		/* progression */
		str_format(buf, sizeof(buf), "%d", m_BrowseinfoProgression);
		p.AddString(buf, 4);
		
		str_format(buf, sizeof(buf), "%d", player_count); p.AddString(buf, 3);  /* num players */
		str_format(buf, sizeof(buf), "%d", m_NetServer.MaxClients()); p.AddString(buf, 3); /* max players */

		for(i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aClients[i].m_State != CClient::STATE_EMPTY)
			{
				p.AddString(m_aClients[i].m_aName, 48);  /* player name */
				str_format(buf, sizeof(buf), "%d", m_aClients[i].m_Score); p.AddString(buf, 6);  /* player score */
			}
		}
		
		
		Packet.m_ClientID = -1;
		Packet.m_Address = *addr;
		Packet.m_Flags = NETSENDFLAG_CONNLESS;
		Packet.m_DataSize = p.Size();
		Packet.m_pData = p.Data();
		m_NetServer.Send(&Packet);
	}

	int BanAdd(NETADDR Addr, int Seconds)
	{
		return m_NetServer.BanAdd(Addr, Seconds);	
	}

	int BanRemove(NETADDR Addr)
	{
		return m_NetServer.BanRemove(Addr);
	}
		

	void PumpNetwork()
	{
		CNetChunk Packet;

		m_NetServer.Update();
		
		/* process packets */
		while(m_NetServer.Recv(&Packet))
		{
			if(Packet.m_ClientID == -1)
			{
				/* stateless */
				if(!register_process_packet(&Packet))
				{
					if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO)+1 &&
						memcmp(Packet.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
					{
						SendServerInfo(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO)]);
					}
					
					
					if(Packet.m_DataSize == sizeof(SERVERBROWSE_OLD_GETINFO) &&
						memcmp(Packet.m_pData, SERVERBROWSE_OLD_GETINFO, sizeof(SERVERBROWSE_OLD_GETINFO)) == 0)
					{
						SendServerInfo(&Packet.m_Address, -1);
					}
				}
			}
			else
				ProcessClientPacket(&Packet);
		}
	}

	int LoadMap(const char *pMapName)
	{
		DATAFILE *df;
		char buf[512];
		str_format(buf, sizeof(buf), "maps/%s.map", pMapName);
		df = datafile_load(buf);
		if(!df)
			return 0;
		
		/* stop recording when we change map */
		demorec_record_stop();
		
		/* reinit snapshot ids */
		m_IDPool.TimeoutIDs();
		
		/* get the crc of the map */
		m_CurrentMapCrc = datafile_crc(buf);
		dbg_msg("server", "%s crc is %08x", buf, m_CurrentMapCrc);
			
		str_copy(m_aCurrentMap, pMapName, sizeof(m_aCurrentMap));
		map_set(df);
		
		/* load compelate map into memory for download */
		{
			IOHANDLE file = engine_openfile(buf, IOFLAG_READ);
			m_CurrentMapSize = (int)io_length(file);
			if(m_pCurrentMapData)
				mem_free(m_pCurrentMapData);
			m_pCurrentMapData = (unsigned char *)mem_alloc(m_CurrentMapSize, 1);
			io_read(file, m_pCurrentMapData, m_CurrentMapSize);
			io_close(file);
		}
		return 1;
	}

	int Run()
	{
		NETADDR bindaddr;

		//snap_init_id();
		net_init();
		
		/* */
		console_register_print_callback(SendRconLineAuthed, this);

		/* load map */
		if(!LoadMap(config.sv_map))
		{
			dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
			return -1;
		}
		
		/* start server */
		/* TODO: IPv6 support */
		if(config.sv_bindaddr[0] && net_host_lookup(config.sv_bindaddr, &bindaddr, NETTYPE_IPV4) == 0)
		{
			/* sweet! */
			bindaddr.port = config.sv_port;
		}
		else
		{
			mem_zero(&bindaddr, sizeof(bindaddr));
			bindaddr.port = config.sv_port;
		}
		
		
		if(!m_NetServer.Open(bindaddr, config.sv_max_clients, 0))
		{
			dbg_msg("server", "couldn't open socket. port might already be in use");
			return -1;
		}

		m_NetServer.SetCallbacks(NewClientCallback, DelClientCallback, this);
		
		dbg_msg("server", "server name is '%s'", config.sv_name);
		
		mods_init();
		dbg_msg("server", "version %s", mods_net_version());

		/* start game */
		{
			int64 reporttime = time_get();
			int reportinterval = 3;
		
			m_Lastheartbeat = 0;
			m_GameStartTime = time_get();
		
			if(config.debug)
				dbg_msg("server", "baseline memory usage %dk", mem_stats()->allocated/1024);

			while(m_RunServer)
			{
				static PERFORMACE_INFO rootscope = {"root", 0};
				int64 t = time_get();
				int new_ticks = 0;
				
				perf_start(&rootscope);
				
				/* load new map TODO: don't poll this */
				if(strcmp(config.sv_map, m_aCurrentMap) != 0 || config.sv_map_reload)
				{
					config.sv_map_reload = 0;
					
					/* load map */
					if(LoadMap(config.sv_map))
					{
						int c;
						
						/* new map loaded */
						mods_shutdown();
						
						for(c = 0; c < MAX_CLIENTS; c++)
						{
							if(m_aClients[c].m_State == CClient::STATE_EMPTY)
								continue;
							
							SendMap(c);
							m_aClients[c].Reset();
							m_aClients[c].m_State = CClient::STATE_CONNECTING;
						}
						
						m_GameStartTime = time_get();
						m_CurrentTick = 0;
						mods_init();
					}
					else
					{
						dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
						config_set_sv_map(&config, m_aCurrentMap);
					}
				}
				
				while(t > TickStartTime(m_CurrentTick+1))
				{
					m_CurrentTick++;
					new_ticks++;
					
					/* apply new input */
					{
						static PERFORMACE_INFO scope = {"input", 0};
						int c, i;
						
						perf_start(&scope);
						
						for(c = 0; c < MAX_CLIENTS; c++)
						{
							if(m_aClients[c].m_State == CClient::STATE_EMPTY)
								continue;
							for(i = 0; i < 200; i++)
							{
								if(m_aClients[c].m_aInputs[i].m_GameTick == server_tick())
								{
									if(m_aClients[c].m_State == CClient::STATE_INGAME)
										mods_client_predicted_input(c, m_aClients[c].m_aInputs[i].m_aData);
									break;
								}
							}
						}
						
						perf_end();
					}
					
					/* progress game */
					{
						static PERFORMACE_INFO scope = {"tick", 0};
						perf_start(&scope);
						mods_tick();
						perf_end();
					}
				}
				
				/* snap game */
				if(new_ticks)
				{
					if(config.sv_high_bandwidth || (m_CurrentTick%2) == 0)
					{
						static PERFORMACE_INFO scope = {"snap", 0};
						perf_start(&scope);
						DoSnapshot();
						perf_end();
					}
				}
				
				/* master server stuff */
				register_update();
		

				{
					static PERFORMACE_INFO scope = {"net", 0};
					perf_start(&scope);
					PumpNetwork();
					perf_end();
				}

				perf_end();
		
				if(reporttime < time_get())
				{
					if(config.debug)
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
		
					reporttime += time_freq()*reportinterval;
				}
				
				/* wait for incomming data */
				net_socket_read_wait(m_NetServer.Socket(), 5);
			}
		}

		mods_shutdown();
		map_unload();

		if(m_pCurrentMapData)
			mem_free(m_pCurrentMapData);
		return 0;
	}

	static void ConKick(void *pResult, void *pUser)
	{
		((CServer *)pUser)->Kick(console_arg_int(pResult, 0), "kicked by console");
	}

	static int str_allnum(const char *str)
	{
		while(*str)
		{
			if(!(*str >= '0' && *str <= '9'))
				return 0;
			str++;
		}
		return 1;
	}

	static void ConBan(void *pResult, void *pUser)
	{
		NETADDR addr;
		char addrstr[128];
		const char *str = console_arg_string(pResult, 0);
		int minutes = 30;
		
		if(console_arg_num(pResult) > 1)
			minutes = console_arg_int(pResult, 1);
		
		if(net_addr_from_str(&addr, str) == 0)
			((CServer *)pUser)->BanAdd(addr, minutes*60);
		else if(str_allnum(str))
		{
			NETADDR addr;
			int cid = atoi(str);

			if(cid < 0 || cid > MAX_CLIENTS || ((CServer *)pUser)->m_aClients[cid].m_State == CClient::STATE_EMPTY)
			{
				dbg_msg("server", "invalid client id");
				return;
			}

			addr = ((CServer *)pUser)->m_NetServer.ClientAddr(cid);
			((CServer *)pUser)->BanAdd(addr, minutes*60);
		}
		
		addr.port = 0;
		net_addr_str(&addr, addrstr, sizeof(addrstr));
		
		if(minutes)
			dbg_msg("server", "banned %s for %d minutes", addrstr, minutes);
		else
			dbg_msg("server", "banned %s for life", addrstr);
	}

	static void ConUnban(void *result, void *pUser)
	{
		NETADDR addr;
		const char *str = console_arg_string(result, 0);
		
		if(net_addr_from_str(&addr, str) == 0)
			((CServer *)pUser)->BanRemove(addr);
		else
			dbg_msg("server", "invalid network address");
	}

	static void ConBans(void *pResult, void *pUser)
	{
		unsigned now = time_timestamp();
		
		int num = ((CServer *)pUser)->m_NetServer.BanNum();
		for(int i = 0; i < num; i++)
		{
			CNetServer::CBanInfo Info;
			((CServer *)pUser)->m_NetServer.BanGet(i, &Info);
			NETADDR Addr = Info.m_Addr;
			
			if(Info.m_Expires == -1)
			{
				dbg_msg("server", "#%d %d.%d.%d.%d for life", i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3]);
			}
			else
			{
				unsigned t = Info.m_Expires - now;
				dbg_msg("server", "#%d %d.%d.%d.%d for %d minutes and %d seconds", i, Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], t/60, t%60);
			}
		}
		dbg_msg("server", "%d ban(s)", num);
	}

	static void ConStatus(void *pResult, void *pUser)
	{
		int i;
		NETADDR addr;
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			if(((CServer *)pUser)->m_aClients[i].m_State == CClient::STATE_INGAME)
			{
				addr = ((CServer *)pUser)->m_NetServer.ClientAddr(i);
				dbg_msg("server", "id=%d addr=%d.%d.%d.%d:%d name='%s' score=%d",
					i, addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], addr.port,
					((CServer *)pUser)->m_aClients[i].m_aName, ((CServer *)pUser)->m_aClients[i].m_Score);
			}
		}
	}

	static void ConShutdown(void *pResult, void *pUser)
	{
		((CServer *)pUser)->m_RunServer = 0;
	}

	static void ConRecord(void *pResult, void *pUser)
	{
		char filename[512];
		str_format(filename, sizeof(filename), "demos/%s.demo", console_arg_string(pResult, 0));
		demorec_record_start(filename, mods_net_version(), ((CServer *)pUser)->m_aCurrentMap, ((CServer *)pUser)->m_CurrentMapCrc, "server");
	}

	static void ConStopRecord(void *pResult, void *pUser)
	{
		demorec_record_stop();
	}
	

	void RegisterCommands()
	{
		MACRO_REGISTER_COMMAND("kick", "i", CFGFLAG_SERVER, ConKick, this, "");
		MACRO_REGISTER_COMMAND("ban", "s?i", CFGFLAG_SERVER, ConBan, this, "");
		MACRO_REGISTER_COMMAND("unban", "s", CFGFLAG_SERVER, ConUnban, this, "");
		MACRO_REGISTER_COMMAND("bans", "", CFGFLAG_SERVER, ConBans, this, "");
		MACRO_REGISTER_COMMAND("status", "", CFGFLAG_SERVER, ConStatus, this, "");
		MACRO_REGISTER_COMMAND("shutdown", "", CFGFLAG_SERVER, ConShutdown, this, "");

		MACRO_REGISTER_COMMAND("record", "s", CFGFLAG_SERVER, ConRecord, this, "");
		MACRO_REGISTER_COMMAND("stoprecord", "", CFGFLAG_SERVER, ConStopRecord, this, "");
	}	
	
};


// UGLY UGLY HACK for now
CServer g_Server;
CNetServer *m_pNetServer;

int server_tick() { return g_Server.Tick(); }
int server_tickspeed() { return g_Server.TickSpeed(); }
int snap_new_id() { return g_Server.m_IDPool.NewID(); }
void snap_free_id(int id) { return g_Server.m_IDPool.FreeID(id); }
int server_send_msg(int client_id) { return g_Server.SendMsg(client_id); }
void server_setbrowseinfo(const char *game_type, int progression) { g_Server.SetBrowseInfo(game_type, progression); }
void server_setclientname(int client_id, const char *name) { g_Server.SetClientName(client_id, name); }
void server_setclientscore(int client_id, int score) { g_Server.SetClientScore(client_id, score); }

int server_getclientinfo(int client_id, CLIENT_INFO *info) { return g_Server.GetClientInfo(client_id, info); }

void *snap_new_item(int type, int id, int size)
{
	dbg_assert(type >= 0 && type <=0xffff, "incorrect type");
	dbg_assert(id >= 0 && id <=0xffff, "incorrect id");
	return g_Server.m_SnapshotBuilder.NewItem(type, id, size);
}

int *server_latestinput(int client_id, int *size)
{
	if(client_id < 0 || client_id > MAX_CLIENTS || g_Server.m_aClients[client_id].m_State < CServer::CClient::STATE_READY)
		return 0;
	return g_Server.m_aClients[client_id].m_LatestInput.m_aData;
}

const char *server_clientname(int client_id)
{
	if(client_id < 0 || client_id > MAX_CLIENTS || g_Server.m_aClients[client_id].m_State < CServer::CClient::STATE_READY)
		return "(invalid client)";
	return g_Server.m_aClients[client_id].m_aName;
}

#if 0


static void reset_client(int cid)
{
	/* reset input */
	int i;
	for(i = 0; i < 200; i++)
		m_aClients[cid].m_aInputs[i].m_GameTick = -1;
	m_aClients[cid].m_CurrentInput = 0;
	mem_zero(&m_aClients[cid].m_Latestinput, sizeof(m_aClients[cid].m_Latestinput));

	m_aClients[cid].m_Snapshots.PurgeAll();
	m_aClients[cid].m_LastAckedSnapshot = -1;
	m_aClients[cid].m_LastInputTick = -1;
	m_aClients[cid].m_SnapRate = CClient::SNAPRATE_INIT;
	m_aClients[cid].m_Score = 0;

}


static int new_client_callback(int cid, void *user)
{
	m_aClients[cid].state = CClient::STATE_AUTH;
	m_aClients[cid].name[0] = 0;
	m_aClients[cid].clan[0] = 0;
	m_aClients[cid].authed = 0;
	reset_client(cid);
	return 0;
}

static int del_client_callback(int cid, void *user)
{
	/* notify the mod about the drop */
	if(m_aClients[cid].state >= CClient::STATE_READY)
		mods_client_drop(cid);
	
	m_aClients[cid].state = CClient::STATE_EMPTY;
	m_aClients[cid].name[0] = 0;
	m_aClients[cid].clan[0] = 0;
	m_aClients[cid].authed = 0;
	m_aClients[cid].snapshots.PurgeAll();
	return 0;
}
static void server_send_map(int cid)
{
	msg_pack_start_system(NETMSG_MAP_CHANGE, MSGFLAG_VITAL|MSGFLAG_FLUSH);
	msg_pack_string(config.sv_map, 0);
	msg_pack_int(current_map_crc);
	msg_pack_end();
	server_send_msg(cid);
}

static void server_send_rcon_line(int cid, const char *line)
{
	msg_pack_start_system(NETMSG_RCON_LINE, MSGFLAG_VITAL);
	msg_pack_string(line, 512);
	msg_pack_end();
	server_send_msg(cid);
}

static void server_send_rcon_line_authed(const char *line, void *user_data)
{
	static volatile int reentry_guard = 0;
	int i;
	
	if(reentry_guard) return;
	reentry_guard++;
	
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].state != CClient::STATE_EMPTY && m_aClients[i].authed)
			server_send_rcon_line(i, line);
	}
	
	reentry_guard--;
}

static void server_pump_network()
{
	CNetChunk Packet;

	m_NetServer.Update();
	
	/* process packets */
	while(m_NetServer.Recv(&Packet))
	{
		if(Packet.m_ClientID == -1)
		{
			/* stateless */
			if(!register_process_packet(&Packet))
			{
				if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO)+1 &&
					memcmp(Packet.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					server_send_serverinfo(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO)]);
				}
				
				
				if(Packet.m_DataSize == sizeof(SERVERBROWSE_OLD_GETINFO) &&
					memcmp(Packet.m_pData, SERVERBROWSE_OLD_GETINFO, sizeof(SERVERBROWSE_OLD_GETINFO)) == 0)
				{
					server_send_serverinfo(&Packet.m_Address, -1);
				}
			}
		}
		else
			server_process_client_packet(&Packet);
	}
}

static int server_load_map(const char *mapname)
{
	DATAFILE *df;
	char buf[512];
	str_format(buf, sizeof(buf), "maps/%s.map", mapname);
	df = datafile_load(buf);
	if(!df)
		return 0;
	
	/* stop recording when we change map */
	demorec_record_stop();
	
	/* reinit snapshot ids */
	snap_timeout_ids();
	
	/* get the crc of the map */
	current_map_crc = datafile_crc(buf);
	dbg_msg("server", "%s crc is %08x", buf, current_map_crc);
		
	str_copy(current_map, mapname, sizeof(current_map));
	map_set(df);
	
	/* load compelate map into memory for download */
	{
		IOHANDLE file = engine_openfile(buf, IOFLAG_READ);
		current_map_size = (int)io_length(file);
		if(current_map_data)
			mem_free(current_map_data);
		current_map_data = (unsigned char *)mem_alloc(current_map_size, 1);
		io_read(file, current_map_data, current_map_size);
		io_close(file);
	}
	return 1;
}

static int server_run()
{
	NETADDR bindaddr;

	snap_init_id();
	net_init();
	
	/* */
	console_register_print_callback(server_send_rcon_line_authed, 0);

	/* load map */
	if(!server_load_map(config.sv_map))
	{
		dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
		return -1;
	}
	
	/* start server */
	/* TODO: IPv6 support */
	if(config.sv_bindaddr[0] && net_host_lookup(config.sv_bindaddr, &bindaddr, NETTYPE_IPV4) == 0)
	{
		/* sweet! */
		bindaddr.port = config.sv_port;
	}
	else
	{
		mem_zero(&bindaddr, sizeof(bindaddr));
		bindaddr.port = config.sv_port;
	}
	
	
	if(!m_NetServer.Open(bindaddr, config.sv_max_clients, 0))
	{
		dbg_msg("server", "couldn't open socket. port might already be in use");
		return -1;
	}

	m_NetServer.SetCallbacks(new_client_callback, del_client_callback, this);
	
	dbg_msg("server", "server name is '%s'", config.sv_name);
	
	mods_init();
	dbg_msg("server", "version %s", mods_net_version());

	/* start game */
	{
		int64 reporttime = time_get();
		int reportinterval = 3;
	
		lastheartbeat = 0;
		game_start_time = time_get();
	
		if(config.debug)
			dbg_msg("server", "baseline memory usage %dk", mem_stats()->allocated/1024);

		while(run_server)
		{
			static PERFORMACE_INFO rootscope = {"root", 0};
			int64 t = time_get();
			int new_ticks = 0;
			
			perf_start(&rootscope);
			
			/* load new map TODO: don't poll this */
			if(strcmp(config.sv_map, current_map) != 0 || config.sv_map_reload)
			{
				config.sv_map_reload = 0;
				
				/* load map */
				if(server_load_map(config.sv_map))
				{
					int c;
					
					/* new map loaded */
					mods_shutdown();
					
					for(c = 0; c < MAX_CLIENTS; c++)
					{
						if(m_aClients[c].state == CClient::STATE_EMPTY)
							continue;
						
						server_send_map(c);
						reset_client(c);
						m_aClients[c].state = CClient::STATE_CONNECTING;
					}
					
					game_start_time = time_get();
					current_tick = 0;
					mods_init();
				}
				else
				{
					dbg_msg("server", "failed to load map. mapname='%s'", config.sv_map);
					config_set_sv_map(&config, current_map);
				}
			}
			
			while(t > server_tick_start_time(current_tick+1))
			{
				current_tick++;
				new_ticks++;
				
				/* apply new input */
				{
					static PERFORMACE_INFO scope = {"input", 0};
					int c, i;
					
					perf_start(&scope);
					
					for(c = 0; c < MAX_CLIENTS; c++)
					{
						if(m_aClients[c].state == CClient::STATE_EMPTY)
							continue;
						for(i = 0; i < 200; i++)
						{
							if(m_aClients[c].inputs[i].game_tick == server_tick())
							{
								if(m_aClients[c].state == CClient::STATE_INGAME)
									mods_client_predicted_input(c, m_aClients[c].inputs[i].data);
								break;
							}
						}
					}
					
					perf_end();
				}
				
				/* progress game */
				{
					static PERFORMACE_INFO scope = {"tick", 0};
					perf_start(&scope);
					mods_tick();
					perf_end();
				}
			}
			
			/* snap game */
			if(new_ticks)
			{
				if(config.sv_high_bandwidth || (current_tick%2) == 0)
				{
					static PERFORMACE_INFO scope = {"snap", 0};
					perf_start(&scope);
					server_do_snap();
					perf_end();
				}
			}
			
			/* master server stuff */
			register_update();
	

			{
				static PERFORMACE_INFO scope = {"net", 0};
				perf_start(&scope);
				server_pump_network();
				perf_end();
			}

			perf_end();
	
			if(reporttime < time_get())
			{
				if(config.debug)
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
	
				reporttime += time_freq()*reportinterval;
			}
			
			/* wait for incomming data */
			net_socket_read_wait(m_NetServer.Socket(), 5);
		}
	}

	mods_shutdown();
	map_unload();

	if(current_map_data)
		mem_free(current_map_data);
	return 0;
}


#endif

int main(int argc, char **argv)
{
	
	m_pNetServer = &g_Server.m_NetServer;
#if defined(CONF_FAMILY_WINDOWS)
	int i;
	for(i = 1; i < argc; i++)
	{
		if(strcmp("-s", argv[i]) == 0 || strcmp("--silent", argv[i]) == 0)
		{
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			break;
		}
	}
#endif

	/* init the engine */
	dbg_msg("server", "starting...");
	engine_init("Teeworlds");
	
	/* register all console commands */
	
	g_Server.RegisterCommands();
	mods_console_init();
	
	/* parse the command line arguments */
	engine_parse_arguments(argc, argv);
	
	/* run the server */
	g_Server.Run();
	return 0;
}

