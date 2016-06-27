#include <modapi/compatibility.h>
#include <modapi/server/event.h>
#include <modapi/graphics.h>

#include <engine/server.h>
#include <generated/server_data.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <mod/entities/character.h>

#define MODAPI_EVENT_CORE(TypeName) \
TypeName& TypeName::Client(int ClientID) \
{ \
	m_Mask = 0; \
	if(ClientID == -1) \
		m_Mask = -1; \
	else \
	{ \
		m_Mask |= (1 << ClientID); \
	} \
	return *this; \
} \
 \
TypeName& TypeName::Mask(int Mask) \
{ \
	m_Mask = Mask; \
	return *this; \
} \
 \
TypeName& TypeName::World(int WorldID) \
{ \
	m_Mask = 0; \
	if(WorldID == MOD_WORLD_ALL) \
		m_Mask = -1; \
	else \
	{ \
		for(int i=0; i<MAX_CLIENTS; i++) \
		{ \
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetWorldID() == WorldID) \
			{ \
				m_Mask |= (1 << i); \
			} \
		} \
	} \
	return *this; \
}
	
CModAPI_Event::CModAPI_Event(CGameContext* pGameServer) :
	m_pGameServer(pGameServer), m_Mask(-1)
{
	
}

IServer* CModAPI_Event::Server()
{
	return m_pGameServer->Server();
}
	
int CModAPI_Event::GetMask06()
{
	int Mask = 0;
	
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if(m_Mask & (1 << i) == 1 && Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW06)
			Mask |= (1 << i);
	}
	
	return Mask;
}
	
int CModAPI_Event::GetMask07()
{
	int Mask = 0;
	
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if(m_Mask & (1 << i) == 1 && Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW07)
			Mask |= (1 << i);
	}
	
	return Mask;
}
	
int CModAPI_Event::GetMask07ModAPI()
{
	int Mask = 0;
	
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if(m_Mask & (1 << i) == 1 && Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW07MODAPI)
			Mask |= (1 << i);
	}
	
	return Mask;
}

/* WEAPONPICKUP *******************************************************/

CModAPI_Event_WeaponPickup::CModAPI_Event_WeaponPickup(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_WeaponPickup)

void CModAPI_Event_WeaponPickup::Send(int WeaponID)
{
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if(m_Mask & (1 << i) == 0 || GameServer()->m_apPlayers[i])
			continue;
		
		if(GameServer()->m_apPlayers[i])
		{
			if(Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW06)
			{
				CTW06_NetMsg_Sv_WeaponPickup Msg;
				Msg.m_Weapon = WeaponID;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
			else
			{
				CNetMsg_Sv_WeaponPickup Msg;
				Msg.m_Weapon = WeaponID;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
		}
	}
}

/* BROADCAST **********************************************************/

CModAPI_Event_Broadcast::CModAPI_Event_Broadcast(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_Broadcast)

void CModAPI_Event_Broadcast::Send(const char* pText, int Alternative)
{
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if(m_Mask & (1 << i) == 0 || GameServer()->m_apPlayers[i])
			continue;
		
		if(GameServer()->m_apPlayers[i])
		{
			if(Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW07MODAPI)
			{
				CNetMsg_ModAPI_Sv_Broadcast Msg;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
			else if(Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW06)
			{
				CTW06_NetMsg_Sv_Broadcast Msg;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
			else if(Alternative == ALT_CHAT)
			{
				CNetMsg_Sv_Chat Msg;
				Msg.m_Team = 0;
				Msg.m_ClientID = -1;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
			else if(Alternative == ALT_MOTD)
			{
				CNetMsg_Sv_Motd Msg;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
		}
	}
}

/* CHAT ***************************************************************/

CModAPI_Event_Chat::CModAPI_Event_Chat(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_Chat)

void CModAPI_Event_Chat::Send(int From, int Team, const char* pText)
{
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if((m_Mask & (1 << i)) == 0 || !GameServer()->m_apPlayers[i])
			continue;
		
		if(GameServer()->m_apPlayers[i])
		{
			if(Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW06)
			{
				CTW06_NetMsg_Sv_Chat Msg;
				Msg.m_Team = Team;
				Msg.m_ClientID = From;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
			}
			else
			{
				CNetMsg_Sv_Chat Msg;
				Msg.m_Team = Team;
				Msg.m_ClientID = From;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
			}
		}
	}
}

/* MOTD ***************************************************************/

CModAPI_Event_MOTD::CModAPI_Event_MOTD(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_MOTD)

void CModAPI_Event_MOTD::Send(const char* pText)
{
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		if((m_Mask & (1 << i)) == 0 || !GameServer()->m_apPlayers[i])
			continue;
		
		if(GameServer()->m_apPlayers[i])
		{
			if(Server()->GetClientProtocol(i) == MODAPI_CLIENTPROTOCOL_TW06)
			{
				CTW06_NetMsg_Sv_Motd Msg;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
			}
			else
			{
				CTW06_NetMsg_Sv_Motd Msg;
				Msg.m_pMessage = pText;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
			}
		}
	}
}

/* ANIMATED TEXT EVENT ************************************************/

CModAPI_Event_AnimatedText::CModAPI_Event_AnimatedText(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_AnimatedText)

void CModAPI_Event_AnimatedText::Send(vec2 Pos, int ItemLayer, const char* pText, int Size, vec4 Color, int Alignment, int AnimationID, int Duration, vec2 Offset)
{
	CNetEvent_ModAPI_AnimatedText *pEvent = (CNetEvent_ModAPI_AnimatedText *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_MODAPI_ANIMATEDTEXT, sizeof(CNetEvent_ModAPI_AnimatedText), GetMask07ModAPI());
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ItemLayer = ItemLayer;
		pEvent->m_Alignment = Alignment;
		pEvent->m_Color = ModAPI_ColorToInt(Color);
		pEvent->m_Size = Size;
		StrToInts(pEvent->m_aText, 16, pText);
		pEvent->m_AnimationId = AnimationID;
		pEvent->m_Duration = Duration;
		pEvent->m_OffsetX = (int)Offset.x;
		pEvent->m_OffsetY = (int)Offset.y;
	}
}

/* SOUND **************************************************************/

CModAPI_Event_Sound::CModAPI_Event_Sound(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_Sound)

void CModAPI_Event_Sound::Send(vec2 Pos, int Sound)
{
	if (Sound < 0)
		return;

	//TW07ModAPI
	{
		CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), GetMask07ModAPI());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_SoundID = Sound;
		}
	}
	//TW07
	{
		CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)GameServer()->m_Events07.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), GetMask07());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_SoundID = Sound;
		}
	}
	//TW06
	{
		CTW06_NetEvent_SoundWorld *pEvent = (CTW06_NetEvent_SoundWorld *)GameServer()->m_Events06.Create(TW06_NETEVENTTYPE_SOUNDWORLD, sizeof(CTW06_NetEvent_SoundWorld), GetMask06());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_SoundID = Sound;
		}
	}
}

/* SPAWN EFFECT *******************************************************/

CModAPI_Event_SpawnEffect::CModAPI_Event_SpawnEffect(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_SpawnEffect)

void CModAPI_Event_SpawnEffect::Send(vec2 Pos)
{
	//TW07ModAPI
	{
		CNetEvent_Spawn *ev = (CNetEvent_Spawn *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn), GetMask07ModAPI());
		
		if(ev)
		{
			ev->m_X = (int)Pos.x;
			ev->m_Y = (int)Pos.y;
		}
	}
	//TW07
	{
		CNetEvent_Spawn *ev = (CNetEvent_Spawn *)GameServer()->m_Events07.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn), GetMask07());
		if(ev)
		{
			ev->m_X = (int)Pos.x;
			ev->m_Y = (int)Pos.y;
		}
	}
	//TW06
	{
		CTW06_NetEvent_Spawn *ev = (CTW06_NetEvent_Spawn *)GameServer()->m_Events06.Create(TW06_NETEVENTTYPE_SPAWN, sizeof(CTW06_NetEvent_Spawn), GetMask06());
		if(ev)
		{
			ev->m_X = (int)Pos.x;
			ev->m_Y = (int)Pos.y;
		}
	}
}

/* HAMMERHIT EFFECT ***************************************************/

CModAPI_Event_HammerHitEffect::CModAPI_Event_HammerHitEffect(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_HammerHitEffect)

void CModAPI_Event_HammerHitEffect::Send(vec2 Pos)
{
	//TW07ModAPI
	{
		CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit), GetMask07ModAPI());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
	//TW07
	{
		CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)GameServer()->m_Events07.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit), GetMask07());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
	//TW06
	{
		CTW06_NetEvent_HammerHit *pEvent = (CTW06_NetEvent_HammerHit *)GameServer()->m_Events06.Create(TW06_NETEVENTTYPE_HAMMERHIT, sizeof(CTW06_NetEvent_HammerHit), GetMask06());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
}

/* DAMAGE INDICATOR EFFECT ********************************************/

CModAPI_Event_DamageIndicator::CModAPI_Event_DamageIndicator(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_DamageIndicator)

void CModAPI_Event_DamageIndicator::Send(vec2 Pos, float Angle, int Amount)
{
	float a = 3*pi/2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		
		//TW07ModAPI
		{
			CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), GetMask07ModAPI());
			if(pEvent)
			{
				pEvent->m_X = (int)Pos.x;
				pEvent->m_Y = (int)Pos.y;
				pEvent->m_Angle = (int)(f*256.0f);
			}
		}
		//TW07
		{
			CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)GameServer()->m_Events07.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), GetMask07());
			if(pEvent)
			{
				pEvent->m_X = (int)Pos.x;
				pEvent->m_Y = (int)Pos.y;
				pEvent->m_Angle = (int)(f*256.0f);
			}
		}
		//TW06
		{
			CTW06_NetEvent_DamageInd *pEvent = (CTW06_NetEvent_DamageInd *)GameServer()->m_Events06.Create(TW06_NETEVENTTYPE_DAMAGEIND, sizeof(CTW06_NetEvent_DamageInd), GetMask06());
			if(pEvent)
			{
				pEvent->m_X = (int)Pos.x;
				pEvent->m_Y = (int)Pos.y;
				pEvent->m_Angle = (int)(f*256.0f);
			}
		}
	}
}

/* DEATH EFFECT *******************************************************/

CModAPI_Event_DeathEffect::CModAPI_Event_DeathEffect(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_DeathEffect)

void CModAPI_Event_DeathEffect::Send(vec2 Pos, int ClientID)
{
	//TW07ModAPI
	{
		CNetEvent_Death *pEvent = (CNetEvent_Death *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death), GetMask07ModAPI());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_ClientID = ClientID;
		}
	}
	//TW07
	{
		CNetEvent_Death *pEvent = (CNetEvent_Death *)GameServer()->m_Events07.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death), GetMask07());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_ClientID = ClientID;
		}
	}
	//TW06
	{
		CTW06_NetEvent_Death *pEvent = (CTW06_NetEvent_Death *)GameServer()->m_Events06.Create(TW06_NETEVENTTYPE_DEATH, sizeof(CTW06_NetEvent_Death), GetMask06());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_ClientID = ClientID;
		}
	}
}

/* EXPLOSION EFFECT ***************************************************/

CModAPI_Event_ExplosionEffect::CModAPI_Event_ExplosionEffect(CGameContext* pGameServer) :
	CModAPI_Event(pGameServer)
{
	
}

MODAPI_EVENT_CORE(CModAPI_Event_ExplosionEffect)

void CModAPI_Event_ExplosionEffect::Send(vec2 Pos)
{
	//TW07ModAPI
	{
		CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)GameServer()->m_Events07ModAPI.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion), GetMask07ModAPI());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
	//TW07
	{
		CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)GameServer()->m_Events07.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion), GetMask07());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
	//TW06
	{
		CTW06_NetEvent_Explosion *pEvent = (CTW06_NetEvent_Explosion *)GameServer()->m_Events06.Create(TW06_NETEVENTTYPE_EXPLOSION, sizeof(CTW06_NetEvent_Explosion), GetMask06());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
}
