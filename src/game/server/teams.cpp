#include "teams.h"


CTeams::CTeams(CGameContext *pGameContext) : m_pGameContext(pGameContext) {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_Team[i] = 0;
		m_TeamState[i] = EMPTY;
		m_TeeFinished[i] = false;
	}
}

void CTeams::OnCharacterStart(int id) {
	int Tick = Server()->Tick();
	if(m_Team[id] == 0) {
		CCharacter* Char = Character(id);
		Char->m_RaceState = RACE_STARTED;
		Char->m_StartTime = Tick;
		Char->m_RefreshTime = Tick;
	} else {
		if(m_TeamState[m_Team[id]] <= CLOSED) {
			ChangeTeamState(m_Team[id], STARTED);
			
			for(int i = 0; i < MAX_CLIENTS; ++i) {
				if(m_Team[i] == m_Team[id]) {
					CCharacter* Char = Character(i);

					Char->m_RaceState = RACE_STARTED;
					Char->m_StartTime = Tick;
					Char->m_RefreshTime = Tick;
				}
			}
		}
	}
}

void CTeams::OnCharacterFinish(int id) {
	if(m_Team[id] == 0) {
		Character(id)->OnFinish();
	} else {
		m_TeeFinished[id] = true;
		if(TeamFinished(m_Team[id])) {
			ChangeTeamState(m_Team[id], FINISHED);//TODO: Make it better
			for(int i = 0; i < MAX_CLIENTS; ++i) {
				if(SameTeam(i, id)) {
					CCharacter * Char = Character(i);
					if(Char != 0) {
						Char->OnFinish();
						m_TeeFinished[i] = false;
					} //else {
					//	m_Team[id] = 0; //i saw zomby =)
					//}
				}
			}
			
		}
	}
}

bool CTeams::SetCharacterTeam(int id, int Team) {
	//TODO: Send error message 
	if(id < 0 || id >= MAX_CLIENTS || Team < 0 || Team >= MAX_CLIENTS) {
		return false;
	}
	if(m_TeamState[Team] >= CLOSED) {
		return false;
	}
	if(m_Team[id] != 0 && m_TeamState[m_Team[id]] != EMPTY) {
		bool NoOneInOldTeam = true;
		for(int i = 0; i < MAX_CLIENTS; ++i) {
			if(SameTeam(i, id)) {
				NoOneInOldTeam = false;//all good exists someone in old team
				break;
			} 
		}
		if(NoOneInOldTeam) {
			m_TeamState[m_Team[id]] = EMPTY;
		}
	}
	m_Team[id] = Team;
	if(m_TeamState[Team] == EMPTY) {
		ChangeTeamState(Team, OPEN);
	}
	return true;
}

void CTeams::ChangeTeamState(int Team, int State) {
	m_TeamState[Team] = State;
}



bool CTeams::TeamFinished(int Team) {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(m_Team[i] == Team && !m_TeeFinished[i]) {
			return false;
		}
	}
	return true;
}

bool CTeams::SameTeam(int Cid1, int Cid2) {
	return m_Team[Cid1] = m_Team[Cid2];
}
