#ifndef GAME_TEAMSCORE_H
#define GAME_TEAMSCORE_H

#include <engine/shared/protocol.h>

class CTeamsCore {
	
	int m_Team[MAX_CLIENTS];
public:
	
	CTeamsCore(void);
	
	bool SameTeam(int Cid1, int Cid2);
	
	int Team(int Cid);
	void Team(int Cid, int Team);
};

#endif