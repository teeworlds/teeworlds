#ifndef GAME_SERVER_TEAMS_H
#define GAME_SERVER_TEAMS_H


#include <game/server/entities/character.h>
#include <engine/server/server.h>
#include <game/server/gamecontext.h>


class CTeams {
	int m_Team[MAX_CLIENTS];
	int m_TeamState[MAX_CLIENTS];
	bool m_TeeFinished[MAX_CLIENTS];
	CGameContext* m_GameServer;
	
public:
	enum {
		EMPTY, 
		OPEN,
		CLOSED,
		STARTED,
		FINISHED
	};
	
	CTeams(CGameContext* gameContext);
	
	//helper methods
	CCharacter* getCharacter(int id) { return GameServer()->GetPlayerChar(id); }	
	CGameContext* GameServer() { return m_GameServer; }
	IServer* Server() { return m_GameServer->Server(); }
	
	void OnCharacterStart(int id);
	void OnCharacterFinish(int id);
	
	bool SetCharacterTeam(int id, int Team);
	
	void ChangeTeamState(int Team, int State);
	
	bool TeamFinished(int Team);
	
};

#endif