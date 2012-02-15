/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.h>

#include <game/generated/protocol.h>

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/
class IGameController
{
	class CGameContext *m_pGameServer;
	class IServer *m_pServer;

	// activity
	void DoActivityCheck();
	bool GetPlayersReadyState();
	void SetPlayersReadyState(bool ReadyState);

	// balancing
	enum
	{
		TBALANCE_CHECK=-2,
		TBALANCE_OK,
	};
	int m_UnbalancedTick;

	virtual bool CanBeMovedOnBalance(int ClientID) const;
	void CheckTeamBalance();
	void DoTeamBalance();

	// game
	int m_GameState;
	int m_GameStateTimer;
	bool m_StartCountdownReset;

	virtual void DoWincheck();
	void SetGameState(int GameState, int Seconds=0);

	// map
	char m_aMapWish[128];
	
	void CycleMap();

	// spawn
	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100,100);
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];
	
	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos) const;
	void EvaluateSpawnType(CSpawnEval *pEval, int Type) const;

	// team
	int ClampTeam(int Team) const;

protected:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }

	// game
	int m_GameStartTick;
	int m_MatchCount;
	int m_SuddenDeath;
	int m_aTeamscore[NUM_TEAMS];

	void EndMatch();
	void ResetGame();
	void StartMatch();

	// info
	int m_GameFlags;
	const char *m_pGameType;

public:
	IGameController(class CGameContext *pGameServer);
	virtual ~IGameController() {};

	// event
	/*
		Function: on_CCharacter_death
			Called when a CCharacter in the world dies.

		Arguments:
			victim - The CCharacter that died.
			killer - The player that killed it.
			weapon - What weapon that killed it. Can be -1 for undefined
				weapon when switching team or player suicides.
	*/
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	/*
		Function: on_CCharacter_spawn
			Called when a CCharacter spawns into the game world.

		Arguments:
			chr - The CCharacter that was spawned.
	*/
	virtual void OnCharacterSpawn(class CCharacter *pChr);

	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.

		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.

		Returns:
			bool?
	*/
	virtual bool OnEntity(int Index, vec2 Pos);

	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason);
	void OnPlayerInfoChange(class CPlayer *pPlayer);	

	void OnReset();

	// game
	enum
	{
		GS_WARMUP,
		GS_STARTCOUNTDOWN,
		GS_GAME,
		GS_PAUSED,
		GS_GAMEOVER,

		TIMER_INFINITE = -1,
		TIMER_STARTCOUNTDOWN = 3,
	};

	void DoPause(int Seconds) { SetGameState(GS_PAUSED, Seconds); }
	void DoRestart() { SetGameState(GS_STARTCOUNTDOWN, TIMER_STARTCOUNTDOWN); }
	void DoWarmup(int Seconds) { SetGameState(GS_WARMUP, Seconds); }

	// general
	virtual void Snap(int SnappingClient);
	virtual void Tick();

	// info
	bool IsFriendlyFire(int ClientID1, int ClientID2) const;
	bool IsPlayerReadyMode() const;
	bool IsTeamplay() const { return m_GameFlags&GAMEFLAG_TEAMS; }
	
	int GetGameState() const { return m_GameState; }
	const char *GetGameType() const { return m_pGameType; }
	const char *GetTeamName(int Team) const;
	
	// map
	void ChangeMap(const char *pToMap);

	//spawn
	bool CanSpawn(int Team, vec2 *pPos) const;

	// team
	bool CanJoinTeam(int Team, int NotThisID) const;
	bool CanChangeTeam(CPlayer *pPplayer, int JoinTeam) const;

	void DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg=true);
	
	int GetStartTeam(int NotThisID);
};

#endif
