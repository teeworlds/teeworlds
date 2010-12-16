/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_GAMECLIENT_H
#define GAME_CLIENT_GAMECLIENT_H

#include <base/vmath.h>
#include <engine/client.h>
#include <engine/console.h>
#include <game/layers.h>
#include <game/gamecore.h>
#include <game/teamscore.h>
#include "render.h"

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
	
	class IInput *m_pInput;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class IClient *m_pClient;
	class ISound *m_pSound;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	class IDemoPlayer *m_pDemoPlayer;
	class IDemoRecorder *m_pDemoRecorder;
	class IServerBrowser *m_pServerBrowser;
	
	CLayers m_Layers;
	class CCollision m_Collision;
	class CTeamsCore m_Teams;
	CUI m_UI;
	
	void DispatchInput();
	void ProcessEvents();
	void UpdateLocalCharacterPos();

	int m_PredictedTick;
	int m_LastNewPredictedTick;

	int64 m_LastSendInfo;
	
	bool m_DDRaceMsgSent;

	static void ConTeam(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void ConKill(IConsole::IResult *pResult, void *pUserData, int ClientID);
	
	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	
public:
	IKernel *Kernel() { return IInterface::Kernel(); }
	class IGraphics *Graphics() const { return m_pGraphics; }
	class IClient *Client() const { return m_pClient; }
	class CUI *UI() { return &m_UI; }
	class ISound *Sound() const { return m_pSound; }
	class IInput *Input() const { return m_pInput; }
	class IStorage *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	class ITextRender *TextRender() const { return m_pTextRender; }
	class IDemoPlayer *DemoPlayer() const { return m_pDemoPlayer; }
	class IDemoRecorder *DemoRecorder() const { return m_pDemoRecorder; }
	class IServerBrowser *ServerBrowser() const { return m_pServerBrowser; }
	class CRenderTools *RenderTools() { return &m_RenderTools; }
	class CLayers *Layers() { return &m_Layers; };
	class CCollision *Collision() { return &m_Collision; };
	
	int NetobjNumCorrections() { return m_NetObjHandler.NumObjCorrections(); }
	const char *NetobjCorrectedOn() { return m_NetObjHandler.CorrectedObjOn(); }

	bool m_SuppressEvents;
	bool m_NewTick;
	bool m_NewPredictedTick;

	// TODO: move this
	CTuningParams m_Tuning;
	
	enum
	{
		SERVERMODE_PURE=0,
		SERVERMODE_MOD,
		SERVERMODE_PUREMOD,
	};
	int m_ServerMode;

	vec2 m_LocalCharacterPos;

	// predicted players
	CCharacterCore m_PredictedPrevChar;
	CCharacterCore m_PredictedChar;

	// snap pointers
	struct CSnapState
	{
		const CNetObj_Character *m_pLocalCharacter;
		const CNetObj_Character *m_pLocalPrevCharacter;
		const CNetObj_PlayerInfo *m_pLocalInfo;
		const CNetObj_Flag *m_paFlags[2];
		const CNetObj_Game *m_pGameobj;

		const CNetObj_PlayerInfo *m_paPlayerInfos[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByScore[MAX_CLIENTS];
		
		int m_LocalCid;
		int m_NumPlayers;
		int m_aTeamSize[2];
		bool m_Spectate;
		
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
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
		
		char m_aName[64];
		char m_aSkinName[64];
		int m_SkinId;
		int m_SkinColor;
		int m_Team;
		int m_Emoticon;
		int m_EmoticonStart;
		CCharacterCore m_Predicted;
		
		int m_Score;
		
		CTeeRenderInfo m_SkinInfo; // this is what the server reports
		CTeeRenderInfo m_RenderInfo; // this is what we use
		
		float m_Angle;
		
		void UpdateRenderInfo();
	};

	CClientData m_aClients[MAX_CLIENTS];
	
	CRenderTools m_RenderTools;
	
	void OnReset();

	// hooks
	virtual void OnConnected();
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnMessage(int MsgId, CUnpacker *pUnpacker);
	virtual void OnNewSnapshot();
	virtual void OnPredict();
	virtual int OnSnapInput(int *pData);
	virtual void OnShutdown();
	virtual void OnEnterGame();
	virtual void OnRconLine(const char *pLine);
	virtual void OnGameOver();
	
	virtual const char *GetItemName(int Type);
	virtual const char *Version();
	virtual const char *NetVersion();
	
	
	// actions
	// TODO: move these
	void SendSwitchTeam(int Team);
	void SendInfo(bool Start);
	void SendKill(int ClientId);
	
	// pointers to all systems
	class CGameConsole *m_pGameConsole;
	class CBinds *m_pBinds;
	class CParticles *m_pParticles;
	class CMenus *m_pMenus;
	class CSkins *m_pSkins;
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
};

extern const char *Localize(const char *Str);

#endif
