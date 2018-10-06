/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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
	void FreeID(int ID);
};


class CServerBan : public CNetBan
{
	class CServer *m_pServer;

	template<class T> int BanExt(T *pBanPool, const typename T::CDataType *pData, int Seconds, const char *pReason);

public:
	class CServer *Server() const { return m_pServer; }

	void InitServerBan(class IConsole *pConsole, class IStorage *pStorage, class CServer* pServer);

	virtual int BanAddr(const NETADDR *pAddr, int Seconds, const char *pReason);
	virtual int BanRange(const CNetRange *pRange, int Seconds, const char *pReason);

	static void ConBanExt(class IConsole::IResult *pResult, void *pUser);
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

	enum
	{
		AUTHED_NO=0,
		AUTHED_MOD,
		AUTHED_ADMIN,

		MAX_RCONCMD_SEND=16,
	};

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
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		int m_Score;
		int m_Authed;
		int m_AuthTries;

		const IConsole::CCommandInfo *m_pRconCmdToSend;

		void Reset();
	};

	CClient m_aClients[MAX_CLIENTS];

	CSnapshotDelta m_SnapshotDelta;
	CSnapshotBuilder m_SnapshotBuilder;
	CSnapIDPool m_IDPool;
	CNetServer m_NetServer;
	CEcon m_Econ;
	CServerBan m_ServerBan;

	IEngineMap *m_pMap;

	int64 m_GameStartTime;
	//int m_CurrentGameTick;
	int m_RunServer;
	int m_MapReload;
	int m_RconClientID;
	int m_RconAuthLevel;
	int m_PrintCBIndex;

	int64 m_Lastheartbeat;
	//static NETADDR4 master_server;

	char m_aCurrentMap[64];
	unsigned m_CurrentMapCrc;
	unsigned char *m_pCurrentMapData;
	int m_CurrentMapSize;

	CDemoRecorder m_DemoRecorder;
	CRegister m_Register;
	CMapChecker m_MapChecker;

	CServer();

	int TrySetClientName(int ClientID, const char *pName);

	virtual void SetClientName(int ClientID, const char *pName);
	virtual void SetClientClan(int ClientID, char const *pClan);
	virtual void SetClientCountry(int ClientID, int Country);
	virtual void SetClientScore(int ClientID, int Score);

	void Kick(int ClientID, const char *pReason);

	void DemoRecorder_HandleAutoStart();
	bool DemoRecorder_IsRecording();

	//int Tick()
	int64 TickStartTime(int Tick);
	//int TickSpeed()

	int Init();

	void SetRconCID(int ClientID);
	bool IsAuthed(int ClientID);
	int GetClientInfo(int ClientID, CClientInfo *pInfo);
	void GetClientAddr(int ClientID, char *pAddrStr, int Size);
	const char *ClientName(int ClientID);
	const char *ClientClan(int ClientID);
	int ClientCountry(int ClientID);
	bool ClientIngame(int ClientID);
	int MaxClients() const;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID);
	int SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System);

	void DoSnapshot();

	static int NewClientCallback(int ClientID, bool Legacy, void *pUser);
	static int DelClientCallback(int ClientID, const char *pReason, void *pUser);

	void SendMap(int ClientID);
	void SendConnectionReady(int ClientID);
	void SendRconLine(int ClientID, const char *pLine);
	static void SendRconLineAuthed(const char *pLine, void *pUser);

	void SendRconCmdAdd(const IConsole::CCommandInfo *pCommandInfo, int ClientID);
	void SendRconCmdRem(const IConsole::CCommandInfo *pCommandInfo, int ClientID);
	void UpdateClientRconCommands();

	void ProcessClientPacket(CNetChunk *pPacket);

	void SendServerInfo(const NETADDR *pAddr, int Token);
	void UpdateServerInfo();

	void PumpNetwork();

	char *GetMapName();
	int LoadMap(const char *pMapName);

	void InitRegister(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole);
	int Run();

	static void ConKick(IConsole::IResult *pResult, void *pUser);
	static void ConStatus(IConsole::IResult *pResult, void *pUser);
	static void ConShutdown(IConsole::IResult *pResult, void *pUser);
	static void ConRecord(IConsole::IResult *pResult, void *pUser);
	static void ConStopRecord(IConsole::IResult *pResult, void *pUser);
	static void ConMapReload(IConsole::IResult *pResult, void *pUser);
	static void ConLogout(IConsole::IResult *pResult, void *pUser);
	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainModCommandUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void RegisterCommands();


	virtual int SnapNewID();
	virtual void SnapFreeID(int ID);
	virtual void *SnapNewItem(int Type, int ID, int Size);
	void SnapSetStaticsize(int ItemType, int Size);
};

#endif
