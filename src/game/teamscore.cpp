#include "teamscore.h"

CTeamsCore::CTeamsCore() {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_Team[i] = TEAM_FLOCK;
	}
}

bool CTeamsCore::SameTeam(int Cid1, int Cid2) {
	if(m_Team[Cid1] == TEAM_SUPER || m_Team[Cid2] == TEAM_SUPER) return true;
	return m_Team[Cid1] == m_Team[Cid2];
}
	
int CTeamsCore::Team(int Cid) {
	return m_Team[Cid];
}

void CTeamsCore::Team(int Cid, int Team) {
	m_Team[Cid] = Team;
}