/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_CLIENT_H
#define ENGINE_CLIENT_CLIENT_H

class CGraph
{
public:
	enum
	{
		// restrictions: Must be power of two
		MAX_VALUES=128,
	};

	float m_Min, m_Max;
	float m_MinRange, m_MaxRange;
	float m_aValues[MAX_VALUES];
	float m_aColors[MAX_VALUES][3];
	int m_Index;

	void Init(float Min, float Max);

	void ScaleMax();
	void ScaleMin();

	void Add(float v, float r, float g, float b);
	void Render(IGraphics *pGraphics, IGraphics::CTextureHandle FontTexture, float x, float y, float w, float h, const char *pDescription);
};


class CSmoothTime
{
	int64 m_Snap;
	int64 m_Current;
	int64 m_Target;

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


class CClient : public IClient, public CDemoPlayer::IListner
{
	// needed interfaces
	IEngine *m_pEngine;
	IEditor *m_pEditor;
	IEngineInput *m_pInput;
	IEngineGraphics *m_pGraphics;
	IEngineSound *m_pSound;
	IGameClient *m_pGameClient;
	IEngineMap *m_pMap;
	IEngineMod *m_pMod;
	IConsole *m_pConsole;
	IStorage *m_pStorage;
	IEngineMasterServer *m_pMasterServer;

	enum
	{
		NUM_SNAPSHOT_TYPES=2,
		PREDICTION_MARGIN=1000/50/2, // magic network prediction value
	};

	class CNetClient m_NetClient;
	class CNetClient m_ContactClient;
	class CDemoPlayer m_DemoPlayer;
	class CDemoRecorder m_DemoRecorder;
	class CServerBrowser m_ServerBrowser;
	class CFriends m_Friends;
	class CMapChecker m_MapChecker;

	char m_aServerAddressStr[256];

	unsigned m_SnapshotParts;
	int64 m_LocalStartTime;

	IGraphics::CTextureHandle m_DebugFont;
	
	int64 m_LastRenderTime;
	float m_RenderFrameTimeLow;
	float m_RenderFrameTimeHigh;
	int m_RenderFrames;

	NETADDR m_ServerAddress;
	int m_WindowMustRefocus;
	int m_SnapCrcErrors;
	bool m_AutoScreenshotRecycle;
	bool m_EditorActive;
	bool m_SoundInitFailed;
	bool m_ResortServerBrowser;
	bool m_RecordGameMessage;

	int m_AckGameTick;
	int m_CurrentRecvTick;
	int m_RconAuthed;
	int m_UseTempRconCommands;

	// version-checking
	char m_aVersionStr[10];

	// pinging
	int64 m_PingStartTime;

	//
	char m_aCurrentMap[256];
	unsigned m_CurrentMapCrc;

	//
	char m_aCmdConnect[256];

	// map download
	char m_aMapdownloadFilename[256];
	char m_aMapdownloadName[256];
	IOHANDLE m_MapdownloadFile;
	int m_MapdownloadChunk;
	int m_MapdownloadChunkNum;
	int m_MapDownloadChunkSize;
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
	class CSnapshotStorage m_SnapshotStorage;
	CSnapshotStorage::CHolder *m_aSnapshots[NUM_SNAPSHOT_TYPES];

	int m_RecivedSnapshots;
	char m_aSnapshotIncommingData[CSnapshot::MAX_SIZE];

	class CSnapshotStorage::CHolder m_aDemorecSnapshotHolders[NUM_SNAPSHOT_TYPES];
	char *m_aDemorecSnapshotData[NUM_SNAPSHOT_TYPES][2][CSnapshot::MAX_SIZE];
	class CSnapshotBuilder m_DemoRecSnapshotBuilder;

	class CSnapshotDelta m_SnapshotDelta;

	//
	class CServerInfo m_CurrentServerInfo;

	// version info
	struct CVersionInfo
	{
		enum
		{
			STATE_INIT=0,
			STATE_START,
			STATE_READY,
		};

		int m_State;
		class CHostLookup m_VersionServeraddr;
	} m_VersionInfo;

	int64 TickStartTime(int Tick);

public:
	IEngine *Engine() { return m_pEngine; }
	IEngineGraphics *Graphics() { return m_pGraphics; }
	IEngineInput *Input() { return m_pInput; }
	IEngineSound *Sound() { return m_pSound; }
	IGameClient *GameClient() { return m_pGameClient; }
	IEngineMasterServer *MasterServer() { return m_pMasterServer; }
	IStorage *Storage() { return m_pStorage; }

	CClient();

	// ----- send functions -----
	virtual int SendMsg(CMsgPacker *pMsg, int Flags);

	void SendInfo();
	void SendEnterGame();
	void SendReady();

	virtual bool RconAuthed() const { return m_RconAuthed != 0; }
	virtual bool UseTempRconCommands() const { return m_UseTempRconCommands != 0; }
	void RconAuth(const char *pName, const char *pPassword);
	virtual void Rcon(const char *pCmd);

	virtual bool ConnectionProblems() const;

	virtual bool SoundInitFailed() const { return m_SoundInitFailed; }

	virtual IGraphics::CTextureHandle GetDebugFont() const { return m_DebugFont; }

	void DirectInput(int *pInput, int Size);
	void SendInput();

	// TODO: OPT: do this alot smarter!
	virtual const int *GetInput(int Tick) const;

	const char *LatestVersion() const;
	void VersionUpdate();

	// ------ state handling -----
	void SetState(int s);

	// called when the map is loaded and we should init for a new round
	void OnEnterGame();
	virtual void EnterGame();

	virtual void Connect(const char *pAddress);
	void DisconnectWithReason(const char *pReason);
	virtual void Disconnect();


	virtual void GetServerInfo(CServerInfo *pServerInfo) const;

	int LoadData();

	// ---

	const void *SnapGetItem(int SnapID, int Index, CSnapItem *pItem) const;
	void SnapInvalidateItem(int SnapID, int Index);
	const void *SnapFindItem(int SnapID, int Type, int ID) const;
	int SnapNumItems(int SnapID) const;
	void *SnapNewItem(int Type, int ID, int Size);
	void SnapSetStaticsize(int ItemType, int Size);

	void Render();
	void DebugRender();

	virtual void Quit();

	virtual const char *ErrorString() const;

	const char *LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc);
	const char *LoadMapSearch(const char *pMapName, int WantedCrc);

	static int PlayerScoreComp(const void *a, const void *b);

	int UnpackServerInfo(CUnpacker *pUnpacker, CServerInfo *pInfo, int *pToken);
	void ProcessConnlessPacket(CNetChunk *pPacket);
	void ProcessServerPacket(CNetChunk *pPacket);

	virtual const char *MapDownloadName() const { return m_aMapdownloadName; }
	virtual int MapDownloadAmount() const { return m_MapdownloadAmount; }
	virtual int MapDownloadTotalsize() const { return m_MapdownloadTotalsize; }
	
	//ModAPI
	virtual const char *ModDownloadName() const { return m_aModdownloadName; }
	virtual int ModDownloadAmount() const { return m_ModdownloadAmount; }
	virtual int ModDownloadTotalsize() const { return m_ModdownloadTotalsize; }

	void PumpNetwork();

	virtual void OnDemoPlayerSnapshot(void *pData, int Size);
	virtual void OnDemoPlayerMessage(void *pData, int Size);

	void Update();

	void RegisterInterfaces();
	void InitInterfaces();

	void Run();


	static void Con_Connect(IConsole::IResult *pResult, void *pUserData);
	static void Con_Disconnect(IConsole::IResult *pResult, void *pUserData);
	static void Con_Quit(IConsole::IResult *pResult, void *pUserData);
	static void Con_Minimize(IConsole::IResult *pResult, void *pUserData);
	static void Con_Ping(IConsole::IResult *pResult, void *pUserData);
	static void Con_Screenshot(IConsole::IResult *pResult, void *pUserData);
	static void Con_Rcon(IConsole::IResult *pResult, void *pUserData);
	static void Con_RconAuth(IConsole::IResult *pResult, void *pUserData);
	static void Con_AddFavorite(IConsole::IResult *pResult, void *pUserData);
	static void Con_RemoveFavorite(IConsole::IResult *pResult, void *pUserData);
	static void Con_Play(IConsole::IResult *pResult, void *pUserData);
	static void Con_Record(IConsole::IResult *pResult, void *pUserData);
	static void Con_StopRecord(IConsole::IResult *pResult, void *pUserData);
	static void Con_AddDemoMarker(IConsole::IResult *pResult, void *pUserData);
	static void ConchainServerBrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainFullscreen(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainWindowBordered(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainWindowScreen(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainWindowVSync(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void RegisterCommands();

	const char *DemoPlayer_Play(const char *pFilename, int StorageType);
	void DemoRecorder_Start(const char *pFilename, bool WithTimestamp);
	void DemoRecorder_HandleAutoStart();
	void DemoRecorder_Stop();
	void DemoRecorder_AddDemoMarker();
	void RecordGameMessage(bool State) { m_RecordGameMessage = State; }

	void AutoScreenshot_Start();
	void AutoScreenshot_Cleanup();

	void ServerBrowserUpdate();

	// gfx
	void SwitchWindowScreen(int Index);
	void ToggleFullscreen();
	void ToggleWindowBordered();
	void ToggleWindowVSync();
	
	//ModAPI
	CModAPI_Client_Graphics* m_pModAPIGraphics;
	char m_aCurrentMod[256];
	unsigned m_CurrentModCrc;

	char m_aModdownloadFilename[256];
	char m_aModdownloadName[256];
	IOHANDLE m_ModdownloadFile;
	int m_ModdownloadChunk;
	int m_ModdownloadChunkNum;
	int m_ModDownloadChunkSize;
	int m_ModdownloadCrc;
	int m_ModdownloadAmount;
	int m_ModdownloadTotalsize;
	
	const char *LoadMod(const char *pName, const char *pFilename, unsigned WantedCrc);
	const char *LoadModSearch(const char *pModName, int WantedCrc);
	
	virtual CModAPI_Client_Graphics *ModAPIGraphics() const { return m_pModAPIGraphics; }
	
	bool m_IsDownloadingMod;
	bool m_MapDownloadNeeded;
	bool m_IsDownloadingMap;
};
#endif
