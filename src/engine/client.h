/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_H
#define ENGINE_CLIENT_H
#include "kernel.h"

#include "message.h"

class IClient : public IInterface
{
	MACRO_INTERFACE("client", 0)
protected:
	// quick access to state of the client
	int m_State;

	// quick access to time variables
	int m_PrevGameTick;
	int m_CurGameTick;
	float m_GameIntraTick;
	float m_GameTickTime;

	int m_PredTick;
	float m_PredIntraTick;

	float m_LocalTime;
	float m_RenderFrameTime;

	int m_GameTickSpeed;
public:

	class CSnapItem
	{
	public:
		int m_Type;
		int m_ID;
		int m_DataSize;
	};

	/* Constants: Client States
		STATE_OFFLINE - The client is offline.
		STATE_CONNECTING - The client is trying to connect to a server.
		STATE_LOADING - The client has connected to a server and is loading resources.
		STATE_ONLINE - The client is connected to a server and running the game.
		STATE_DEMOPLAYBACK - The client is playing a demo
		STATE_QUITING - The client is quiting.
	*/

	enum
	{
		STATE_OFFLINE=0,
		STATE_CONNECTING,
		STATE_LOADING,
		STATE_ONLINE,
		STATE_DEMOPLAYBACK,
		STATE_QUITING,
	};

	//
	inline int State() const { return m_State; }

	// tick time access
	inline int PrevGameTick() const { return m_PrevGameTick; }
	inline int GameTick() const { return m_CurGameTick; }
	inline int PredGameTick() const { return m_PredTick; }
	inline float IntraGameTick() const { return m_GameIntraTick; }
	inline float PredIntraGameTick() const { return m_PredIntraTick; }
	inline float GameTickTime() const { return m_GameTickTime; }
	inline int GameTickSpeed() const { return m_GameTickSpeed; }

	// other time access
	inline float RenderFrameTime() const { return m_RenderFrameTime; }
	inline float LocalTime() const { return m_LocalTime; }

	// actions
	virtual void Connect(const char *pAddress) = 0;
	virtual void Disconnect() = 0;
	virtual void Quit() = 0;
	virtual const char *DemoPlayer_Play(const char *pFilename, int StorageType) = 0;
	virtual void DemoRecorder_Start(const char *pFilename, bool WithTimestamp) = 0;
	virtual void DemoRecorder_HandleAutoStart() = 0;
	virtual void DemoRecorder_Stop() = 0;
	virtual void AutoScreenshot_Start() = 0;
	virtual void ServerBrowserUpdate() = 0;

	// networking
	virtual void EnterGame() = 0;

	//
	virtual int MapDownloadAmount() = 0;
	virtual int MapDownloadTotalsize() = 0;

	// input
	virtual int *GetInput(int Tick) = 0;

	// remote console
	virtual void RconAuth(const char *pUsername, const char *pPassword) = 0;
	virtual bool RconAuthed() = 0;
	virtual bool UseTempRconCommands() = 0;
	virtual void Rcon(const char *pLine) = 0;

	// server info
	virtual void GetServerInfo(class CServerInfo *pServerInfo) = 0;

	// snapshot interface

	enum
	{
		SNAP_CURRENT=0,
		SNAP_PREV=1
	};

	// TODO: Refactor: should redo this a bit i think, too many virtual calls
	virtual int SnapNumItems(int SnapID) = 0;
	virtual void *SnapFindItem(int SnapID, int Type, int ID) = 0;
	virtual void *SnapGetItem(int SnapID, int Index, CSnapItem *pItem) = 0;
	virtual void SnapInvalidateItem(int SnapID, int Index) = 0;

	virtual void SnapSetStaticsize(int ItemType, int Size) = 0;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags) = 0;

	template<class T>
	int SendPackMsg(T *pMsg, int Flags)
	{
		CMsgPacker Packer(pMsg->MsgID());
		if(pMsg->Pack(&Packer))
			return -1;
		return SendMsg(&Packer, Flags);
	}

	//
	virtual const char *ErrorString() = 0;
	virtual const char *LatestVersion() = 0;
	virtual bool ConnectionProblems() = 0;

	virtual bool SoundInitFailed() = 0;

	virtual int GetDebugFont() = 0;
};

class IGameClient : public IInterface
{
	MACRO_INTERFACE("gameclient", 0)
protected:
public:
	virtual void OnConsoleInit() = 0;

	virtual void OnRconLine(const char *pLine) = 0;
	virtual void OnInit() = 0;
	virtual void OnNewSnapshot() = 0;
	virtual void OnEnterGame() = 0;
	virtual void OnShutdown() = 0;
	virtual void OnRender() = 0;
	virtual void OnStateChange(int NewState, int OldState) = 0;
	virtual void OnConnected() = 0;
	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker) = 0;
	virtual void OnPredict() = 0;
	virtual void OnActivateEditor() = 0;

	virtual int OnSnapInput(int *pData) = 0;

	virtual const char *GetItemName(int Type) = 0;
	virtual const char *Version() = 0;
	virtual const char *NetVersion() = 0;

};

extern IGameClient *CreateGameClient();
#endif
