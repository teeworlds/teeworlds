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

	static void ConMute(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConSetlvl(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConKillPlayer(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConNinjaMe(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConNinja(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConHammerMe(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConHammer(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConUnSuperMe(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConUnSuper(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConSuper(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConSuperMe(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConWeaponsMe(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConWeapons(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConTeleport(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData, int cid);

	static void ConPhook(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConTimerStop(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConTimerStart(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConTimerReStart(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConTimerZero(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConSay(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void  ConGoLeft(IConsole::IResult *pResult, void *pUserData, int cid);
	static void  ConGoRight(IConsole::IResult *pResult, void *pUserData, int cid);
	static void  ConGoUp(IConsole::IResult *pResult, void *pUserData, int cid);
	static void  ConGoDown(IConsole::IResult *pResult, void *pUserData, int cid);
	
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConVote(IConsole::IResult *pResult, void *pUserData, int cid);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	
	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;
public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }

	CGameContext();
	~CGameContext();
	
	void Clear();
	
	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];
	
	//bool m_Cheats;
	//bool m_EndlessDrag;
	//bool m_Tunes;
	//bool m_PlayersCollision;

	IGameController *m_pController;
	CGameWorld m_World;
	
	// helper functions
	class CCharacter *GetPlayerChar(int ClientId);
	
	// voting
	void StartVote(const char *pDesc, const char *pCommand);
	void EndVote();
	void SendVoteSet(int ClientId);
	void SendVoteStatus(int ClientId, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientId);
	
	bool CheatsAvailable(int cid);
	
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
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage);
	void CreateSmoke(vec2 Pos);
	void CreateHammerHit(vec2 Pos);
	void CreatePlayerSpawn(vec2 Pos);
	void CreateDeath(vec2 Pos, int Who);
	void CreateSound(vec2 Pos, int Sound, int Mask=-1);
	void CreateSoundGlobal(int Sound, int Target=-1);	


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendChatTarget(int To, const char *pText);
	void SendChat(int ClientId, int Team, const char *pText);
	void SendEmoticon(int ClientId, int Emoticon);
	void SendWeaponPickup(int ClientId, int Weapon);
	void SendBroadcast(const char *pText, int ClientId);
	
	
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
	virtual void OnSetResistent(int ClientId, int Resistent);

	virtual const char *Version();
	virtual const char *NetVersion();
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientId) { return 1<<ClientId; }
inline int CmaskAllExceptOne(int ClientId) { return 0x7fffffff^CmaskOne(ClientId); }
inline bool CmaskIsSet(int Mask, int ClientId) { return (Mask&CmaskOne(ClientId)) != 0; }
#endif
