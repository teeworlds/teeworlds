#include <new>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "gamemodes/dm.h"
#include "gamemodes/tdm.h"
#include "gamemodes/ctf.h"
#include "gamemodes/mod.h"

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
		m_apPlayers[i] = 0;
	
	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;

	if(Resetting==NO_RESET)
		m_pVoteOptionHeap = new CHeap();
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

	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET);

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

void CGameContext::CreateDamageInd(vec2 p, float Angle, int Amount)
{
	float a = 3 * 3.14159f / 2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(NETEVENT_DAMAGEIND));
		if(ev)
		{
			ev->m_X = (int)p.x;
			ev->m_Y = (int)p.y;
			ev->m_Angle = (int)(f*256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 p)
{
	// create the event
	NETEVENT_HAMMERHIT *ev = (NETEVENT_HAMMERHIT *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(NETEVENT_HAMMERHIT));
	if(ev)
	{
		ev->m_X = (int)p.x;
		ev->m_Y = (int)p.y;
	}
}


void CGameContext::CreateExplosion(vec2 p, int Owner, int Weapon, bool NoDamage)
{
	// create the event
	NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(NETEVENT_EXPLOSION));
	if(ev)
	{
		ev->m_X = (int)p.x;
		ev->m_Y = (int)p.y;
	}

	if (!NoDamage)
	{
		// deal damage
		CCharacter *apEnts[64];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = m_World.FindEntities(p, Radius, (CEntity**)apEnts, 64, NETOBJTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->m_Pos - p;
			vec2 ForceDir(0,1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if((int)Dmg)
				apEnts[i]->TakeDamage(ForceDir*Dmg*2, (int)Dmg, Owner, Weapon);
		}
	}
}

/*
void create_smoke(vec2 p)
{
	// create the event
	EV_EXPLOSION *ev = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 p)
{
	// create the event
	NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(NETEVENT_SPAWN));
	if(ev)
	{
		ev->m_X = (int)p.x;
		ev->m_Y = (int)p.y;
	}
}

void CGameContext::CreateDeath(vec2 p, int ClientId)
{
	// create the event
	NETEVENT_DEATH *ev = (NETEVENT_DEATH *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(NETEVENT_DEATH));
	if(ev)
	{
		ev->m_X = (int)p.x;
		ev->m_Y = (int)p.y;
		ev->m_ClientId = ClientId;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, int Mask)
{
	if (Sound < 0)
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
	if (Sound < 0)
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


void CGameContext::SendChat(int ChatterClientId, int Team, const char *pText)
{
	if(ChatterClientId >= 0 && ChatterClientId < MAX_CLIENTS)
		dbg_msg("chat", "%d:%d:%s: %s", ChatterClientId, Team, Server()->ClientName(ChatterClientId), pText);
	else
		dbg_msg("chat", "*** %s", pText);

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
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_Cid = ChatterClientId;
		Msg.m_pMessage = pText;
		
		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NOSEND, -1);

		// send to the clients
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
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
		CTuningParams p;
		if(mem_comp(&p, &m_Tuning, sizeof(p)) != 0)
		{
			dbg_msg("server", "resetting tuning due to pure server");
			m_Tuning = p;
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

				if(Yes >= Total/2+1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if(No >= Total/2+1 || Yes+No == Total)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}
			
			if(m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				Console()->ExecuteLine(m_aVoteCommand);
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote passed");
			
				if(m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_Last_VoteCall = 0;
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
	dbg_msg("game", "join player='%d:%s'", ClientId, Server()->ClientName(ClientId));


	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%s entered and joined the %s", Server()->ClientName(ClientId), m_pController->GetTeamName(m_apPlayers[ClientId]->GetTeam()));
	SendChat(-1, CGameContext::CHAT_ALL, aBuf); 

	dbg_msg("game", "team_join player='%d:%s' team=%d", ClientId, Server()->ClientName(ClientId), m_apPlayers[ClientId]->GetTeam());

	m_VoteUpdate = true;
}

void CGameContext::OnClientConnected(int ClientId)
{
	// Check which team the player should be on
	const int StartTeam = g_Config.m_SvTournamentMode ? -1 : m_pController->GetAutoTeam(ClientId);

	m_apPlayers[ClientId] = new(ClientId) CPlayer(this, ClientId, StartTeam);
	//players[client_id].init(client_id);
	//players[client_id].client_id = client_id;
	
	(void)m_pController->CheckTeamBalance();

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
	
	(void)m_pController->CheckTeamBalance();
	m_VoteUpdate = true;
}

void CGameContext::OnMessage(int MsgId, CUnpacker *pUnpacker, int ClientId)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
	CPlayer *p = m_apPlayers[ClientId];
	
	if(!pRawMsg)
	{
		dbg_msg("server", "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
		return;
	}
	
	if(MsgId == NETMSGTYPE_CL_SAY)
	{
		CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
		int Team = pMsg->m_Team;
		if(Team)
			Team = p->GetTeam();
		else
			Team = CGameContext::CHAT_ALL;
		
		if(g_Config.m_SvSpamprotection && p->m_Last_Chat && p->m_Last_Chat+Server()->TickSpeed() > Server()->Tick())
			return;
		
		p->m_Last_Chat = Server()->Tick();

		// check for invalid chars
		unsigned char *pMessage = (unsigned char *)pMsg->m_pMessage;
		while (*pMessage)
		{
			if(*pMessage < 32)
				*pMessage = ' ';
			pMessage++;
		}
		
		SendChat(ClientId, Team, pMsg->m_pMessage);
	}
	else if(MsgId == NETMSGTYPE_CL_CALLVOTE)
	{
		if(g_Config.m_SvSpamprotection && p->m_Last_VoteTry && p->m_Last_VoteTry+Server()->TickSpeed()*3 > Server()->Tick())
			return;

		int64 Now = Server()->Tick();
		p->m_Last_VoteTry = Now;
		if(m_VoteCloseTime)
		{
			SendChatTarget(ClientId, "Wait for current vote to end before calling a new one.");
			return;
		}
		
		int Timeleft = p->m_Last_VoteCall + Server()->TickSpeed()*60 - Now;
		if(p->m_Last_VoteCall && Timeleft > 0)
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
			while(pOption)
			{
				if(str_comp_nocase(pMsg->m_Value, pOption->m_aCommand) == 0)
				{
					str_format(aChatmsg, sizeof(aChatmsg), "%s called vote to change server option '%s'", Server()->ClientName(ClientId), pOption->m_aCommand);
					str_format(aDesc, sizeof(aDesc), "%s", pOption->m_aCommand);
					str_format(aCmd, sizeof(aCmd), "%s", pOption->m_aCommand);
					break;
				}

				pOption = pOption->m_pNext;
			}
			
			if(!pOption)
			{
				str_format(aChatmsg, sizeof(aChatmsg), "'%s' isn't an option on this server", pMsg->m_Value);
				SendChatTarget(ClientId, aChatmsg);
				return;
			}
		}
		else if(str_comp_nocase(pMsg->m_Type, "kick") == 0)
		{
			if(!g_Config.m_SvVoteKick)
			{
				SendChatTarget(ClientId, "Server does not allow voting to kick players");
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
				SendChatTarget(ClientId, "You cant kick yourself");
				return;
			}
			if(Server()->IsAuthed(KickId))
			{
				SendChatTarget(ClientId, "You cant kick admins");
				char aBufKick[128];
				str_format(aBufKick, sizeof(aBufKick), "%s called for vote to kick you", Server()->ClientName(ClientId));
				SendChatTarget(KickId, aBufKick);
				return;
			}
			
			str_format(aChatmsg, sizeof(aChatmsg), "%s called for vote to kick '%s'", Server()->ClientName(ClientId), Server()->ClientName(KickId));
			str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickId));
			if (!g_Config.m_SvVoteKickBantime)
				str_format(aCmd, sizeof(aCmd), "kick %d", KickId);
			else
			{
				char aBuf[64] = {0};
				Server()->GetClientIP(KickId, aBuf, sizeof(aBuf));
				str_format(aCmd, sizeof(aCmd), "ban %s %d", aBuf, g_Config.m_SvVoteKickBantime);
			}
		}
		
		if(aCmd[0])
		{
			SendChat(-1, CGameContext::CHAT_ALL, aChatmsg);
			StartVote(aDesc, aCmd);
			p->m_Vote = 1;
			p->m_VotePos = m_VotePos = 1;
			m_VoteCreator = ClientId;
			p->m_Last_VoteCall = Now;
		}
	}
	else if(MsgId == NETMSGTYPE_CL_VOTE)
	{
		if(!m_VoteCloseTime)
			return;

		if(p->m_Vote == 0)
		{
			CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
			if(!pMsg->m_Vote)
				return;

			p->m_Vote = pMsg->m_Vote;
			p->m_VotePos = ++m_VotePos;
			m_VoteUpdate = true;
		}
	}
	else if (MsgId == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
	{
		CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;
		
		if(p->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && p->m_Last_SetTeam && p->m_Last_SetTeam+Server()->TickSpeed()*3 > Server()->Tick()))
			return;

		// Switch team on given client and kill/respawn him
		if(m_pController->CanJoinTeam(pMsg->m_Team, ClientId))
		{
			if(m_pController->CanChangeTeam(p, pMsg->m_Team))
			{
				p->m_Last_SetTeam = Server()->Tick();
				if(p->GetTeam() == -1 || pMsg->m_Team == -1)
					m_VoteUpdate = true;
				p->SetTeam(pMsg->m_Team);
				(void)m_pController->CheckTeamBalance();
			}
			else
				SendBroadcast("Teams must be balanced, please join other team", ClientId);
		}
		else
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Only %d active players are allowed", g_Config.m_SvMaxClients-g_Config.m_SvSpectatorSlots);
			SendBroadcast(aBuf, ClientId);
		}
	}
	else if (MsgId == NETMSGTYPE_CL_CHANGEINFO || MsgId == NETMSGTYPE_CL_STARTINFO)
	{
		CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
		
		if(g_Config.m_SvSpamprotection && p->m_Last_ChangeInfo && p->m_Last_ChangeInfo+Server()->TickSpeed()*5 > Server()->Tick())
			return;
			
		p->m_Last_ChangeInfo = Server()->Tick();
		
		p->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
		p->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
		p->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;

		// check for invalid chars
		unsigned char *pName = (unsigned char *)pMsg->m_pName;
		while (*pName)
		{
			if(*pName < 32)
				*pName = ' ';
			pName++;
		}

		// copy old name
		char aOldName[MAX_NAME_LENGTH];
		str_copy(aOldName, Server()->ClientName(ClientId), MAX_NAME_LENGTH);
		
		Server()->SetClientName(ClientId, pMsg->m_pName);
		if(MsgId == NETMSGTYPE_CL_CHANGEINFO && str_comp(aOldName, Server()->ClientName(ClientId)) != 0)
		{
			char aChatText[256];
			str_format(aChatText, sizeof(aChatText), "%s changed name to %s", aOldName, Server()->ClientName(ClientId));
			SendChat(-1, CGameContext::CHAT_ALL, aChatText);
		}
		
		// set skin
		str_copy(p->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(p->m_TeeInfos.m_SkinName));
		
		m_pController->OnPlayerInfoChange(p);
		
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
	else if (MsgId == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
	{
		CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;
		
		if(g_Config.m_SvSpamprotection && p->m_Last_Emote && p->m_Last_Emote+Server()->TickSpeed()*3 > Server()->Tick())
			return;
			
		p->m_Last_Emote = Server()->Tick();
		
		SendEmoticon(ClientId, pMsg->m_Emoticon);
	}
	else if (MsgId == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
	{
		if(p->m_Last_Kill && p->m_Last_Kill+Server()->TickSpeed()*3 > Server()->Tick())
			return;
		
		p->m_Last_Kill = Server()->Tick();
		p->KillCharacter(WEAPON_SELF);
		p->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3;
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if(pSelf->Tuning()->Set(pParamName, NewValue))
	{
		dbg_msg("tuning", "%s changed to %.2f", pParamName, NewValue);
		pSelf->SendTuningParams(-1);
	}
	else
		dbg_msg("tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams p;
	*pSelf->Tuning() = p;
	pSelf->SendTuningParams(-1);
	dbg_msg("tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	for(int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		dbg_msg("tuning", "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
	}
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->GetString(0));
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientId = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	
	dbg_msg("", "%d %d", ClientId, Team);
	
	if(!pSelf->m_apPlayers[ClientId])
		return;
	
	pSelf->m_apPlayers[ClientId]->SetTeam(Team);
	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
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
	dbg_msg("server", "added option '%s'", pOption->m_aCommand);

	CNetMsg_Sv_VoteOption OptionMsg;
	OptionMsg.m_pCommand = pOption->m_aCommand;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
	dbg_msg("server", "forcing vote %s", pResult->GetString(0));
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
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

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "");

	Console()->Register("change_map", "r", CFGFLAG_SERVER, ConChangeMap, this, "");
	Console()->Register("restart", "?i", CFGFLAG_SERVER, ConRestart, this, "");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "");
	Console()->Register("set_team", "ii", CFGFLAG_SERVER, ConSetTeam, this, "");

	Console()->Register("addvote", "r", CFGFLAG_SERVER, ConAddVote, this, "");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "");

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

	// select gametype
	if(str_comp(g_Config.m_SvGametype, "mod") == 0)
		m_pController = new CGameControllerMOD(this);
	else if(str_comp(g_Config.m_SvGametype, "ctf") == 0)
		m_pController = new CGameControllerCTF(this);
	else if(str_comp(g_Config.m_SvGametype, "tdm") == 0)
		m_pController = new CGameControllerTDM(this);
	else
		m_pController = new CGameControllerDM(this);

	Server()->SetBrowseInfo(m_pController->m_pGameType, -1);

	// setup core world
	//for(int i = 0; i < MAX_CLIENTS; i++)
	//	game.players[i].core.world = &game.world.core;

	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Data);
	
	
	
	
	/*
	num_spawn_points[0] = 0;
	num_spawn_points[1] = 0;
	num_spawn_points[2] = 0;
	*/
	
	for(int y = 0; y < pTileMap->m_Height; y++)
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width+x].m_Index;
			
			if(Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
				m_pController->OnEntity(Index-ENTITY_OFFSET, Pos);
			}
		}
	}

	//game.world.insert_entity(game.Controller);

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
