#include "teamscore.h"

CTeamsCore::CTeamsCore() {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_Team[i] = 0;
	}
}

bool CTeamsCore::SameTeam(int Cid1, int Cid2) {
	return m_Team[Cid1] = m_Team[Cid2];
}
	
int CTeamsCore::Team(int Cid) {
	return m_Team[Cid];
}

void CTeamsCore::Team(int Cid, int Team) {
	m_Team[Cid] = Team;
}