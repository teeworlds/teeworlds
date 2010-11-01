#include <new>
#include <stdio.h>
#include <string.h>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "gamemodes/DDRace.h"

#include "score.h"
#include "score/file_score.h"
#include "score/sql_score.h"

enum
{
	RESET,
	NO_RESET
};

void CGameContext::Construct(int Resetting)
{
	m_Resetting = 0;
	m_pServer = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_apPlayers[i] = 0;
		mem_zero(m_pReconnectInfo[i].Ip,sizeof(m_pReconnectInfo[i].Ip));
	}

	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;

	if(Resetting==NO_RESET)
		m_pVoteOptionHeap = new CHeap();
	m_pScore = 0;
}

CGameContext::CGameContext(int Resetting)
{
	Construct(Resetting);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET);
}

CGameContext::~CGameContext()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
	if(!m_Resetting)
		delete m_pVoteOptionHeap;
}

void CGameContext::Clear()
{
	CHeap *pVoteOptionHeap = m_pVoteOptionHeap;
	CVoteOption *pVoteOptionFirst = m_pVoteOptionFirst;
	CVoteOption *pVoteOptionLast = m_pVoteOptionLast;
	CTuningParams Tuning = m_Tuning;

	//bool cheats = m_Cheats;
	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET);
	//this->m_Cheats = cheats;

	m_pVoteOptionHeap = pVoteOptionHeap;
	m_pVoteOptionFirst = pVoteOptionFirst;
	m_pVoteOptionLast = pVoteOptionLast;
	m_Tuning = Tuning;
}


class CCharacter *CGameContext::GetPlayerChar(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || !m_apPlayers[ClientId])
		return 0;
	return m_apPlayers[ClientId]->GetCharacter();
}

void CGameContext::CreateDamageInd(vec2 P, float Angle, int Amount, int Mask)
{
	float a = 3 * 3.14159f / 2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(NETEVENT_DAMAGEIND), Mask);
		if(ev)
		{
			ev->m_X = (int)P.x;
			ev->m_Y = (int)P.y;
			ev->m_Angle = (int)(f*256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 P, int Mask)
{
	// create the event
	NETEVENT_HAMMERHIT *ev = (NETEVENT_HAMMERHIT *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(NETEVENT_HAMMERHIT), Mask);
	if(ev)
	{
		ev->m_X = (int)P.x;
		ev->m_Y = (int)P.y;
	}
}


void CGameContext::CreateExplosion(vec2 P, int Owner, int Weapon, bool NoDamage, int Mask)
{
	// create the event
	NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(NETEVENT_EXPLOSION), Mask);
	if(ev)
	{
		ev->m_X = (int)P.x;
		ev->m_Y = (int)P.y;
	}

	/*if(!NoDamage)
	{*/
		// deal damage
		CCharacter *apEnts[64];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = m_World.FindEntities(P, Radius, (CEntity**)apEnts, 64, NETOBJTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->m_Pos - P;
			vec2 ForceDir(0, 1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if((int)Dmg)
				if((g_Config.m_SvHit||NoDamage) || Owner == apEnts[i]->m_pPlayer->GetCID())
				{
					if(Owner != -1 && apEnts[i]->m_Alive && !apEnts[i]->CanCollide(Owner)) continue;
					apEnts[i]->TakeDamage(ForceDir*Dmg*2, (int)Dmg, Owner, Weapon);
					if(!g_Config.m_SvHit||NoDamage) break;
				}

		}
	//}
}

/*
void create_smoke(vec2 P)
{
	// create the event
	EV_EXPLOSION *ev = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(ev)
	{
		ev->x = (int)P.x;
		ev->y = (int)P.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 P, int Mask)
{
	// create the event
	NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(NETEVENT_SPAWN), Mask);
	if(ev)
	{
		ev->m_X = (int)P.x;
		ev->m_Y = (int)P.y;
	}
}

void CGameContext::CreateDeath(vec2 P, int ClientId, int Mask)
{
	// create the event
	NETEVENT_DEATH *ev = (NETEVENT_DEATH *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(NETEVENT_DEATH), Mask);
	if(ev)
	{
		ev->m_X = (int)P.x;
		ev->m_Y = (int)P.y;
		ev->m_ClientId = ClientId;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, int Mask)
{
	if(Sound < 0)
		return;

	// create a sound
	NETEVENT_SOUNDWORLD *ev = (NETEVENT_SOUNDWORLD *)m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(NETEVENT_SOUNDWORLD), Mask);
	if(ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
		ev->m_SoundId = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if(Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_Soundid = Sound;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, Target);
}


void CGameContext::SendChatTarget(int To, const char *pText)
{
	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_Cid = -1;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
}

void CGameContext::SendChatResponseAll(const char *pLine, void *pUser)
{
	CGameContext *pSelf = (CGameContext *)pUser;

	static volatile int ReentryGuard = 0;

	if(ReentryGuard)
		return;
	ReentryGuard++;

	if(*pLine == '[')
	do
		pLine++;
	while(*(pLine - 2) != ':' && *pLine != 0);//remove the category (e.g. [Console]: No Such Command)

	pSelf->SendChat(-1, CHAT_ALL, pLine);

	ReentryGuard--;
}

void CGameContext::SendChatResponse(const char *pLine, void *pUser)
{
	ChatResponseInfo *pInfo = (ChatResponseInfo *)pUser;

	static volatile int ReentryGuard = 0;

	if(ReentryGuard)
		return;
	ReentryGuard++;

	if(*pLine == '[')
	do
		pLine++;
	while(*(pLine - 2) != ':' && *pLine != 0); // remove the category (e.g. [Console]: No Such Command)

	pInfo->m_GameContext->SendChatTarget(pInfo->m_To, pLine);

	ReentryGuard--;
}

void CGameContext::SendChat(int ChatterClientId, int Team, const char *pText)
{
	char aBuf[256];
	if(ChatterClientId >= 0 && ChatterClientId < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientId, Team, Server()->ClientName(ChatterClientId), pText);
	else
		str_format(aBuf, sizeof(aBuf), "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "chat", aBuf);

	if(Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_Cid = ChatterClientId;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		CTeamsCore * Teams = &((CGameControllerDDRace*)m_pController)->m_Teams.m_Core;
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_Cid = ChatterClientId;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NOSEND, -1);
		//char aBuf[512];
		//str_format(aBuf, sizeof(aBuf), "Chat Team = %d", Team);
		//dbg_msg("Chat", aBuf);
		// send to the clients
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_apPlayers[i] != 0) {
				if(Team == CHAT_SPEC) {
					if(m_apPlayers[i]->GetTeam() == CHAT_SPEC) {
						Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
					}
				} else {
					if(Teams->Team(i) == Team) {
						Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
					}
				}
			}
		}
	}
}

void CGameContext::SendEmoticon(int ClientId, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_Cid = ClientId;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendWeaponPickup(int ClientId, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);
}


void CGameContext::SendBroadcast(const char *pText, int ClientId)
{
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);
}

void CGameContext::SendRecord(int ClientId) {
	CNetMsg_Sv_Record RecordsMsg;
	RecordsMsg.m_PlayerTimeBest = Score()->PlayerData(ClientId)->m_BestTime * 100.0f;//
	RecordsMsg.m_ServerTimeBest = m_pController->m_CurrentRecord * 100.0f;//TODO: finish this
	Server()->SendPackMsg(&RecordsMsg, MSGFLAG_VITAL, ClientId);
}

//
void CGameContext::StartVote(const char *pDesc, const char *pCommand)
{
	// check if a vote is already running
	if(m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq()*25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}


void CGameContext::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameContext::SendVoteSet(int ClientId)
{
	CNetMsg_Sv_VoteSet Msg;
	if(m_VoteCloseTime)
	{
		Msg.m_Timeout = (m_VoteCloseTime-time_get())/time_freq();
		Msg.m_pDescription = m_aVoteDescription;
		Msg.m_pCommand = "";
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pCommand = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);
}

void CGameContext::SendVoteStatus(int ClientId, int Total, int Yes, int No)
{
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = Total - (Yes+No);

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);

}

void CGameContext::AbortVoteKickOnDisconnect(int ClientId)
{
	if(m_VoteCloseTime && !str_comp_num(m_aVoteCommand, "kick ", 5) && str_toint(&m_aVoteCommand[5]) == ClientId)
		m_VoteCloseTime = -1;
}


void CGameContext::CheckPureTuning()
{
	// might not be created yet during start up
	if(!m_pController)
		return;

	if(	str_comp(m_pController->m_pGameType, "DM")==0 ||
		str_comp(m_pController->m_pGameType, "TDM")==0 ||
		str_comp(m_pController->m_pGameType, "CTF")==0)
	{
		CTuningParams P;
		if(mem_comp(&P, &m_Tuning, sizeof(P)) != 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "resetting tuning due to pure server");
			m_Tuning = P;
		}
	}
}

void CGameContext::SendTuningParams(int Cid)
{
	CheckPureTuning();

	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&m_Tuning;
	for(unsigned i = 0; i < sizeof(m_Tuning)/sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, Cid);
}

void CGameContext::OnTick()
{
	// check tuning
	CheckPureTuning();

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	//if(world.paused) // make sure that the game object always updates
	m_pController->Tick();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
			m_apPlayers[i]->Tick();
	}

	// update voting
	if(m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if(m_VoteCloseTime == -1)
		{
			SendChat(-1, CGameContext::CHAT_ALL, "Vote aborted");
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if(m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][64] = {{0}};
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(m_apPlayers[i])
						Server()->GetClientIP(i, aaBuf[i], 64);
				bool aVoteChecked[MAX_CLIENTS] = {0};
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == -1 || aVoteChecked[i])	// don't count in votes by spectators
						continue;
					if(m_VoteKick && 
						GetPlayerChar(m_VoteCreator) && GetPlayerChar(i) &&
						GetPlayerChar(m_VoteCreator)->Team() != GetPlayerChar(i)->Team()) continue;
					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for(int j = i+1; j < MAX_CLIENTS; ++j)
					{
						if(!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if(m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if(ActVote > 0)
						Yes++;
					else if(ActVote < 0)
						No++;
				}

				if(Yes >= (Total/(100/g_Config.m_SvVotePercentage))+1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if(No >= (Total+1)/((1.0/g_Config.m_SvVotePercentage)*100.0))
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if(m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				Console()->ExecuteLine(m_aVoteCommand, 4, -1, CServer::SendRconLineAuthed, Server(), SendChatResponseAll, this);
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote passed");

				if(m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_Last_VoteCall = 0;
			}
			else if(m_VoteEnforce == VOTE_ENFORCE_YES_ADMIN)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf),"Vote passed enforced by '%s'",Server()->ClientName(m_VoteEnforcer));
				Console()->ExecuteLine(m_aVoteCommand, 3, -1, CServer::SendRconLineAuthed, Server(), SendChatResponseAll, this);
				SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				dbg_msg("Vote","Due to vote enforcing, vote level has been set to 3");
				EndVote();
				m_VoteEnforcer = -1;
			}
			else if(m_VoteEnforce == VOTE_ENFORCE_NO_ADMIN)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf),"Vote failed enforced by '%s'",Server()->ClientName(m_VoteEnforcer));
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			}
			else if(m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote failed");
			}
			else if(m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}


#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		for(int i = 0; i < g_Config.m_DbgDummies ; i++)
		{
			CNetObj_PlayerInput Input = {0};
			Input.m_Direction = (i&1)?-1:1;
			m_apPlayers[MAX_CLIENTS-i-1]->OnPredictedInput(&Input);
		}
	}
#endif
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientId)
{
	//world.insert_entity(&players[client_id]);
	m_apPlayers[ClientId]->Respawn();
	
	// init the player

	Score()->PlayerData(ClientId)->Reset();
	Score()->LoadScore(ClientId);
	Score()->PlayerData(ClientId)->m_CurrentTime = Score()->PlayerData(ClientId)->m_BestTime;
	m_apPlayers[ClientId]->m_Score = (Score()->PlayerData(ClientId)->m_BestTime)?Score()->PlayerData(ClientId)->m_BestTime:-9999;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), m_pController->GetTeamName(m_apPlayers[ClientId]->GetTeam()));
	SendChat(-1, CGameContext::CHAT_ALL, aBuf); 
	
	SendChatTarget(ClientId, "DDRace Mod. Version: " DDRACE_VERSION);
	SendChatTarget(ClientId, "Official site: DDRace.info");
	SendChatTarget(ClientId, "For more Info /CMDList");
	SendChatTarget(ClientId, "Or visit DDRace.info");
	SendChatTarget(ClientId, "To see this again say /info");
	SendChatTarget(ClientId, "Note This is an Alpha release, just for testing, your feedback is important!!");

	if(g_Config.m_SvWelcome[0]!=0) SendChatTarget(ClientId,g_Config.m_SvWelcome);
	//str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientId, Server()->ClientName(ClientId), m_apPlayers[ClientId]->GetTeam());
	
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	m_VoteUpdate = true;
}

bool compare_players(CPlayer *pl1, CPlayer *pl2)
{
	if(((pl1->m_Authed >= 0) ? pl1->m_Authed : 0) > ((pl2->m_Authed >= 0) ? pl2->m_Authed : 0))
		return true;
	else
		return false;
}

void CGameContext::OnSetAuthed(int client_id, int Level)
{
	if(m_apPlayers[client_id])
	{
		m_apPlayers[client_id]->m_Authed = Level;
		char buf[11];
		str_format(buf, sizeof(buf), "ban %d %d", client_id, g_Config.m_SvVoteKickBantime);
		if( !strcmp(m_aVoteCommand,buf))
		{
			m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
			dbg_msg("hooks","Aborting vote");
		}
	}
}

void CGameContext::OnClientConnected(int ClientId)
{
	// Check which team the player should be on
	const int StartTeam = g_Config.m_SvTournamentMode ? -1 : m_pController->GetAutoTeam(ClientId);

	m_apPlayers[ClientId] = new(ClientId) CPlayer(this, ClientId, StartTeam);
	//players[client_id].init(client_id);
	//players[client_id].client_id = client_id;

	//(void)m_pController->CheckTeamBalance();

#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		if(ClientId >= MAX_CLIENTS-g_Config.m_DbgDummies)
			return;
	}
#endif

	// send active vote
	if(m_VoteCloseTime)
		SendVoteSet(ClientId);

	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);
}

void CGameContext::OnClientDrop(int ClientId)
{
	AbortVoteKickOnDisconnect(ClientId);
	m_apPlayers[ClientId]->OnDisconnect();
	delete m_apPlayers[ClientId];
	m_apPlayers[ClientId] = 0;

	//(void)m_pController->CheckTeamBalance();
	m_VoteUpdate = true;
}

void CGameContext::OnMessage(int MsgId, CUnpacker *pUnpacker, int ClientId)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientId];

	if(!pRawMsg)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
		return;
	}

	if(MsgId == NETMSGTYPE_CL_SAY)
	{
		CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
		int Team = pMsg->m_Team;
		//if(Team)
		int GameTeam = ((CGameControllerDDRace*)m_pController)->m_Teams.m_Core.Team(pPlayer->GetCID());
		if(Team) {
			Team = (pPlayer->GetTeam() == -1) ? CHAT_SPEC : (GameTeam == 0 ? CHAT_ALL : GameTeam);
		} else {
			Team = CHAT_ALL;
		}

		if(str_length(pMsg->m_pMessage)>370)
		{
			SendChatTarget(ClientId, "Your Message is too long");
			return;
		}

		// check for invalid chars
		unsigned char *pMessage = (unsigned char *)pMsg->m_pMessage;
		while (*pMessage)
		{
			if(*pMessage < 32)
				*pMessage = ' ';
			pMessage++;
		}
		if(pMsg->m_pMessage[0]=='/')
		{
			ChatResponseInfo Info;
			Info.m_GameContext = this;
			Info.m_To = ClientId;

			Console()->ExecuteLine(pMsg->m_pMessage + 1, ((CServer *) Server())->m_aClients[ClientId].m_Authed, ClientId, CServer::SendRconLineAuthed, Server(), SendChatResponse, &Info);
		}
		else if(!str_comp_nocase(pMsg->m_pMessage, "kill"))
			SendChatTarget(ClientId, "kill does nothing, say /kill if you want to die, also you can press f1 and type kill");
		else
		{
			if(m_apPlayers[ClientId]->m_Muted == 0)
			{
				if(/*g_Config.m_SvSpamprotection && */pPlayer->m_Last_Chat && pPlayer->m_Last_Chat + Server()->TickSpeed() + g_Config.m_SvChatDelay > Server()->Tick())
					return;

				SendChat(ClientId, Team, pMsg->m_pMessage);
				
				pPlayer->m_Last_Chat = Server()->Tick();
			}
			else
			{
				char aBuf[64];
				str_format(aBuf,sizeof(aBuf), "You are muted, Please wait for %d second(s)", m_apPlayers[ClientId]->m_Muted / Server()->TickSpeed());
				SendChatTarget(ClientId, aBuf);
			}
		}


	}
	else if(MsgId == NETMSGTYPE_CL_CALLVOTE)
	{
		if(/*g_Config.m_SvSpamprotection && */pPlayer->m_Last_VoteTry && pPlayer->m_Last_VoteTry + Server()->TickSpeed() * g_Config.m_SvVoteDelay > Server()->Tick())
			return;

		int64 Now = Server()->Tick();
		pPlayer->m_Last_VoteTry = Now;
		if(pPlayer->GetTeam() == -1)
		{
			SendChatTarget(ClientId, "Spectators aren't allowed to start a vote.");
			return;
		}

		if(m_VoteCloseTime)
		{
			SendChatTarget(ClientId, "Wait for current vote to end before calling a new one.");
			return;
		}

		int Timeleft = pPlayer->m_Last_VoteCall + Server()->TickSpeed()*60 - Now;
		if(pPlayer->m_Last_VoteCall && Timeleft > 0)
		{
			char aChatmsg[512] = {0};
			str_format(aChatmsg, sizeof(aChatmsg), "You must wait %d seconds before making another vote", (Timeleft/Server()->TickSpeed())+1);
			SendChatTarget(ClientId, aChatmsg);
			return;
		}

		char aChatmsg[512] = {0};
		char aDesc[512] = {0};
		char aCmd[512] = {0};
		CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
		if(str_comp_nocase(pMsg->m_Type, "option") == 0)
		{
			CVoteOption *pOption = m_pVoteOptionFirst;
			static int64 last_mapvote = 0; //floff
			while(pOption)
			{
				if(str_comp_nocase(pMsg->m_Value, pOption->m_aCommand) == 0)
				{
					//str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s'", Server()->ClientName(ClientId), pOption->m_aCommand);
					if(m_apPlayers[ClientId]->m_Authed <= 0 && strncmp(pOption->m_aCommand, "sv_map ", 7) == 0 && time_get() < last_mapvote + (time_freq() * g_Config.m_SvVoteMapTimeDelay))
						{
							char chatmsg[512] = {0};
							str_format(chatmsg, sizeof(chatmsg), "There's a %d second delay between map-votes,Please wait %d Second(s)", g_Config.m_SvVoteMapTimeDelay,((last_mapvote+(g_Config.m_SvVoteMapTimeDelay * time_freq()))/time_freq())-(time_get()/time_freq()));
							SendChatTarget(ClientId, chatmsg);

							return;
						}
					str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s'", Server()->ClientName(ClientId), pOption->m_aCommand);
					str_format(aDesc, sizeof(aDesc), "%s", pOption->m_aCommand);
					str_format(aCmd, sizeof(aCmd), "%s", pOption->m_aCommand);
					last_mapvote = time_get();
					break;
				}

				pOption = pOption->m_pNext;
			}

			if(!pOption)
			{
				if (pPlayer->m_Authed < 3)  // allow admins to call any vote they want
				{
					str_format(aChatmsg, sizeof(aChatmsg), "'%s' isn't an option on this server", pMsg->m_Value);
					SendChatTarget(ClientId, aChatmsg);
					return;
				}
				else
				{
					str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s'", Server()->ClientName(ClientId), pMsg->m_Value);
					str_format(aDesc, sizeof(aDesc), "%s", pMsg->m_Value);
					str_format(aCmd, sizeof(aCmd), "%s", pMsg->m_Value);
				}
			}
			
			last_mapvote = time_get();
			m_VoteKick = false;
		}
		else if(str_comp_nocase(pMsg->m_Type, "kick") == 0)
		{
			if(m_apPlayers[ClientId]->m_Authed == 0 && time_get() < m_apPlayers[ClientId]->m_Last_KickVote + (time_freq() * 5))
			return;
			else if(m_apPlayers[ClientId]->m_Authed == 0 && time_get() < m_apPlayers[ClientId]->m_Last_KickVote + (time_freq() * g_Config.m_SvVoteKickTimeDelay))
			{
				char chatmsg[512] = {0};
				str_format(chatmsg, sizeof(chatmsg), "There's a %d second wait time between kick votes for each player please wait %d second(s)",
				g_Config.m_SvVoteKickTimeDelay,
				((m_apPlayers[ClientId]->m_Last_KickVote + (m_apPlayers[ClientId]->m_Last_KickVote*time_freq()))/time_freq())-(time_get()/time_freq())
				);
				SendChatTarget(ClientId, chatmsg);
				m_apPlayers[ClientId]->m_Last_KickVote = time_get();
				return;
			}
			else if(!g_Config.m_SvVoteKick && pPlayer->m_Authed < 2) // allow admins to call kick votes even if they are forbidden
			{
				SendChatTarget(ClientId, "Server does not allow voting to kick players");
				m_apPlayers[ClientId]->m_Last_KickVote = time_get();
				return;
			}

			int KickId = str_toint(pMsg->m_Value);
			if(KickId < 0 || KickId >= MAX_CLIENTS || !m_apPlayers[KickId])
			{
				SendChatTarget(ClientId, "Invalid client id to kick");
				return;
			}
			if(KickId == ClientId)
			{
				SendChatTarget(ClientId, "You can't kick yourself");
				return;
			}
			if(compare_players(m_apPlayers[KickId], pPlayer))
			{
				SendChatTarget(ClientId, "You can't kick admins");
				m_apPlayers[ClientId]->m_Last_KickVote = time_get();
				char aBufKick[128];
				str_format(aBufKick, sizeof(aBufKick), "'%s' called for vote to kick you", Server()->ClientName(ClientId));
				SendChatTarget(KickId, aBufKick);
				return;
			}
			if(GetPlayerChar(ClientId) && GetPlayerChar(KickId) && GetPlayerChar(ClientId)->Team() != GetPlayerChar(KickId)->Team())
			{
				SendChatTarget(ClientId, "You can kick only your team member");
				m_apPlayers[ClientId]->m_Last_KickVote = time_get();
				return;
			}
			const char *pReason = "No reason given";
			for(const char *p = pMsg->m_Value; *p; ++p)
			{
				if(*p == ' ')
				{
					pReason = p+1;
					break;
				}
			}
			
			str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to kick '%s' (%s)", Server()->ClientName(ClientId), Server()->ClientName(KickId), pReason);
			str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickId));
			if(!g_Config.m_SvVoteKickBantime)
				str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickId);
			else
			{
				char aBuf[64] = {0};
				Server()->GetClientIP(KickId, aBuf, sizeof(aBuf));
				str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aBuf, g_Config.m_SvVoteKickBantime);
			}
			m_apPlayers[ClientId]->m_Last_KickVote = time_get();
			m_VoteKick = true;
		}

		if(aCmd[0])
		{
			SendChat(-1, CGameContext::CHAT_ALL, aChatmsg);
			StartVote(aDesc, aCmd);
			pPlayer->m_Vote = 1;
			pPlayer->m_VotePos = m_VotePos = 1;
			m_VoteCreator = ClientId;
			pPlayer->m_Last_VoteCall = Now;
		}
	}
	else if(MsgId == NETMSGTYPE_CL_VOTE)
	{
		if(!m_VoteCloseTime)
			return;

		if(pPlayer->m_Vote == 0)
		{
			CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
			if(!pMsg->m_Vote)
				return;

			pPlayer->m_Vote = pMsg->m_Vote;
			pPlayer->m_VotePos = ++m_VotePos;
			m_VoteUpdate = true;
		}
	}
	else if(MsgId == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
	{
		CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

		if(pPlayer->GetTeam() == pMsg->m_Team || (/*g_Config.m_SvSpamprotection &&*/ pPlayer->m_Last_SetTeam && pPlayer->m_Last_SetTeam+Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay > Server()->Tick()))
			return;

		// Switch team on given client and kill/respawn him
		if(m_pController->CanJoinTeam(pMsg->m_Team, ClientId))
		{
			//if(m_pController->CanChangeTeam(pPlayer, pMsg->m_Team))
			//{
			//CCharacter* pChr=GetPlayerChar(ClientId);
			if(pPlayer->GetTeam()==-1 && pPlayer->m_InfoSaved)
				SendChatTarget(ClientId,"Use /pause first then you can kill");
			else{
				pPlayer->m_Last_SetTeam = Server()->Tick();
				if(pPlayer->GetTeam() == -1 || pMsg->m_Team == -1)
					m_VoteUpdate = true;
				pPlayer->SetTeam(pMsg->m_Team);
			}

				//if(pChr && pMsg->m_Team!=-1 && pChr->m_Paused)
					//pChr->LoadPauseData();
				//TODO:Check if this system Works

				//(void)m_pController->CheckTeamBalance();
			//}
			//else
				//SendBroadcast("Teams must be balanced, please join other team", ClientId);
		}
		else
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Only %d active players are allowed", g_Config.m_SvMaxClients-g_Config.m_SvSpectatorSlots);
			SendBroadcast(aBuf, ClientId);
		}
	}
	else if (MsgId == NETMSGTYPE_CL_ISDDRACE)
	{
		pPlayer->m_IsUsingDDRaceClient = true;
		
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%d use DDRace Client", ClientId);
		dbg_msg("DDRace", aBuf);
		
		//first update his teams state
		((CGameControllerDDRace*)m_pController)->m_Teams.SendTeamsState(ClientId);
		
		//second give him records
		SendRecord(ClientId);
		
		
		//third give him others current time for table score
		if(g_Config.m_SvHideScore) return;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_apPlayers[i] && Score()->PlayerData(i)->m_CurrentTime > 0)
			{
				char aBuf[16];
				str_format(aBuf, sizeof(aBuf), "%.0f", Score()->PlayerData(i)->m_CurrentTime*100.0f); // damn ugly but the only way i know to do it
				int TimeToSend;
				sscanf(aBuf, "%d", &TimeToSend);
				CNetMsg_Sv_PlayerTime Msg;
				Msg.m_Time = TimeToSend;
				Msg.m_Cid = i;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);
			}
		}
	}
	else if(MsgId == NETMSGTYPE_CL_CHANGEINFO || MsgId == NETMSGTYPE_CL_STARTINFO)
	{
		CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;

		if(/*g_Config.m_SvSpamprotection &&*/ pPlayer->m_Last_ChangeInfo && pPlayer->m_Last_ChangeInfo + Server()->TickSpeed() * g_Config.m_SvInfoChangeDelay > Server()->Tick())
			return;

		pPlayer->m_Last_ChangeInfo = time_get();
		if(!pPlayer->m_ColorSet|| g_Config.m_SvAllowColorChange)
		{
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
		}

		// copy old name
		char aOldName[MAX_NAME_LENGTH];
		str_copy(aOldName, Server()->ClientName(ClientId), MAX_NAME_LENGTH);

		Server()->SetClientName(ClientId, pMsg->m_pName);
		if(MsgId == NETMSGTYPE_CL_CHANGEINFO && str_comp(aOldName, Server()->ClientName(ClientId)) != 0)
		{
			char aChatText[256];
			str_format(aChatText, sizeof(aChatText), "'%s' changed name to '%s'", aOldName, Server()->ClientName(ClientId));
			SendChat(-1, CGameContext::CHAT_ALL, aChatText);
		}

		// set skin
		str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));

		//m_pController->OnPlayerInfoChange(pPlayer);

		if(MsgId == NETMSGTYPE_CL_STARTINFO)
		{
			// send vote options
			CNetMsg_Sv_VoteClearOptions ClearMsg;
			Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientId);
			CVoteOption *pCurrent = m_pVoteOptionFirst;
			while(pCurrent)
			{
				CNetMsg_Sv_VoteOption OptionMsg;
				OptionMsg.m_pCommand = pCurrent->m_aCommand;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientId);
				pCurrent = pCurrent->m_pNext;
			}

			// send tuning parameters to client
			SendTuningParams(ClientId);

			//
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientId);
		}
	}
	else if(MsgId == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
	{
		CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

		if(/*g_Config.m_SvSpamprotection &&*/ pPlayer->m_Last_Emote && pPlayer->m_Last_Emote+Server()->TickSpeed()*g_Config.m_SvEmoticonDelay > Server()->Tick())
			return;

		pPlayer->m_Last_Emote = Server()->Tick();

		SendEmoticon(ClientId, pMsg->m_Emoticon);
		CCharacter* pChr = pPlayer->GetCharacter();
		if(pChr && g_Config.m_SvEmotionalTees && pChr->m_EyeEmote)
		{
			switch(pMsg->m_Emoticon)
			{
				case EMOTICON_2:
				case EMOTICON_8:
					pChr->m_EmoteType = EMOTE_SURPRISE;
					break;
				case EMOTICON_1:
				case EMOTICON_4:
				case EMOTICON_7:
				case EMOTICON_13:
					pChr->m_EmoteType = EMOTE_BLINK;
					break;
				case EMOTICON_3:
				case EMOTICON_6:
					pChr->m_EmoteType = EMOTE_HAPPY;
					break;
				case EMOTICON_9:
				case EMOTICON_15:
					pChr->m_EmoteType = EMOTE_PAIN;
					break;
				case EMOTICON_10:
				case EMOTICON_11:
				case EMOTICON_12:
					pChr->m_EmoteType = EMOTE_ANGRY;
					break;
				default:
					break;
			}
			pChr->m_EmoteStop = Server()->Tick() + 2 * Server()->TickSpeed();
		}
	}
	
	else if(MsgId == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
	{
		if(pPlayer->m_Last_Kill && pPlayer->m_Last_Kill+Server()->TickSpeed() * g_Config.m_SvKillDelay > Server()->Tick())
			return;

		pPlayer->m_Last_Kill = Server()->Tick();
		pPlayer->KillCharacter(WEAPON_SELF);
		pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed() * g_Config.m_SvSuicidePenalty;
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if(pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams P;
	*pSelf->Tuning() = P;
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for(int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->GetString(0));
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", Victim, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	if(!pSelf->m_apPlayers[Victim] || !compare_players(pSelf->m_apPlayers[ClientId], pSelf->m_apPlayers[Victim]))
	if(!pSelf->m_apPlayers[Victim])
		return;

	pSelf->m_apPlayers[ClientId]->SetTeam(Team);
	//(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	// check for valid option
	if(!pSelf->Console()->LineIsValid(pResult->GetString(0)))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pResult->GetString(0));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	int Len = str_length(pResult->GetString(0));

	CGameContext::CVoteOption *pOption = (CGameContext::CVoteOption *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CGameContext::CVoteOption) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if(pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if(!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	mem_copy(pOption->m_aCommand, pResult->GetString(0), Len+1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s'", pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	CNetMsg_Sv_VoteOption OptionMsg;
	OptionMsg.m_pCommand = pOption->m_aCommand;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared vote options");

	CNetMsg_Sv_VoteClearOptions ClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&ClearOptionsMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[64];
	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES_ADMIN;
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO_ADMIN;
	str_format(aBuf, sizeof(aBuf), "vote forced to %s by %s", pResult->GetString(0),pSelf->Server()->ClientName(ClientID));
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pResult->GetString(0));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	pSelf->m_VoteEnforcer = ClientID;
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData, -1);
	if(pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}


bool CGameContext::CheatsAvailable(IConsole *pConsole, int ClientId)
{
	if(!g_Config.m_SvCheats)
	{
		pConsole->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "cheats", "Cheats are not available on this server.");
	}
	return g_Config.m_SvCheats;
}

void CGameContext::ConGoLeft(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId))
		return;
	int Victim=-1;
	if(pResult->NumArguments())
		Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(Victim ==-1 || Victim == ClientId)
	{
		CCharacter* chr = pSelf->GetPlayerChar(ClientId);
		if(chr)
		{
			chr->m_Core.m_Pos.x -= 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->m_Core.m_Pos.x -= 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else
	{
		CServer* pServ = (CServer*)pSelf->Server();
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pSelf->m_apPlayers[ClientId]->m_Authed>1)?"You can't move a player with the same or higher rank":"You can't move others as a helper");
	}
}

void CGameContext::ConGoRight(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId))
		return;
	int Victim=-1;
	if(pResult->NumArguments())
		Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(Victim ==-1 || Victim == ClientId)
	{
		CCharacter* chr = pSelf->GetPlayerChar(ClientId);
		if(chr)
		{
			chr->m_Core.m_Pos.x += 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->m_Core.m_Pos.x += 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else
	{
		CServer* pServ = (CServer*)pSelf->Server();
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pSelf->m_apPlayers[ClientId]->m_Authed>1)?"You can't move a player with the same or higher rank":"You can't move others as a helper");
	}
}

void CGameContext::ConGoDown(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId))
		return;
	int Victim=-1;
	if(pResult->NumArguments())
		Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(Victim ==-1 || Victim == ClientId)
	{
		CCharacter* chr = pSelf->GetPlayerChar(ClientId);
		if(chr)
		{
			chr->m_Core.m_Pos.y += 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->m_Core.m_Pos.y += 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else
	{
		CServer* pServ = (CServer*)pSelf->Server();
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pSelf->m_apPlayers[ClientId]->m_Authed>1)?"You can't move a player with the same or higher rank":"You can't move others as a helper");
	}
}

void CGameContext::ConGoUp(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId))
		return;
	int Victim=-1;
	if(pResult->NumArguments())
		Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(Victim ==-1 || Victim == ClientId)
	{
		CCharacter* chr = pSelf->GetPlayerChar(ClientId);
		if(chr)
		{
			chr->m_Core.m_Pos.y -= 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->m_Core.m_Pos.y -= 32;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
	else
	{
		CServer* pServ = (CServer*)pSelf->Server();
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pSelf->m_apPlayers[ClientId]->m_Authed>1)?"You can't move a player with the same or higher rank":"You can't move others as a helper");
	}
}

void CGameContext::ConMute(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Seconds = pResult->GetInteger(1);
	char buf[512];
	if(Seconds < 10)
		Seconds = 10;
	if(pSelf->m_apPlayers[Victim] && (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim])))
	{
		if(pSelf->m_apPlayers[Victim]->m_Muted < Seconds * pSelf->Server()->TickSpeed())
		{
			pSelf->m_apPlayers[Victim]->m_Muted = Seconds * pSelf->Server()->TickSpeed();
		}
		else
			Seconds = pSelf->m_apPlayers[Victim]->m_Muted / pSelf->Server()->TickSpeed();
		str_format(buf, sizeof(buf), "%s muted by %s for %d seconds", pSelf->Server()->ClientName(Victim), pSelf->Server()->ClientName(ClientId), Seconds);
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, buf);
	}
}

void CGameContext::ConSetlvl3(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]) || ClientId == Victim))
	{
		pSelf->m_apPlayers[Victim]->m_Authed = 3;
		pServ->SetRconLevel(Victim, 3);
	}
}

void CGameContext::ConSetlvl2(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]) || ClientId == Victim))
	{
		pSelf->m_apPlayers[Victim]->m_Authed = 2;
		pServ->SetRconLevel(Victim, 2);
	}
}

void CGameContext::ConSetlvl1(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]) || ClientId == Victim))
	{
		pSelf->m_apPlayers[Victim]->m_Authed = 1;
		pServ->SetRconLevel(Victim, 1);
	}
}

void CGameContext::ConLogOut(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = ClientId;
	if(pResult->NumArguments() && pSelf->m_apPlayers[Victim]->m_Authed > 1)
		Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]) || ClientId == Victim))
	{
		pSelf->m_apPlayers[Victim]->m_Authed = -1;
		pServ->SetRconLevel(Victim, -1);
	}
}

void CGameContext::ConKillPlayer(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(!pSelf->m_apPlayers[Victim])
		return;

	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		pSelf->m_apPlayers[Victim]->KillCharacter(WEAPON_GAME);
		char buf[512];
		str_format(buf, sizeof(buf), "%s killed by %s", pSelf->Server()->ClientName(Victim), pSelf->Server()->ClientName(ClientId));
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, buf);
	}
}

void CGameContext::ConNinjaMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;

	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		chr->GiveNinja();
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConNinja(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(chr)
	{
		if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
		{
			chr->GiveNinja();
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}


void CGameContext::ConHammer(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	char buf[128];

	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int type = pResult->GetInteger(1);
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	CServer* pServ = (CServer*)pSelf->Server();
	if(type>3 || type<0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Select hammer between 0 and 3");
	}
	else
	{
		if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
		{
			chr->m_HammerType = type;
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
			str_format(buf, sizeof(buf), "Hammer of '%s' ClientId=%d setted to %d", pServ->ClientName(ClientId), Victim, type);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
		}
	}
}

void CGameContext::ConHammerMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	char buf[128];
	int type = pResult->GetInteger(0);
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(!chr)
		return;
	CServer* pServ = (CServer*)pSelf->Server();
	if(type>3 || type<0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Select hammer between 0 and 3");
	}
	else
	{
		chr->m_HammerType = type;
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
		str_format(buf, sizeof(buf), "Hammer setted to %d",type);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}


void CGameContext::ConSuper(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr && !chr->m_Super)
		{
			chr->m_Super = true;
			chr->UnFreeze();
			chr->m_TeamBeforeSuper = chr->Team();
			dbg_msg("Teamb4super","%d",chr->m_TeamBeforeSuper = chr->Team());
			chr->Teams()->SetCharacterTeam(Victim, TEAM_SUPER);
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConUnSuper(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr && chr->m_Super)
		{
			chr->m_Super = false;
			chr->Teams()->SetForceCharacterTeam(Victim, chr->m_TeamBeforeSuper);
		}
	}
}

void CGameContext::ConSuperMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	if(pSelf->m_apPlayers[ClientId])
	{
		CCharacter* chr = pSelf->GetPlayerChar(ClientId);
		if(chr && !chr->m_Super)
		{
			chr->m_Super = true;
			chr->UnFreeze();
			chr->m_TeamBeforeSuper = chr->Team();
			dbg_msg("Teamb4super","%d",chr->m_TeamBeforeSuper = chr->Team());
			chr->Teams()->SetCharacterTeam(ClientId, TEAM_SUPER);
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConUnSuperMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	if(pSelf->m_apPlayers[ClientId])
	{
		CCharacter* chr = pSelf->GetPlayerChar(ClientId);
		if(chr && chr->m_Super)
		{
			chr->m_Super = false;
			chr->Teams()->SetForceCharacterTeam(ClientId, chr->m_TeamBeforeSuper);
		}
	}
}

void CGameContext::ConShotgun(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->GiveWeapon(WEAPON_SHOTGUN,-1);
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConShotgunMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		chr->GiveWeapon(WEAPON_SHOTGUN,-1);
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConGrenade(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->GiveWeapon(WEAPON_GRENADE,-1);
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConGrenadeMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		chr->GiveWeapon(WEAPON_GRENADE,-1);
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConRifle(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->GiveWeapon(WEAPON_RIFLE,-1);
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConRifleMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		chr->GiveWeapon(WEAPON_RIFLE,-1);
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConWeapons(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			chr->GiveAllWeapons();
			if(!g_Config.m_SvCheatTime)
				chr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConWeaponsMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		chr->GiveAllWeapons();
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConUnShotgun(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			if(chr->m_ActiveWeapon == WEAPON_SHOTGUN)
				chr->m_ActiveWeapon = WEAPON_GUN;
			chr->m_aWeapons[WEAPON_SHOTGUN].m_Got = false;
		}
	}
}

void CGameContext::ConUnShotgunMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		if(chr->m_ActiveWeapon == WEAPON_SHOTGUN)
			chr->m_ActiveWeapon = WEAPON_GUN;
		chr->m_aWeapons[WEAPON_SHOTGUN].m_Got = false;
	}
}

void CGameContext::ConUnGrenade(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			if(chr->m_ActiveWeapon == WEAPON_GRENADE)
				chr->m_ActiveWeapon = WEAPON_GUN;
			chr->m_aWeapons[WEAPON_GRENADE].m_Got = false;
		}
	}
}

void CGameContext::ConUnGrenadeMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		if(chr->m_ActiveWeapon == WEAPON_GRENADE)
			chr->m_ActiveWeapon = WEAPON_GUN;
		chr->m_aWeapons[WEAPON_GRENADE].m_Got = false;
	}
}

void CGameContext::ConUnRifle(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			if(chr->m_ActiveWeapon == WEAPON_RIFLE)
				chr->m_ActiveWeapon = WEAPON_GUN;
			chr->m_aWeapons[WEAPON_RIFLE].m_Got = false;
		}
	}
}

void CGameContext::ConUnRifleMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		if(chr->m_ActiveWeapon == WEAPON_RIFLE)
			chr->m_ActiveWeapon = WEAPON_GUN;
		chr->m_aWeapons[WEAPON_RIFLE].m_Got = false;
	}
}

void CGameContext::ConUnWeapons(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(chr)
		{
			if(chr->m_ActiveWeapon == WEAPON_SHOTGUN || chr->m_ActiveWeapon == WEAPON_GRENADE || chr->m_ActiveWeapon == WEAPON_RIFLE)
				chr->m_ActiveWeapon = WEAPON_GUN;
			chr->m_aWeapons[WEAPON_SHOTGUN].m_Got = false;
			chr->m_aWeapons[WEAPON_GRENADE].m_Got = false;
			chr->m_aWeapons[WEAPON_RIFLE].m_Got = false;
		}
	}
}

void CGameContext::ConUnWeaponsMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	CCharacter* chr = pSelf->GetPlayerChar(ClientId);
	if(chr)
	{
		if(chr->m_ActiveWeapon == WEAPON_SHOTGUN || chr->m_ActiveWeapon == WEAPON_GRENADE || chr->m_ActiveWeapon == WEAPON_RIFLE)
			chr->m_ActiveWeapon = WEAPON_GUN;
		chr->m_aWeapons[WEAPON_SHOTGUN].m_Got = false;
		chr->m_aWeapons[WEAPON_GRENADE].m_Got = false;
		chr->m_aWeapons[WEAPON_RIFLE].m_Got = false;
	}
}

void CGameContext::ConTeleport(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int cid2 = clamp(pResult->GetInteger(1), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && pSelf->m_apPlayers[cid2])
	{
		if(ClientId==Victim
			|| (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]) && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[cid2]))
			|| (compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]) && cid2==ClientId))
		{
			CCharacter* chr = pSelf->GetPlayerChar(Victim);
			if(chr)
			{
				chr->m_Core.m_Pos = pSelf->m_apPlayers[cid2]->m_ViewPos;
				if(!g_Config.m_SvCheatTime)
					chr->m_DDRaceState = DDRACE_CHEAT;
			}
		}
	}
}

void CGameContext::ConTimerStop(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char buf[128];
	CServer* pServ = (CServer*)pSelf->Server();
	if(!g_Config.m_SvTimer)
	{

		int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(!chr)
			return;
		if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
		{
			chr->m_DDRaceState=DDRACE_CHEAT;
			str_format(buf, sizeof(buf), "'%s' ClientId=%d Hasn't time now (Timer Stopped)", pServ->ClientName(ClientId), Victim);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
		}
	}
	else
	{

		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Timer commands are disabled");
	}
}

void CGameContext::ConTimerStart(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char buf[128];
	CServer* pServ = (CServer*)pSelf->Server();
	if(!g_Config.m_SvTimer)
	{
		int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(!chr)
			return;
		if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
		{
			chr->m_DDRaceState = DDRACE_STARTED;
			str_format(buf, sizeof(buf), "'%s' ClientId=%d Has time now (Timer Started)", pServ->ClientName(ClientId), Victim);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
		}
	}
	else
	{

		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Timer commands are disabled");
	}
}

void CGameContext::ConTimerZero(IConsole::IResult *pResult, void *pUserData, int ClientId)
{

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	char buf[128];
	CServer* pServ = (CServer*)pSelf->Server();
	if(!g_Config.m_SvTimer)
	{
		int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);

		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(!chr)
			return;
		if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
		{
			chr->m_StartTime = pSelf->Server()->Tick();
			chr->m_RefreshTime = pSelf->Server()->Tick();
			chr->m_DDRaceState=DDRACE_CHEAT;
			str_format(buf, sizeof(buf), "'%s' ClientId=%d time has been reset & stopped.", pServ->ClientName(ClientId), Victim);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
		}
	}
	else
	{

		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Timer commands are disabled");
	}
}

void CGameContext::ConTimerReStart(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	char buf[128];
	CServer* pServ = (CServer*)pSelf->Server();
	if(!g_Config.m_SvTimer)
	{
		int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);

		CCharacter* chr = pSelf->GetPlayerChar(Victim);
		if(!chr)
			return;
		if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
		{
			chr->m_StartTime = pSelf->Server()->Tick();
			chr->m_RefreshTime = pSelf->Server()->Tick();
			chr->m_DDRaceState=DDRACE_STARTED;
			str_format(buf, sizeof(buf), "'%s' ClientId=%d time has been reset & stopped.", pServ->ClientName(ClientId), Victim);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
		}
	}
	else
	{

		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Timer commands are disabled");
	}
}

void CGameContext::ConFreeze(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	//if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	char buf[128];
	int time=-1;
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pResult->NumArguments()>1)
		time = clamp(pResult->GetInteger(1), -1, 29999);
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		chr->Freeze(((time!=0&&time!=-1)?(pSelf->Server()->TickSpeed()*time):(-1)));
		chr->m_pPlayer->m_RconFreeze = true;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(buf, sizeof(buf), "'%s' ClientId=%d has been Frozen.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}

}

void CGameContext::ConUnFreeze(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	//if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	char buf[128];
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	chr->m_FreezeTime=2;
	chr->m_pPlayer->m_RconFreeze = false;
	CServer* pServ = (CServer*)pSelf->Server();
	str_format(buf, sizeof(buf), "'%s' ClientId=%d has been UnFreezed.", pServ->ClientName(ClientId), Victim);
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);

}

void CGameContext::ConInvisMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	//if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	if(!pSelf->m_apPlayers[ClientId])
		return;
	pSelf->m_apPlayers[ClientId]->m_Invisible = true;
}

void CGameContext::ConVisMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	//if(!pSelf->CheatsAvailable(pSelf->Console(), ClientId)) return;
	if(!pSelf->m_apPlayers[ClientId])
		return;
	pSelf->m_apPlayers[ClientId]->m_Invisible = false;
}

void CGameContext::ConInvis(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_apPlayers[ClientId])
		return;
	char buf[128];
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		pSelf->m_apPlayers[Victim]->m_Invisible = true;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(buf, sizeof(buf), "'%s' ClientId=%d is now invisible.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConVis(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_apPlayers[ClientId])
		return;
	char buf[128];
	int Victim = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[Victim] && compare_players(pSelf->m_apPlayers[ClientId],pSelf->m_apPlayers[Victim]))
	{
		pSelf->m_apPlayers[Victim]->m_Invisible = false;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(buf, sizeof(buf), "'%s' ClientId=%d is visible.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConCredits(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "This mod was originally created by 3DA");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Now it is maintained & coded by:");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "[Egypt]GreYFoX@GTi and [BlackTee]den among others:");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Code: heinrich5991");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Documentation: Zeta-Hoernchen");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Code (in the past): LemonFace, noother and Fluxid");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Please check the changelog on DDRace.info.");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Also the commit log on github.com/GreYFoXGTi/DDRace.");
}

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "DDRace Mod. Version: " DDRACE_VERSION);
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Official site: DDRace.info");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "For more Info /CMDList");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Or visit DDRace.info");
}

void CGameContext::ConCmdList(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;


	if(pSelf->m_apPlayers[ClientId]->m_Authed == 3)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "O Really!!, You call yourself an admin!!");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "check the documentation on DDRace.info");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "leave cmdlist for others.. too many commands to show you here");
	}
	else
		pSelf->Console()->List(pSelf->m_apPlayers[ClientId]->m_Authed, CFGFLAG_SERVER);
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pResult->NumArguments() == 0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "/cmdlist will show a list of all chat commands");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "/help + any command will show you the help for this command");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Example /help flags will display the help about ");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		IConsole::CCommandInfo *pCmdInfo = pSelf->Console()->GetCommandInfo(pArg, CFGFLAG_SERVER);
		if(pCmdInfo && pCmdInfo->m_pHelp)
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", pCmdInfo->m_pHelp);
		else
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Command is either unknown or you have given a blank command without any parameters.");
	}
}

void CGameContext::ConFlags(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	char buf[64];
	float temp1;
	float temp2;
	pSelf->m_Tuning.Get("player_collision",&temp1);
	pSelf->m_Tuning.Get("player_hooking",&temp2);
	str_format(buf, sizeof(buf), "Flags: cheats[%s]%s%s collision[%s] hooking[%s]",
		g_Config.m_SvCheats?"yes":"no",
		(g_Config.m_SvCheats)?" w/Time":"",
		(g_Config.m_SvCheats)?(g_Config.m_SvCheatTime)?"[yes]":"[no]":"",
		temp1?"yes":"no",
		temp2?"yes":"no");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);

	str_format(buf, sizeof(buf), "endless hook[%s] weapons effect others[%s]",g_Config.m_SvEndlessDrag?"yes":"no",g_Config.m_SvHit?"yes":"no");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	if(g_Config.m_SvPauseable)
	{
		str_format(buf, sizeof(buf), "Server Allows /pause with%s",g_Config.m_SvPauseTime?" time pause.":"out time pause.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConRules(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	bool printed=false;
	if(g_Config.m_SvDDRaceRules)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No blocking.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No insulting / spamming.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No fun voting / vote spamming.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Breaking any of these rules will result in a penalty, decided by server admins.");
		printed=true;
	}
	if(g_Config.m_SvRulesLine1[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine1);
		printed=true;
	}
	if(g_Config.m_SvRulesLine2[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine2);
		printed=true;
	}
	if(g_Config.m_SvRulesLine3[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine3);
		printed=true;
	}
	if(g_Config.m_SvRulesLine4[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine4);
		printed=true;
	}
	if(g_Config.m_SvRulesLine5[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine5);
		printed=true;
	}
	if(g_Config.m_SvRulesLine6[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine6);
		printed=true;
	}
	if(g_Config.m_SvRulesLine7[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine7);
		printed=true;
	}
	if(g_Config.m_SvRulesLine8[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine8);
		printed=true;
	}
	if(g_Config.m_SvRulesLine9[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine9);
		printed=true;
	}
	if(g_Config.m_SvRulesLine10[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine10);
		printed=true;
	}
	if(!printed)
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No Rules Defined, Kill em all!!");
}

void CGameContext::ConKill(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(pPlayer->m_Last_Kill && pPlayer->m_Last_Kill + pSelf->Server()->TickSpeed() * g_Config.m_SvKillDelay > pSelf->Server()->Tick())
		return;

	pPlayer->m_Last_Kill = pSelf->Server()->Tick();
	pPlayer->KillCharacter(WEAPON_SELF);
	pPlayer->m_RespawnTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * g_Config.m_SvSuicidePenalty;
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(g_Config.m_SvPauseable)
	{
		CCharacter* chr = pPlayer->GetCharacter();
		if(!pPlayer->GetTeam() && chr && (!chr->m_aWeapons[WEAPON_NINJA].m_Got || chr->m_FreezeTime) && chr->IsGrounded() && chr->m_Pos==chr->m_PrevPos && !pPlayer->m_InfoSaved)
		{
			pPlayer->SaveCharacter();
			pPlayer->SetTeam(-1);
			pPlayer->m_InfoSaved = true;
		}
		else if(pPlayer->GetTeam()==-1 && pPlayer->m_InfoSaved)
		{
			pPlayer->m_InfoSaved = false;
			pPlayer->m_PauseInfo.m_Respawn = true;
			pPlayer->SetTeam(0);
			//pPlayer->LoadCharacter();//TODO:Check if this system Works
		}
		else if(chr)
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (chr->m_aWeapons[WEAPON_NINJA].m_Got)?"You can't use /pause while you are a ninja":(!chr->IsGrounded())?"You can't use /pause while you are a in air":"You can't use /pause while you are moving");
		else
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No pause data saved.");
	}
	else
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Pause isn't allowed on this server.");
}

void CGameContext::ConTop5(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(g_Config.m_SvHideScore)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Showing the top 5 is not allowed on this server.");
		return;
	}

	if(pResult->NumArguments() > 0)
		pSelf->Score()->ShowTop5(pPlayer->GetCID(), pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTop5(pPlayer->GetCID());
}

void CGameContext::ConRank(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(pResult->NumArguments() > 0)
		if(!g_Config.m_SvHideScore)
		pSelf->Score()->ShowRank(pPlayer->GetCID(), pResult->GetString(0), true);
	else
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Showing the rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowRank(pPlayer->GetCID(), pSelf->Server()->ClientName(ClientId));
}

void CGameContext::ConBroadTime(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter* pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();
	if(pChr)
		pChr->m_BroadTime = !pChr->m_BroadTime;
}

void CGameContext::ConJoinTeam(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(pResult->NumArguments() > 0)
	{
		if(pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You can't change teams while you are dead/a spectator.");
		}
		else
		{
			if(((CGameControllerDDRace*)pSelf->m_pController)->m_Teams.SetCharacterTeam(pPlayer->GetCID(), pResult->GetInteger(0)))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s joined team %d", pSelf->Server()->ClientName(pPlayer->GetCID()), pResult->GetInteger(0));
				pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			}
			else
			{
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You cannot join this team");
			}
		}
	}
	else
	{
		char aBuf[512];
		if(pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You can't check your team while you are dead/a spectator.");
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "You are in team %d", pPlayer->GetCharacter()->Team());
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
		}
	}
}


void CGameContext::ConToggleFly(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter* pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();
	if(pChr && pChr->m_Super)
	{
		pChr->m_Fly = !pChr->m_Fly;
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pChr->m_Fly) ? "Fly enabled" : "Fly disabled");
	}
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256 + 24];
	
	str_format(aBuf, 256 + 24, "%s %s", pSelf->Server()->ClientName(ClientId), pResult->GetString(0));
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

void CGameContext::ConToggleEyeEmote(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	CCharacter *pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();

	if(pChr)
	{
		pChr->m_EyeEmote = !pChr->m_EyeEmote;
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pChr->m_EyeEmote) ? "You can now use the preset eye emotes." : "You don't have any eye emotes, remember to bind some. (until you die)");
	}
}

void CGameContext::ConToggleBroadcast(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	CCharacter *pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();

	if(pChr)
		pChr->m_BroadCast = !pChr->m_BroadCast;
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	CCharacter *pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();
	
	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Example: /emote surprise 10 for 10 seconds or /emote surprise (default 1 second)");
	}
	else 
	{
	  if (pChr)
		{
			if (!str_comp(pResult->GetString(0), "angry"))
			  pChr->m_EmoteType = EMOTE_ANGRY;
			else if (!str_comp(pResult->GetString(0), "blink"))
			  pChr->m_EmoteType = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "close"))
			  pChr->m_EmoteType = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "happy"))
			  pChr->m_EmoteType = EMOTE_HAPPY;
			else if (!str_comp(pResult->GetString(0), "pain"))
			  pChr->m_EmoteType = EMOTE_PAIN;
			else if (!str_comp(pResult->GetString(0), "surprise"))
			  pChr->m_EmoteType = EMOTE_SURPRISE;
			else
			{
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Unkown emote... Say /emote");
			}
			
			int Duration = 1;
			if (pResult->NumArguments() > 1)
				Duration = pResult->GetInteger(1);
			  
			pChr->m_EmoteStop = pSelf->Server()->Tick() + Duration * pSelf->Server()->TickSpeed();
		}
	}
}

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	Console()->Register("change_map", "r", CFGFLAG_SERVER, ConChangeMap, this, "Changes the map to r", 3);
	Console()->Register("restart", "?i", CFGFLAG_SERVER, ConRestart, this, "Kills all players", 3);
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Changes the broadcast message for a moment", 3);
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Sends a server message to all players", 3);
	Console()->Register("set_team", "ii", CFGFLAG_SERVER, ConSetTeam, this, "Changes the team of player i1 to team i2", 2);
	Console()->Register("addvote", "r", CFGFLAG_SERVER, ConAddVote, this, "Adds a vote entry to the clients", 4);
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the vote list", 4);

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Modifies tune parameter s to value i", 4);
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Resets all tuning", 4);
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Shows all tuning options", 4);

	Console()->Register("kill_pl", "i", CFGFLAG_SERVER, ConKillPlayer, this, "Kills player i and announces the kill", 2);
	
	Console()->Register("logout", "?i", CFGFLAG_SERVER, ConLogOut, this, "If you are a helper or didn't specify [i] it logs you out, otherwise it logs player i out", 0);
	Console()->Register("helper", "i", CFGFLAG_SERVER, ConSetlvl1, this, "Authenticates player i to the Level of 1", 2);
	Console()->Register("moder", "i", CFGFLAG_SERVER, ConSetlvl2, this, "Authenticates player i to the Level of 2", 3);
	Console()->Register("admin", "i", CFGFLAG_SERVER, ConSetlvl3, this, "Authenticates player i to the Level of 3 (CAUTION: Irreversible, once he is an admin you can't control him)", 3);
	
	Console()->Register("mute", "ii", CFGFLAG_SERVER, ConMute, this, "Mutes player i1 for i2 seconds", 2);
	
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Forces the current vote to result in r (Yes/No)", 3);
	
	Console()->Register("invis_me", "", CFGFLAG_SERVER, ConInvisMe, this, "Makes you invisible", 1);
	Console()->Register("vis_me", "", CFGFLAG_SERVER, ConVisMe, this, "Makes you visible again", 1);
	Console()->Register("invis", "i", CFGFLAG_SERVER, ConInvis, this, "Makes player i invisible", 2);
	Console()->Register("vis", "i", CFGFLAG_SERVER, ConVis, this, "Makes player i visible again", 2);
	
	Console()->Register("timerstop", "i", CFGFLAG_SERVER, ConTimerStop, this, "Stops the timer of player i", 2);
	Console()->Register("timerstart", "i", CFGFLAG_SERVER, ConTimerStart, this, "Starts the timer of player i", 2);
	Console()->Register("timerrestart", "i", CFGFLAG_SERVER, ConTimerReStart, this, "Sets the timer of player i to 0 and starts it", 2);
	Console()->Register("timerzero", "i", CFGFLAG_SERVER, ConTimerZero, this, "Sets the timer of player i to 0 and stops it", 2);

	Console()->Register("tele", "ii", CFGFLAG_SERVER, ConTeleport, this, "Teleports player i1 to player i2", 2);
	
	Console()->Register("freeze", "i?i", CFGFLAG_SERVER, ConFreeze, this, "Freezes player i1 for i2 seconds (infinity by default)", 2);
	Console()->Register("unfreeze", "i", CFGFLAG_SERVER, ConUnFreeze, this, "Unfreezes player i", 2);

	Console()->Register("shotgun", "i", CFGFLAG_SERVER, ConShotgun, this, "Gives a shotgun to player i", 2);
	Console()->Register("shotgun_me", "", CFGFLAG_SERVER, ConShotgunMe, this, "Gives shotgun to yourself", 1);
	Console()->Register("grenade", "i", CFGFLAG_SERVER, ConGrenade, this, "Gives a grenade launcher to player i", 2);
	Console()->Register("grenade_me", "", CFGFLAG_SERVER, ConGrenadeMe, this, "Gives grenade launcher to yourself", 1);
	Console()->Register("rifle", "i", CFGFLAG_SERVER, ConRifle, this, "Gives a rifle to player i", 2);
	Console()->Register("rifle_me", "", CFGFLAG_SERVER, ConRifleMe, this, "Gives rifle to yourself", 1);
	Console()->Register("weapons", "i", CFGFLAG_SERVER, ConWeapons, this, "Gives all weapons to player i", 2);
	Console()->Register("weapons_me", "", CFGFLAG_SERVER, ConWeaponsMe, this, "Gives all weapons to yourself", 1);

	Console()->Register("unshotgun", "i", CFGFLAG_SERVER, ConUnShotgun, this, "Takes a shotgun from player i", 2);
	Console()->Register("unshotgun_me", "", CFGFLAG_SERVER, ConUnShotgunMe, this, "Takes shotgun from yourself", 1);
	Console()->Register("ungrenade", "i", CFGFLAG_SERVER, ConUnGrenade, this, "Takes a grenade launcher from player i", 2);
	Console()->Register("ungrenade_me", "", CFGFLAG_SERVER, ConUnGrenadeMe, this, "Takes grenade launcher from yourself", 1);
	Console()->Register("unrifle", "i", CFGFLAG_SERVER, ConUnRifle, this, "Takes a rifle from player i", 2);
	Console()->Register("unrifle_me", "", CFGFLAG_SERVER, ConUnRifleMe, this, "Takes rifle from yourself", 1);
	Console()->Register("unweapons", "i", CFGFLAG_SERVER, ConUnWeapons, this, "Takes all weapons from player i", 2);
	Console()->Register("unweapons_me", "", CFGFLAG_SERVER, ConUnWeaponsMe, this, "Takes all weapons from yourself", 1);

	Console()->Register("ninja", "i", CFGFLAG_SERVER, ConNinja, this, "Makes player i a ninja", 2);
	Console()->Register("ninja_me", "", CFGFLAG_SERVER, ConNinjaMe, this, "Makes yourself a ninja", 1);

	Console()->Register("hammer_me", "i", CFGFLAG_SERVER, ConHammerMe, this, "Sets your hammer power to i", 1);
	Console()->Register("hammer", "ii", CFGFLAG_SERVER, ConHammer, this, "Sets the hammer power of player i1 to i2", 2);

	Console()->Register("super", "i", CFGFLAG_SERVER, ConSuper, this, "Makes player i super", 2);
	Console()->Register("unsuper", "i", CFGFLAG_SERVER, ConUnSuper, this, "Removes super from player i", 2);
	Console()->Register("super_me", "", CFGFLAG_SERVER, ConSuperMe, this, "Makes yourself super", 1);
	Console()->Register("unsuper_me", "", CFGFLAG_SERVER, ConUnSuperMe, this, "Removes super from yourself", 1);

	Console()->Register("left", "?i", CFGFLAG_SERVER, ConGoLeft, this, "Makes you or player i move 1 tile left", 1);
	Console()->Register("right", "?i", CFGFLAG_SERVER, ConGoRight, this, "Makes you or player i move 1 tile right", 1);
	Console()->Register("up", "?i", CFGFLAG_SERVER, ConGoUp, this, "Makes you or player i move 1 tile up", 1);
	Console()->Register("down", "?i", CFGFLAG_SERVER, ConGoDown, this, "Makes you or player i move 1 tile down", 1);

	Console()->Register("broadtime", "", CFGFLAG_SERVER, ConBroadTime, this, "Toggles Showing the time string in race", -1);
	Console()->Register("cmdlist", "", CFGFLAG_SERVER, ConCmdList, this, "Shows the list of all commands", -1);
	Console()->Register("credits", "", CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDRace mod", -1);
	Console()->Register("emote", "?s?i", CFGFLAG_SERVER, ConEyeEmote, this, "Sets your tee's eye emote", -1);
	Console()->Register("broadmsg", "", CFGFLAG_SERVER, ConToggleBroadcast, this, "Toggle Showing the Server's Broadcast message during race", -1);
	Console()->Register("eyeemote", "", CFGFLAG_SERVER, ConEyeEmote, this, "Toggles whether you automatically use eyeemotes with standard emotes", -1);
	Console()->Register("flags", "", CFGFLAG_SERVER, ConFlags, this, "Shows gameplay information for this server", -1);
	Console()->Register("fly", "", CFGFLAG_SERVER, ConToggleFly, this, "Toggles whether you fly by pressing jump", 1);
	Console()->Register("help", "?r", CFGFLAG_SERVER, ConHelp, this, "Helps you with commands", -1);
	Console()->Register("info", "", CFGFLAG_SERVER, ConInfo, this, "Shows info about this server", -1);
	Console()->Register("kill", "", CFGFLAG_SERVER, ConKill, this, "Kills you", -1);
	Console()->Register("me", "s", CFGFLAG_SERVER, ConMe, this, "Like the famous irc commands /me says hi, will display YOURNAME says hi", -1);
	Console()->Register("pause", "", CFGFLAG_SERVER, ConTogglePause, this, "If enabled on this server it pauses the game for you", -1);
	Console()->Register("rank", "?r", CFGFLAG_SERVER, ConRank, this, "Shows either your rank or the rank of the given player", -1);
	Console()->Register("rules", "", CFGFLAG_SERVER, ConRules, this, "Shows the rules of this server", -1);
	Console()->Register("team", "?i", CFGFLAG_SERVER, ConJoinTeam, this, "Lets you join the specified team", -1);
	Console()->Register("top5", "?i", CFGFLAG_SERVER, ConTop5, this, "Shows the top 5 from the 1st, or starting at the specified number", -1);

	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);

}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);
	//if(!data) // only load once
		//data = load_data_from_memory(internal_data);

	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	m_Layers.Init(Kernel());
	m_Collision.Init(&m_Layers);

	// reset everything here
	//world = new GAMEWORLD;
	//players = new CPlayer[MAX_CLIENTS];

	//TODO: No need any more?
	char buf[512];
	str_format(buf, sizeof(buf), "data/maps/%s.cfg", g_Config.m_SvMap);
	Console()->ExecuteFile(buf);
	str_format(buf, sizeof(buf), "data/maps/%s.map.cfg", g_Config.m_SvMap);
	Console()->ExecuteFile(buf);

	// select gametype
	m_pController = new CGameControllerDDRace(this);
	((CGameControllerDDRace*)m_pController)->m_Teams.Reset();

	Server()->SetBrowseInfo(m_pController->m_pGameType, -1);


	// delete old score object
	if(m_pScore)
		delete m_pScore;
		
	// create score object (add sql later)
	if(g_Config.m_SvUseSQL)
		m_pScore = new CSqlScore(this);
	else
		m_pScore = new CFileScore(this);
	// setup core world
	//for(int i = 0; i < MAX_CLIENTS; i++)
	//	game.players[i].core.world = &game.world.core;
	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Data);
	CTile *pFront=0;
	if(m_Layers.FrontLayer())
		pFront = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Front);
	

	for(int y = 0; y < pTileMap->m_Height; y++)
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width+x].m_Index;
			if(Index == TILE_NPC)
				m_Tuning.Set("player_collision",0);
			else if(Index == TILE_EHOOK)
				g_Config.m_SvEndlessDrag = 1;
			else if(Index == TILE_NOHIT)
				g_Config.m_SvHit = 0;
			else if(Index == TILE_NPH)
				m_Tuning.Set("player_hooking",0);
			if(Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
				m_pController->OnEntity(Index-ENTITY_OFFSET, Pos, false, pTiles[y*pTileMap->m_Width+x].m_Flags);
			}
			if(pFront)
			{
				int Index = pFront[y*pTileMap->m_Width+x].m_Index;
				if(Index == TILE_NPC)
					m_Tuning.Set("player_collision",0);
				else if(Index == TILE_EHOOK)
					g_Config.m_SvEndlessDrag = 1;
				else if(Index == TILE_NOHIT)
					g_Config.m_SvHit = 0;
				else if(Index == TILE_NPH)
					m_Tuning.Set("player_hooking",0);
				if(Index >= ENTITY_OFFSET)
				{
					vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
					m_pController->OnEntity(Index-ENTITY_OFFSET, Pos, true, pFront[y*pTileMap->m_Width+x].m_Flags);
				}
			}
		}
	}

#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		for(int i = 0; i < g_Config.m_DbgDummies ; i++)
		{
			OnClientConnected(MAX_CLIENTS-i-1);
		}
	}
#endif
}

void CGameContext::OnShutdown()
{
	Layers()->Dest();
	Collision()->Dest();
	delete m_pController;
	m_pController = 0;
	Clear();
}

void CGameContext::OnSnap(int ClientId)
{
	m_World.Snap(ClientId);
	m_pController->Snap(ClientId);
	m_Events.Snap(ClientId);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
			m_apPlayers[i]->Snap(ClientId);
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }

