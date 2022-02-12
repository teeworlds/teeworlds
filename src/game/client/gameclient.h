/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_GAMECLIENT_H
#define GAME_CLIENT_GAMECLIENT_H

#include <base/vmath.h>
#include <engine/client.h>
#include <engine/console.h>
#include <game/layers.h>
#include <game/gamecore.h>
#include "render.h"
#include "ui.h"

class CGameClient : public IGameClient
{
	class CStack
	{
	public:
		enum
		{
			MAX_COMPONENTS = 64,
		};

		CStack();
		void Add(class CComponent *pComponent);

		class CComponent *m_paComponents[MAX_COMPONENTS];
		int m_Num;
	};

	CStack m_All;
	CStack m_Input;
	CNetObjHandler m_NetObjHandler;

	class IEngine *m_pEngine;
	class IInput *m_pInput;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class IClient *m_pClient;
	class ISound *m_pSound;
	class CConfig *m_pConfig;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	class IDemoPlayer *m_pDemoPlayer;
	class IDemoRecorder *m_pDemoRecorder;
	class IServerBrowser *m_pServerBrowser;
	class IEditor *m_pEditor;
	class IFriends *m_pFriends;
	class IBlacklist *m_pBlacklist;

	CLayers m_Layers;
	class CCollision m_Collision;
	CUI m_UI;

	void ProcessEvents();
	void ProcessTriggeredEvents(int Events, vec2 Pos);
	void UpdatePositions();

	int m_PredictedTick;
	int m_LastNewPredictedTick;

	int m_LastGameStartTick;
	int m_LastFlagCarrierRed;
	int m_LastFlagCarrierBlue;

	static void ConTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConKill(IConsole::IResult *pResult, void *pUserData);
	static void ConReadyChange(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSkinChange(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainFriendUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainBlacklistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainXmasHatUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void EvolveCharacter(CNetObj_Character *pCharacter, int Tick);

	void LoadFonts();

public:
	IKernel *Kernel() { return IInterface::Kernel(); }
	IEngine *Engine() const { return m_pEngine; }
	class IGraphics *Graphics() const { return m_pGraphics; }
	class IClient *Client() const { return m_pClient; }
	class CUI *UI() { return &m_UI; }
	class ISound *Sound() const { return m_pSound; }
	class IInput *Input() const { return m_pInput; }
	class IStorage *Storage() const { return m_pStorage; }
	class CConfig *Config() const { return m_pConfig; }
	class IConsole *Console() { return m_pConsole; }
	class ITextRender *TextRender() const { return m_pTextRender; }
	class IDemoPlayer *DemoPlayer() const { return m_pDemoPlayer; }
	class IDemoRecorder *DemoRecorder() const { return m_pDemoRecorder; }
	class IServerBrowser *ServerBrowser() const { return m_pServerBrowser; }
	class CRenderTools *RenderTools() { return &m_RenderTools; }
	class CLayers *Layers() { return &m_Layers; }
	class CCollision *Collision() { return &m_Collision; }
	class IEditor *Editor() { return m_pEditor; }
	class IFriends *Friends() { return m_pFriends; }
	class IBlacklist *Blacklist() { return m_pBlacklist; }

	const char *NetobjFailedOn() { return m_NetObjHandler.FailedObjOn(); }
	int NetobjNumFailures() { return m_NetObjHandler.NumObjFailures(); }
	const char *NetmsgFailedOn() { return m_NetObjHandler.FailedMsgOn(); }

	bool m_SuppressEvents;

	// TODO: move this
	CTuningParams m_Tuning;

	enum
	{
		SERVERMODE_PURE=0,
		SERVERMODE_MOD,
		SERVERMODE_PUREMOD,
	};
	int m_ServerMode;

	int m_DemoSpecMode;
	int m_DemoSpecID;

	vec2 m_LocalCharacterPos;

	// Whether we should use/render predicted entities. Depends on client
	// and game state.
	bool ShouldUsePredicted() const;

	// Whether we should use/render predictions for a specific `ClientID`.
	// Should check `ShouldUsePredicted` before checking this.
	bool ShouldUsePredictedChar(int ClientID) const;

	// Replaces `pPrevChar`, `pPlayerChar`, and `IntraTick` with their predicted
	// counterparts for `ClientID`. Should check `ShouldUsePredictedChar`
	// before using this.
	void UsePredictedChar(
		CNetObj_Character *pPrevChar,
		CNetObj_Character *pPlayerChar,
		float *IntraTick,
		int ClientID
	) const;

	vec2 GetCharPos(int ClientID, bool Predicted = false) const;

	// ---

	struct CPlayerInfoItem
	{
		const CNetObj_PlayerInfo *m_pPlayerInfo;
		int m_ClientID;
	};

	// snap pointers
	struct CSnapState
	{
		const CNetObj_Character *m_pLocalCharacter;
		const CNetObj_Character *m_pLocalPrevCharacter;
		const CNetObj_PlayerInfo *m_pLocalInfo;
		const CNetObj_SpectatorInfo *m_pSpectatorInfo;
		const CNetObj_SpectatorInfo *m_pPrevSpectatorInfo;
		const CNetObj_Flag *m_paFlags[2];
		const CNetObj_GameData *m_pGameData;
		const CNetObj_GameDataTeam *m_pGameDataTeam;
		const CNetObj_GameDataFlag *m_pGameDataFlag;
		const CNetObj_GameDataRace *m_pGameDataRace;
		int m_GameDataFlagSnapID;

		int m_NotReadyCount;
		int m_AliveCount[NUM_TEAMS];

		const CNetObj_PlayerInfo *m_paPlayerInfos[MAX_CLIENTS];
		const CNetObj_PlayerInfoRace *m_paPlayerInfosRace[MAX_CLIENTS];
		CPlayerInfoItem m_aInfoByScore[MAX_CLIENTS];

		// spectate data
		struct CSpectateInfo
		{
			bool m_Active;
			int m_SpecMode;
			int m_SpectatorID;
			bool m_UsePosition;
			vec2 m_Position;
		} m_SpecInfo;

		//
		struct CCharacterInfo
		{
			bool m_Active;

			// snapshots
			CNetObj_Character m_Prev;
			CNetObj_Character m_Cur;

			// interpolated position
			vec2 m_Position;
		};

		CCharacterInfo m_aCharacters[MAX_CLIENTS];
	};

	CSnapState m_Snap;

	// client data
	struct CClientData
	{
		char m_aName[MAX_NAME_ARRAY_SIZE];
		char m_aClan[MAX_CLAN_ARRAY_SIZE];
		int m_Country;
		char m_aaSkinPartNames[NUM_SKINPARTS][MAX_SKIN_ARRAY_SIZE];
		int m_aUseCustomColors[NUM_SKINPARTS];
		int m_aSkinPartColors[NUM_SKINPARTS];
		int m_SkinPartIDs[NUM_SKINPARTS];
		int m_Team;
		int m_Emoticon;
		int m_EmoticonStart;
		CCharacterCore m_Predicted;
		CCharacterCore m_PrevPredicted;

		CTeeRenderInfo m_SkinInfo; // this is what the server reports
		CTeeRenderInfo m_RenderInfo; // this is what we use

		CNetObj_Character m_Evolved;

		float m_Angle;
		bool m_Active;
		bool m_ChatIgnore;
		bool m_Friend;

		void UpdateRenderInfo(CGameClient *pGameClient, int ClientID, bool UpdateSkinInfo);
		void UpdateBotRenderInfo(CGameClient *pGameClient, int ClientID);
		void Reset(CGameClient *pGameClient, int CLientID);
	};

	CClientData m_aClients[MAX_CLIENTS];
	int m_LocalClientID;
	int m_TeamCooldownTick;
	float m_TeamChangeTime;
	bool m_IsXmasDay;
	float m_LastSkinChangeTime;
	int m_IdentityState;
	bool m_IsEasterDay;
	bool m_InitComplete;

	struct CGameInfo
	{
		int m_GameFlags;
		int m_ScoreLimit;
		int m_TimeLimit;
		int m_MatchNum;
		int m_MatchCurrent;

		int m_NumPlayers;
		int m_aTeamSize[NUM_TEAMS];
	};

	CGameInfo m_GameInfo;

	struct CServerSettings
	{
		bool m_KickVote;
		int m_KickMin;
		bool m_SpecVote;
		bool m_TeamLock;
		bool m_TeamBalance;
		int m_PlayerSlots;
	} m_ServerSettings;

	CRenderTools m_RenderTools;

	void OnReset();

	// hooks
	virtual void OnConnected();
	virtual void OnRender();
	virtual void OnUpdate();
	virtual void OnRelease();
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnMessage(int MsgId, CUnpacker *pUnpacker);
	virtual void OnNewSnapshot();
	virtual void OnDemoRecSnap();
	virtual void OnPredict();
	virtual void OnActivateEditor();
	virtual int OnSnapInput(int *pData);
	virtual void OnShutdown();
	virtual void OnEnterGame();
	virtual void OnRconLine(const char *pLine);
	virtual void OnGameOver();
	virtual void OnStartGame();

	virtual const char *GetItemName(int Type) const;
	virtual const char *Version() const;
	virtual const char *NetVersion() const;
	virtual const char *NetVersionHashUsed() const;
	virtual const char *NetVersionHashReal() const;
	virtual int ClientVersion() const;
	void GetPlayerLabel(char* aBuf, int BufferSize, int ClientID, const char* ClientName);
	void StartRendering();

	bool IsXmas() const;
	bool IsEaster() const;
	int RacePrecision() const { return m_Snap.m_pGameDataRace ? m_Snap.m_pGameDataRace->m_Precision : 3; }
	bool IsWorldPaused() const { return m_Snap.m_pGameData && (m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER)); }
	bool IsDemoPlaybackPaused() const;
	float GetAnimationPlaybackSpeed() const;

	//
	void DoEnterMessage(const char *pName, int ClientID, int Team);
	void DoLeaveMessage(const char *pName, int ClientID, const char *pReason);
	void DoTeamChangeMessage(const char *pName, int ClientID, int Team);

	int GetClientID(const char *pName);

	// actions
	// TODO: move these
	void SendSwitchTeam(int Team);
	void SendStartInfo();
	void SendKill();
	void SendReadyChange();
	void SendSkinChange();

	// pointers to all systems
	class CGameConsole *m_pGameConsole;
	class CBinds *m_pBinds;
	class CBroadcast *m_pBroadcast;
	class CParticles *m_pParticles;
	class CMenus *m_pMenus;
	class CSkins *m_pSkins;
	class CCountryFlags *m_pCountryFlags;
	class CFlow *m_pFlow;
	class CChat *m_pChat;
	class CDamageInd *m_pDamageind;
	class CCamera *m_pCamera;
	class CControls *m_pControls;
	class CEffects *m_pEffects;
	class CSounds *m_pSounds;
	class CMotd *m_pMotd;
	class CMapImages *m_pMapimages;
	class CVoting *m_pVoting;
	class CScoreboard *m_pScoreboard;
	class CStats *m_pStats;
	class CItems *m_pItems;
	class CMapLayers *m_pMapLayersBackGround;
	class CMapLayers *m_pMapLayersForeGround;
};


void FormatTime(char *pBuf, int Size, int Time, int Precision);
void FormatTimeDiff(char *pBuf, int Size, int Time, int Precision, bool ForceSign = true);

const char *Localize(const char *pStr, const char *pContext="")
GNUC_ATTRIBUTE((format_arg(1)));

#endif
