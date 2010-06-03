#ifndef ENGINE_SERVER_SERVER_H
#define ENGINE_SERVER_SERVER_H

#include <engine/server.h>

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
		short m_State; // 0 = free, 1 = alloced, 2 = timed
		int m_Timeout;
	};

	CID m_aIDs[MAX_IDS];
		
	int m_FirstFree;
	int m_FirstTimed;
	int m_LastTimed;
	int m_Usage;
	int m_InUsage;
	
public:	

	CSnapIDPool();
	
	void Reset();
	void RemoveFirstTimeout();
	int NewID();
	void TimeoutIDs();
	void FreeID(int Id);
};

class CServer : public IServer
{
	class IGameServer *m_pGameServer;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
public:
	class IGameServer *GameServer() { return m_pGameServer; }
	class IConsole *Console() { return m_pConsole; }
	class IStorage *Storage() { return m_pStorage; }
	class CEngine *Engine() { return &m_Engine; }

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
			int m_GameTick; // the tick that was chosen for the input
		};
	
		// connection state info
		int m_State;
		int m_Latency;
		int m_SnapRate;
		
		int m_LastAckedSnapshot;
		int m_LastInputTick;
		CSnapshotStorage m_Snapshots;
		
		CInput m_LatestInput;
		CInput m_aInputs[200]; // TODO: handle input better
		int m_CurrentInput;
		
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLANNAME_LENGTH];
		int m_Score;
		int m_Authed;
		
		void Reset();
	};
	
	CClient m_aClients[MAX_CLIENTS];

	CSnapshotDelta m_SnapshotDelta;
	CSnapshotBuilder m_SnapshotBuilder;
	CSnapIDPool m_IDPool;
	CNetServer m_NetServer;
	
	IEngineMap *m_pMap;

	int64 m_GameStartTime;
	//int m_CurrentGameTick;
	int m_RunServer;
	int m_MapReload;

	char m_aBrowseinfoGametype[16];
	int m_BrowseinfoProgression;

	int64 m_Lastheartbeat;
	//static NETADDR4 master_server;

	char m_aCurrentMap[64];
	int m_CurrentMapCrc;
	unsigned char *m_pCurrentMapData;
	int m_CurrentMapSize;	
	
	CDemoRecorder m_DemoRecorder;
	CEngine m_Engine;
	CRegister m_Register;
	
	CServer();
	
	int TrySetClientName(int ClientID, const char *pName);

	virtual void SetClientName(int ClientID, const char *pName);
	virtual void SetClientScore(int ClientID, int Score);
	virtual void SetBrowseInfo(const char *pGameType, int Progression);

	void Kick(int ClientID, const char *pReason);

	//int Tick()
	int64 TickStartTime(int Tick);
	//int TickSpeed()

	int Init();

	bool IsAuthed(int ClientID);
	int GetClientInfo(int ClientID, CClientInfo *pInfo);
	void GetClientIP(int ClientID, char *pIPString, int Size);
	const char *ClientName(int ClientId);
	bool ClientIngame(int ClientID);

	int *LatestInput(int ClientId, int *size);

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientId);
	int SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System);

	void DoSnapshot();

	static int NewClientCallback(int ClientId, void *pUser);
	static int DelClientCallback(int ClientId, void *pUser);

	void SendMap(int ClientId);
	void SendRconLine(int ClientId, const char *pLine);
	static void SendRconLineAuthed(const char *pLine, void *pUser);
	
	void ProcessClientPacket(CNetChunk *pPacket);
		
	void SendServerInfo(NETADDR *pAddr, int Token);
	void UpdateServerInfo();

	int BanAdd(NETADDR Addr, int Seconds);
	int BanRemove(NETADDR Addr);
		

	void PumpNetwork();

	int LoadMap(const char *pMapName);

	void InitEngine(const char *pAppname);
	void InitRegister(CNetServer *pNetServer, IEngineMasterServer *pMasterServer);
	int Run();

	static void ConKick(IConsole::IResult *pResult, void *pUser);
	static void ConBan(IConsole::IResult *pResult, void *pUser);
	static void ConUnban(IConsole::IResult *pResult, void *pUser);
	static void ConBans(IConsole::IResult *pResult, void *pUser);
 	static void ConStatus(IConsole::IResult *pResult, void *pUser);
	static void ConShutdown(IConsole::IResult *pResult, void *pUser);
	static void ConRecord(IConsole::IResult *pResult, void *pUser);
	static void ConStopRecord(IConsole::IResult *pResult, void *pUser);
	static void ConMapReload(IConsole::IResult *pResult, void *pUser);
	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void RegisterCommands();
	
	
	virtual int SnapNewID();
	virtual void SnapFreeID(int ID);
	virtual void *SnapNewItem(int Type, int Id, int Size);
	void SnapSetStaticsize(int ItemType, int Size);
};

#endif
