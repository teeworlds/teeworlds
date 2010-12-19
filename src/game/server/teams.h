#ifndef GAME_SERVER_TEAMS_H
#define GAME_SERVER_TEAMS_H

#include <game/teamscore.h>
#include <game/server/gamecontext.h>

class CGameTeams {
	int m_TeamState[MAX_CLIENTS];
	int m_MembersCount[MAX_CLIENTS];
	bool m_TeeFinished[MAX_CLIENTS];
	
	class CGameContext * m_pGameContext;
	
	
public:
	enum {
		EMPTY, 
		OPEN,
		CLOSED,
		STARTED,
		FINISHED
	};
	
	CTeamsCore m_Core;
	
	CGameTeams(CGameContext *pGameContext);
	
	//helper methods
	CCharacter* Character(int id) { return GameServer()->GetPlayerChar(id); }	

	class CGameContext *GameServer() { return m_pGameContext; }
	class IServer *Server() { return m_pGameContext->Server(); }
	
	void OnCharacterStart(int id);
	void OnCharacterFinish(int id);
	
	bool SetCharacterTeam(int id, int Team);
	
	void ChangeTeamState(int Team, int State);
	
	bool TeamFinished(int Team);

	int TeamMask(int Team);
	
	int Count(int Team) const;
	
	//need to be very carefull using this method
	void SetForceCharacterTeam(int id, int Team);
	
	void Reset();
	
	void SendTeamsState(int Cid);

	int m_LastChat[MAX_CLIENTS];
};

#endif
