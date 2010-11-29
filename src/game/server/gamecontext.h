/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <game/layers.h>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"
#include "score.h"

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)
			

	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/
class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;
	class IScore *m_pScore;

	static void ConMute(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConLogOut(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConSetlvl1(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConSetlvl2(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConSetlvl3(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConKillPlayer(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConNinja(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConHammer(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConUnSuper(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConSuper(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConShotgun(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConGrenade(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConRifle(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConWeapons(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConUnShotgun(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConUnGrenade(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConUnRifle(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConUnWeapons(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConAddWeapon(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConRemoveWeapon(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	void ModifyWeapons(int ClientId, int Victim, int Weapon, bool Remove);
	void MoveCharacter(int ClientId, int Victim, int X, int Y, bool Raw = false);
	
	static void ConTeleport(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData, int ClientId);

	static void ConPhook(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConFreeze(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConUnFreeze(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConTimerStop(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConTimerStart(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConTimerReStart(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConTimerZero(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConSay(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConGoLeft(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConGoRight(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConGoUp(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConGoDown(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConMove(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConMoveRaw(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConVote(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	
	static void ConInvisMe(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConVisMe(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConInvis(IConsole::IResult *pResult, void *pUserData, int ClientId);
	static void ConVis(IConsole::IResult *pResult, void *pUserData, int ClientId);
	
  static void ConCredits(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConInfo(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConHelp(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConFlags(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConRules(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConKill(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConTogglePause(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConTop5(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConRank(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConBroadTime(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConJoinTeam(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConToggleFly(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConMe(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConToggleEyeEmote(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConToggleBroadcast(IConsole::IResult *pResult, void *pUserData, int ClientId);
  static void ConEyeEmote(IConsole::IResult *pResult, void *pUserData, int ClientId);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	int m_VoteEnforcer;
	bool m_Resetting;
	int m_AnnouncementLine;
public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CLayers *Layers() { return &m_Layers; }
	CTuningParams *Tuning() { return &m_Tuning; }
	
	class IScore *Score() { return m_pScore; }

	CGameContext();
	~CGameContext();
	
	void Clear();
	
	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
	CGameWorld m_World;
	//CTeleTile *m_pSwitch;
	// helper functions
	class CCharacter *GetPlayerChar(int ClientId);
	
	// voting
	void StartVote(const char *pDesc, const char *pCommand);
	void EndVote();
	void SendVoteSet(int ClientId);
	void SendVoteStatus(int ClientId, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientId);
	
	bool m_VoteKick;
	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[512];
	char m_aVoteCommand[512];
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
		VOTE_ENFORCE_NO_ADMIN,
		VOTE_ENFORCE_YES_ADMIN
	};
	struct CVoteOption
	{
		CVoteOption *m_pNext;
		CVoteOption *m_pPrev;
		char m_aCommand[1];
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOption *m_pVoteOptionFirst;
	CVoteOption *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, int Mask=-1);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, int ActivatedTeam, int Mask);
	void CreateSmoke(vec2 Pos, int Mask=-1);
	void CreateHammerHit(vec2 Pos, int Mask=-1);
	void CreatePlayerSpawn(vec2 Pos, int Mask=-1);
	void CreateDeath(vec2 Pos, int Who, int Mask=-1);
	void CreateSound(vec2 Pos, int Sound, int Mask=-1);
	void CreateSoundGlobal(int Sound, int Target=-1);	

	struct ReconnectInfo
	{
		struct PlayerInfo {
			CCharacterCore m_Core;
			int m_StartTime;
			int m_DDRaceState;
			//int m_RefreshTime;
			int m_FreezeTime;
			bool m_Doored;
			vec2 m_OldPos;
			vec2 m_OlderPos;
			int m_LastAction;
			int m_Jumped;
			int m_Health;
			int m_Armor;
			int m_PlayerState;
			int m_LastMove;
			vec2 m_PrevPos;
			int m_ActiveWeapon;
			int m_LastWeapon;
			bool m_aHasWeapon[NUM_WEAPONS];
			int m_HammerType;
			bool m_Super;
			int m_PauseTime;
		} m_PlayerInfo;
		char Ip[64];
		int m_ClientId;
		int m_DisconnectTick;
	}m_pReconnectInfo[MAX_CLIENTS];
	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendChatTarget(int To, const char *pText);
	void SendChat(int ClientId, int Team, const char *pText, int SpamProtectionClientId = -1);
	void SendEmoticon(int ClientId, int Emoticon);
	void SendWeaponPickup(int ClientId, int Weapon);
	void SendBroadcast(const char *pText, int ClientId);
	void SendRecord(int ClientId);
	
	static void SendChatResponse(const char *pLine, void *pUser);
	static void SendChatResponseAll(const char *pLine, void *pUser);
	
	struct ChatResponseInfo
	{
	  CGameContext *m_GameContext;
	  int m_To;
	};

	//
	void CheckPureTuning();
	void SendTuningParams(int ClientId);
	
	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();
	
	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientId);
	virtual void OnPostSnap();
	
	virtual void OnMessage(int MsgId, CUnpacker *pUnpacker, int ClientId);

	virtual void OnClientConnected(int ClientId);
	virtual void OnClientEnter(int ClientId);
	virtual void OnClientDrop(int ClientId);
	virtual void OnClientDirectInput(int ClientId, void *pInput);
	virtual void OnClientPredictedInput(int ClientId, void *pInput);
	
	virtual void OnSetAuthed(int ClientId,int Level);

	virtual const char *Version();
	virtual const char *NetVersion();
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientId) { return 1<<ClientId; }
inline int CmaskAllExceptOne(int ClientId) { return 0x7fffffff^CmaskOne(ClientId); }
inline bool CmaskIsSet(int Mask, int ClientId) { return (Mask&CmaskOne(ClientId)) != 0; }
#endif
