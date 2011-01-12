#ifndef GAME_TEAMSCORE_H
#define GAME_TEAMSCORE_H

#include <engine/shared/protocol.h>

enum {
	TEAM_FLOCK = 0,
	TEAM_SUPER = 16
};

class CTeamsCore {
	
	int m_Team[MAX_CLIENTS];
public:
	
	CTeamsCore(void);
	
	bool SameTeam(int Cid1, int Cid2);

	bool CanCollide(int Cid1, int Cid2);
	
	int Team(int Cid);
	void Team(int Cid, int Team);

	void Reset();
};

#endif
