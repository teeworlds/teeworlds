/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_CLIENT_H
#define ENGINE_CLIENT_CLIENT_H


#include <engine/console.h>
#include <engine/editor.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/client.h>
#include <engine/config.h>
#include <engine/serverbrowser.h>
#include <engine/sound.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/map.h>
#include <engine/masterserver.h>
#include <engine/storage.h>

#include <engine/shared/engine.h>
#include <engine/shared/protocol.h>
#include <engine/shared/demo.h>
#include <engine/shared/network.h>

#include "srvbrowse.h"

class CGraph
{
public:
	enum
	{
		// restrictions: Must be power of two
		MAX_VALUES=128,
	};

	float m_Min, m_Max;
	float m_aValues[MAX_VALUES];
	float m_aColors[MAX_VALUES][3];
	int m_Index;

	void Init(float Min, float Max);

	void ScaleMax();
	void ScaleMin();

	void Add(float v, float r, float g, float b);
	void Render(IGraphics *pGraphics, int Font, float x, float y, float w, float h, const char *pDescription);
};


class CSmoothTime
{
	int64 m_Snap;
	int64 m_Current;
	int64 m_Target;

	int64 m_RLast;
	int64 m_TLast;
	CGraph m_Graph;

	int m_SpikeCounter;

	float m_aAdjustSpeed[2]; // 0 = down, 1 = up
public:
	void Init(int64 Target);
	void SetAdjustSpeed(int Direction, float Value);

	int64 Get(int64 Now);

	void UpdateInt(int64 Target);
	void Update(CGraph *pGraph, int64 Target, int TimeLeft, int AdjustDirection);
};


class CFileCollection
{
	enum
	{
		MAX_ENTRIES=1000,
		TIMESTAMP_LENGTH=20,	// _YYYY-MM-DD_HH-MM-SS
	};

	int64 m_aTimestamps[MAX_ENTRIES];
	int m_NumTimestamps;
	int m_MaxEntries;
	char m_aFileDesc[128];
	int m_FileDescLength;
	char m_aFileExt[32];
	int m_FileExtLength;
	char m_aPath[512];
	IStorage *m_pStorage;

	bool IsFilenameValid(const char *pFilename);
	int64 ExtractTimestamp(const char *pTimestring);
	void BuildTimestring(int64 Timestamp, char *pTimestring);

public:
	void Init(IStorage *pStorage, const char *pPath, const char *pFileDesc, const char *pFileExt, int MaxEntries);
	void AddEntry(int64 Timestamp);

	static void FilelistCallback(const char *pFilename, int IsDir, int StorageType, void *pUser);
};


class CClient : public IClient, public CDemoPlayer::IListner
{
	// needed interfaces
	IEditor *m_pEditor;
	IEngineInput *m_pInput;
	IEngineGraphics *m_pGraphics;
	IEngineSound *m_pSound;
	IGameClient *m_pGameClient;
	IEngineMap *m_pMap;
	IConsole *m_pConsole;
	IStorage *m_pStorage;
	IEngineMasterServer *m_pMasterServer;

	enum
	{
		NUM_SNAPSHOT_TYPES=2,
		PREDICTION_MARGIN=1000/50/2, // magic network prediction value
	};

	CNetClient m_NetClient;
	CDemoPlayer m_DemoPlayer;
	CDemoRecorder m_DemoRecorder;
	CEngine m_Engine;
	CServerBrowser m_ServerBrowser;

	char m_aServerAddressStr[256];

	unsigned m_SnapshotParts;
	int64 m_LocalStartTime;

	int m_DebugFont;
	float m_FrameTimeLow;
	float m_FrameTimeHigh;
	int m_Frames;
	NETADDR m_ServerAddress;
	int m_WindowMustRefocus;
	int m_SnapCrcErrors;
	bool m_AutoScreenshotRecycle;

	int m_AckGameTick;
	int m_CurrentRecvTick;
	int m_RconAuthed;

	// version-checking
	char m_aVersionStr[10];

	// pinging
	int64 m_PingStartTime;

	//
	char m_aCurrentMap[256];
	int m_CurrentMapCrc;

	//
	char m_aCmdConnect[256];

	// map download
	char m_aMapdownloadFilename[256];
	char m_aMapdownloadName[256];
	IOHANDLE m_MapdownloadFile;
	int m_MapdownloadChunk;
	int m_MapdownloadCrc;
	int m_MapdownloadAmount;
	int m_MapdownloadTotalsize;

	// time
	CSmoothTime m_GameTime;
	CSmoothTime m_PredictedTime;

	// input
	struct // TODO: handle input better
	{
		int m_aData[MAX_INPUT_SIZE]; // the input data
		int m_Tick; // the tick that the input is for
		int64 m_PredictedTime; // prediction latency when we sent this input
		int64 m_Time;
	} m_aInputs[200];

	int m_CurrentInput;

	// graphs
	CGraph m_InputtimeMarginGraph;
	CGraph m_GametimeMarginGraph;
	CGraph m_FpsGraph;

	// the game snapshots are modifiable by the game
	CSnapshotStorage m_SnapshotStorage;
	CSnapshotStorage::CHolder *m_aSnapshots[NUM_SNAPSHOT_TYPES];

	int m_RecivedSnapshots;
	char m_aSnapshotIncommingData[CSnapshot::MAX_SIZE];

	CSnapshotStorage::CHolder m_aDemorecSnapshotHolders[NUM_SNAPSHOT_TYPES];
	char *m_aDemorecSnapshotData[NUM_SNAPSHOT_TYPES][2][CSnapshot::MAX_SIZE];

	CSnapshotDelta m_SnapshotDelta;

	//
	CServerInfo m_CurrentServerInfo;
	int64 m_CurrentServerInfoRequestTime; // >= 0 should request, == -1 got info

	// version info
	struct
	{
		int m_State;
		CHostLookup m_VersionServeraddr;
	} m_VersionInfo;
public:
	IEngineGraphics *Graphics() { return m_pGraphics; }
	IEngineInput *Input() { return m_pInput; }
	IEngineSound *Sound() { return m_pSound; }
	IGameClient *GameClient() { return m_pGameClient; }
	IEngineMasterServer *MasterServer() { return m_pMasterServer; }
	IStorage *Storage() { return m_pStorage; }

	CClient();

	// ----- send functions -----
	virtual int SendMsg(CMsgPacker *pMsg, int Flags);

	int SendMsgEx(CMsgPacker *pMsg, int Flags, bool System=true);
	void SendInfo();
	void SendEnterGame();
	void SendReady();

	virtual bool RconAuthed();
	void RconAuth(const char *pName, const char *pPassword);
	virtual void Rcon(const char *pCmd);

	virtual bool ConnectionProblems();

	void DirectInput(int *pInput, int Size);
	void SendInput();

	// TODO: OPT: do this alot smarter!
	virtual int *GetInput(int Tick);

	const char *LatestVersion();
	void VersionUpdate();

	// ------ state handling -----
	void SetState(int s);

	// called when the map is loaded and we should init for a new round
	void OnEnterGame();
	virtual void EnterGame();

	virtual void Connect(const char *pAddress);
	void DisconnectWithReason(const char *pReason);
	virtual void Disconnect();


	virtual void GetServerInfo(CServerInfo *pServerInfo);
	void ServerInfoRequest();

	int LoadData();

	// ---

	void *SnapGetItem(int SnapId, int Index, CSnapItem *pItem);
	void SnapInvalidateItem(int SnapId, int Index);
	void *SnapFindItem(int SnapId, int Type, int Id);
	int SnapNumItems(int SnapId);
	void SnapSetStaticsize(int ItemType, int Size);

	void Render();
	void DebugRender();

	virtual void Quit();

	virtual const char *ErrorString();

	const char *LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc);
	const char *LoadMapSearch(const char *pMapName, int WantedCrc);

	static int PlayerScoreComp(const void *a, const void *b);

	void ProcessPacket(CNetChunk *pPacket);

	virtual int MapDownloadAmount() { return m_MapdownloadAmount; }
	virtual int MapDownloadTotalsize() { return m_MapdownloadTotalsize; }

	void PumpNetwork();

	virtual void OnDemoPlayerSnapshot(void *pData, int Size);
	virtual void OnDemoPlayerMessage(void *pData, int Size);

	void Update();

	void InitEngine(const char *pAppname);
	void RegisterInterfaces();
	void InitInterfaces();

	void Run();


	static void Con_Connect(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Disconnect(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Quit(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Minimize(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Ping(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Screenshot(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Rcon(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_RconAuth(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_AddFavorite(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Play(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_Record(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void Con_StopRecord(IConsole::IResult *pResult, void *pUserData, int ClientID);

	void RegisterCommands();

	const char *DemoPlayer_Play(const char *pFilename, int StorageType);
	void DemoRecorder_Init();
	void DemoRecorder_Start(const char *pFilename, bool WithTimestamp);
	void DemoRecorder_HandleAutoStart();
	void DemoRecorder_Stop();

	void AutoScreenshot_Start();
	void AutoScreenshot_Cleanup();

	virtual class CEngine *Engine() { return &m_Engine; }
};
#endif
