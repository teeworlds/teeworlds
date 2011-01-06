#include "teams.h"
#include <engine/shared/config.h>

CGameTeams::CGameTeams(CGameContext *pGameContext) : m_pGameContext(pGameContext)
{
	Reset();
}

void CGameTeams::Reset()
{
	m_Core.Reset();
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		m_TeamState[i] = EMPTY;
		m_TeeFinished[i] = false;
		m_MembersCount[i] = 0;
		m_LastChat[i] = 0;
	}
}

void CGameTeams::OnCharacterStart(int id){
	int Tick = Server()->Tick();
	CCharacter* StartingChar = Character(id);
	if(!StartingChar)
		return;
	if(StartingChar->m_DDRaceState == DDRACE_FINISHED)
		StartingChar->m_DDRaceState = DDRACE_NONE;
	if(m_Core.Team(id) == TEAM_FLOCK || m_Core.Team(id) == TEAM_SUPER)
	{
		StartingChar->m_DDRaceState = DDRACE_STARTED;
		StartingChar->m_StartTime = Tick;
		StartingChar->m_RefreshTime = Tick;
	}
	else
	{
		bool Waiting = false;
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(m_Core.Team(id) == m_Core.Team(i))
			{
				CCharacter* Char = Character(i);
				if(Char->m_DDRaceState == DDRACE_FINISHED)
				{
					Waiting = true;
					if(m_LastChat[id] + Server()->TickSpeed() + g_Config.m_SvChatDelay < Tick)
					{
						char aBuf[128];
						str_format(aBuf, sizeof(aBuf), "%s has finished and didn't go through start yet, wait for him or join another team.", Server()->ClientName(i));
						GameServer()->SendChatTarget(id, aBuf);
						m_LastChat[id] = Tick;
					}
					if(m_LastChat[i] + Server()->TickSpeed() + g_Config.m_SvChatDelay < Tick)
					{
						char aBuf[128];
						str_format(aBuf, sizeof(aBuf), "%s wants to start a new round, kill or walk to start.", Server()->ClientName(id));
						GameServer()->SendChatTarget(i, aBuf);
						m_LastChat[i] = Tick;
					}
				}
			}
		}

		if(m_TeamState[m_Core.Team(id)] <= CLOSED && !Waiting)
		{
			ChangeTeamState(m_Core.Team(id), STARTED);
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(m_Core.Team(id) == m_Core.Team(i))
				{
					CCharacter* Char = Character(i);
					if(Char)
					{
						Char->m_DDRaceState = DDRACE_STARTED;
						Char->m_StartTime = Tick;
						Char->m_RefreshTime = Tick;
					}
				}
			}
		}
	}
}

void CGameTeams::OnCharacterFinish(int id)
{
	if(m_Core.Team(id) == TEAM_FLOCK || m_Core.Team(id) == TEAM_SUPER)
	{
		Character(id)->OnFinish();
	}
	else
	{
		m_TeeFinished[id] = true;
		if(TeamFinished(m_Core.Team(id)))
		{
			//ChangeTeamState(m_Core.Team(id), FINISHED);//TODO: Make it better
			ChangeTeamState(m_Core.Team(id), OPEN);
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(m_Core.Team(id) == m_Core.Team(i))
				{
					CCharacter * Char = Character(i);
					if(Char != 0)
					{
						Char->OnFinish();
						m_TeeFinished[i] = false;
					}
					/*else
					 *{
					 *	m_Core.Team(id) = 0; //i saw zomby =)
					 *}
					 */
				}
			}
			
		}
	}
}

bool CGameTeams::SetCharacterTeam(int id, int Team)
{
	//Check on wrong parameters. +1 for TEAM_SUPER
	if(id < 0 || id >= MAX_CLIENTS || Team < 0 || Team >= MAX_CLIENTS + 1)
		return false;
	//You can join to TEAM_SUPER at any time, but any other group you cannot if it started
	if(Team != TEAM_SUPER && m_TeamState[Team] >= CLOSED)
		return false;
	//No need to switch team if you there
	if(m_Core.Team(id) == Team)
		return false;
	//You cannot be in TEAM_SUPER if you not super
	if(Team == TEAM_SUPER && !Character(id)->m_Super) return false;
	//if you begin race
	if(Character(id)->m_DDRaceState != DDRACE_NONE)
		//you will be killed if you try to join FLOCK
		if(Team == TEAM_FLOCK && m_Core.Team(id) != TEAM_FLOCK)
			Character(id)->GetPlayer()->KillCharacter(WEAPON_GAME);
		else if(Team != TEAM_SUPER)
			return false;
	SetForceCharacterTeam(id, Team);
	
	
	//GameServer()->CreatePlayerSpawn(Character(id)->m_Core.m_Pos, TeamMask());
	return true;
}

void CGameTeams::SetForceCharacterTeam(int id, int Team)
{
	m_TeeFinished[id] = false;
	if(m_Core.Team(id) != TEAM_FLOCK 
		&& m_Core.Team(id) != TEAM_SUPER 
		&& m_TeamState[m_Core.Team(id)] != EMPTY)
	{
		bool NoOneInOldTeam = true;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(i != id && m_Core.Team(id) == m_Core.Team(i))
			{
				NoOneInOldTeam = false;//all good exists someone in old team
				break;
			} 
		if(NoOneInOldTeam)
			m_TeamState[m_Core.Team(id)] = EMPTY;
	}
	if(Count(m_Core.Team(id)) > 0) m_MembersCount[m_Core.Team(id)]--;
	m_Core.Team(id, Team);
	if(m_Core.Team(id) != TEAM_SUPER) m_MembersCount[m_Core.Team(id)]++;
	if(Team != TEAM_SUPER && m_TeamState[Team] == EMPTY)
		ChangeTeamState(Team, OPEN);
	dbg_msg1("Teams", "Id = %d Team = %d", id, Team);
	
	for (int ClientID = 0; ClientID < MAX_CLIENTS; ++ClientID)
	{
		if(Character(ClientID) && Character(ClientID)->GetPlayer()->m_IsUsingDDRaceClient)
			SendTeamsState(ClientID);
	}
}

int CGameTeams::Count(int Team) const
{
	if(Team == TEAM_SUPER) return -1;
	return m_MembersCount[Team];
}



void CGameTeams::ChangeTeamState(int Team, int State)
{
	m_TeamState[Team] = State;
}



bool CGameTeams::TeamFinished(int Team)
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(m_Core.Team(i) == Team && !m_TeeFinished[i])
			return false;
	return true;
}

int CGameTeams::TeamMask(int Team)
{
	if(Team == TEAM_SUPER) return -1;
	int Mask = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
		if((Character(i) && (m_Core.Team(i) == Team || m_Core.Team(i) == TEAM_SUPER)) 
			|| (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == -1))
			Mask |= 1 << i;
	return Mask;
}

void CGameTeams::SendTeamsState(int Cid)
{
	CNetMsg_Cl_TeamsState Msg;
	Msg.m_Tee0 = m_Core.Team(0);
	Msg.m_Tee1 = m_Core.Team(1);
	Msg.m_Tee2 = m_Core.Team(2);
	Msg.m_Tee3 = m_Core.Team(3);
	Msg.m_Tee4 = m_Core.Team(4);
	Msg.m_Tee5 = m_Core.Team(5);
	Msg.m_Tee6 = m_Core.Team(6);
	Msg.m_Tee7 = m_Core.Team(7);
	Msg.m_Tee8 = m_Core.Team(8);
	Msg.m_Tee9 = m_Core.Team(9);
	Msg.m_Tee10 = m_Core.Team(10);
	Msg.m_Tee11 = m_Core.Team(11);
	Msg.m_Tee12 = m_Core.Team(12);
	Msg.m_Tee13 = m_Core.Team(13);
	Msg.m_Tee14 = m_Core.Team(14);
	Msg.m_Tee15 = m_Core.Team(15);
	
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, Cid);
	
}
