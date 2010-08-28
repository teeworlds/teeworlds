#include "teams.h"


CTeams::CTeams(CGameContext* gameContext) : m_GameServer(gameContext) {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_Team[i] = 0;
		m_TeamState[i] = EMPTY;
		m_TeeFinished[i] = false;
	}
}

void CTeams::OnCharacterStart(int id) {
	if(m_TeamState[m_Team[id]] <= CLOSED) {
		ChangeTeamState(m_Team[id], STARTED);
		int Tick = Server()->Tick();
		for(int i = 0; i < MAX_CLIENTS; ++i) {
			if(m_Team[i] == m_Team[id]) {
				CCharacter* Char = getCharacter(i);

				Char->m_RaceState = RACE_STARTED;
				Char->m_StartTime = Tick;
				Char->m_RefreshTime = Tick;
			}
		}
	}
}

void CTeams::OnCharacterFinish(int id) {
	if(m_Team[id] == 0) {
		//TODO: as simple DDRace results
	} else {
		m_TeeFinished[id] = true;
		if(TeamFinished(m_Team[id])) {
			//TODO: as simple DDRace results
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
	if(getCharacter(id)->m_RaceState != RACE_NONE) {
		return false;
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

