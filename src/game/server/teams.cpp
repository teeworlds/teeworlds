#include "teams.h"


CGameTeams::CGameTeams(CGameContext *pGameContext) : m_pGameContext(pGameContext) {
	Reset();
}

void CGameTeams::Reset() {
	m_Core.Reset();
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_TeamState[i] = EMPTY;
		m_TeeFinished[i] = false;
	}
}

void CGameTeams::OnCharacterStart(int id) {
	int Tick = Server()->Tick();
	if(m_Core.Team(id) == TEAM_FLOCK || m_Core.Team(id) == TEAM_SUPER) {
		CCharacter* Char = Character(id);
		Char->m_RaceState = RACE_STARTED;
		Char->m_StartTime = Tick;
		Char->m_RefreshTime = Tick;
	} else {
		if(m_TeamState[m_Core.Team(id)] <= CLOSED) {
			ChangeTeamState(m_Core.Team(id), STARTED);
			
			for(int i = 0; i < MAX_CLIENTS; ++i) {
				if(m_Core.Team(id) == m_Core.Team(i)) {
					CCharacter* Char = Character(i);

					Char->m_RaceState = RACE_STARTED;
					Char->m_StartTime = Tick;
					Char->m_RefreshTime = Tick;
				}
			}
		}
	}
}

void CGameTeams::OnCharacterFinish(int id) {
	if(m_Core.Team(id) == TEAM_FLOCK || m_Core.Team(id) == TEAM_SUPER) {
		Character(id)->OnFinish();
	} else {
		m_TeeFinished[id] = true;
		if(TeamFinished(m_Core.Team(id))) {
			ChangeTeamState(m_Core.Team(id), FINISHED);//TODO: Make it better
			for(int i = 0; i < MAX_CLIENTS; ++i) {
				if(m_Core.Team(id) == m_Core.Team(i)) {
					CCharacter * Char = Character(i);
					if(Char != 0) {
						Char->OnFinish();
						m_TeeFinished[i] = false;
					} //else {
					//	m_Core.Team(id) = 0; //i saw zomby =)
					//}
				}
			}
			
		}
	}
}

bool CGameTeams::SetCharacterTeam(int id, int Team) {
	//Check on wrong parameters. +1 for TEAM_SUPER
	if(id < 0 || id >= MAX_CLIENTS || Team < 0 || Team >= MAX_CLIENTS + 1) {
		return false;
	}
	//You can join to TEAM_SUPER at any time, but any other group you cannot if it started
	if(Team != TEAM_SUPER && m_TeamState[Team] >= CLOSED) {
		return false;
	}
	//No need to switch team if you there
	if(m_Core.Team(id) == Team) {
		return false;
	}
	//You cannot be in TEAM_SUPER if you not super
	if(Team == TEAM_SUPER && !Character(id)->m_Super) return false;
	//if you begin race
	if(Character(id)->m_RaceState != RACE_NONE) {
		//you will be killed if you try to join FLOCK
		if(Team == TEAM_FLOCK && m_Core.Team(id) != TEAM_FLOCK) {
			Character(id)->GetPlayer()->KillCharacter(WEAPON_GAME);
		} else {
			if(Team != TEAM_SUPER) {
				return false;
			}
		}
	}
	SetForceCharacterTeam(id, Team);
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "Id = %d Team = %d", id, Team);
	dbg_msg("Teams", aBuf);
	//GameServer()->CreatePlayerSpawn(Character(id)->m_Core.m_Pos, TeamMask());
	if(Character(id)->GetPlayer()->m_IsUsingRaceClient)
	{
		CNetMsg_Sv_PlayerTeam Msg;
		Msg.m_Team = Team;
		Msg.m_Cid = id;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
		//dbg_msg("Teams", "Sended all");
	}
	return true;
}

void CGameTeams::SetForceCharacterTeam(int id, int Team) {
	m_TeeFinished[id] = false;
	if(m_Core.Team(id) != TEAM_FLOCK 
		&& m_Core.Team(id) != TEAM_SUPER 
		&& m_TeamState[m_Core.Team(id)] != EMPTY) {
		bool NoOneInOldTeam = true;
		for(int i = 0; i < MAX_CLIENTS; ++i) {
			if(i != id && m_Core.Team(id) == m_Core.Team(i)) {
				NoOneInOldTeam = false;//all good exists someone in old team
				break;
			} 
		}
		if(NoOneInOldTeam) {
			m_TeamState[m_Core.Team(id)] = EMPTY;
		}
	}
	m_Core.Team(id, Team);
	if(Team != TEAM_SUPER && m_TeamState[Team] == EMPTY) {
		ChangeTeamState(Team, OPEN);
	}
}

void CGameTeams::ChangeTeamState(int Team, int State) {
	m_TeamState[Team] = State;
}



bool CGameTeams::TeamFinished(int Team) {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(m_Core.Team(i) == Team && !m_TeeFinished[i]) {
			return false;
		}
	}
	return true;
}

int CGameTeams::TeamMask(int Team) {
	if(Team == TEAM_SUPER) return -1;
	int Mask = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(m_Core.Team(i) == Team 
			|| (Character(i) 
				&& Character(i)->GetPlayer() 
				&& Character(i)->GetPlayer()->m_ShowOthers)
			|| m_Core.Team(i) == TEAM_SUPER) {
			Mask |= 1 << i;
		}
	}
	return Mask;
}

void CGameTeams::SendAllInfo(int Cid) {
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(m_Core.Team(i) != 0) {
			CNetMsg_Sv_PlayerTeam Msg;
			Msg.m_Team = m_Core.Team(i);
			Msg.m_Cid = i;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, Cid);
		}
	}
}
