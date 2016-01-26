#include <modapi/compatibility.h>
#include <modapi/server/event.h>
#include <modapi/graphics.h>

#include <engine/server.h>
#include <generated/server_data.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <mod/entities/character.h>

CModAPI_WorldEvent::CModAPI_WorldEvent(CGameContext* pGameServer, int WorldID) :
	m_pGameServer(pGameServer), m_WorldID(WorldID)
{
	
}

IServer* CModAPI_WorldEvent::Server()
{
	return m_pGameServer->Server();
}

CGameContext* CModAPI_WorldEvent::GameServer()
{
	return m_pGameServer;
}

int CModAPI_WorldEvent::WorldID() const
{
	return m_WorldID;
}

int CModAPI_WorldEvent::GenerateMask()
{
	int Mask = 0;
	if(WorldID() == MOD_WORLD_ALL)
	{
		Mask = -1;
	}
	else
	{
		for(int i=0; i<MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetWorldID() == WorldID())
			{
				Mask |= (1 << i);
			}
		}
	}
	
	return Mask;
}

/* ANIMATED TEXT EVENT ************************************************/

CModAPI_WorldEvent_AnimatedText::CModAPI_WorldEvent_AnimatedText(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_AnimatedText::Send(vec2 Pos, int ItemLayer, const char* pText, int Size, vec4 Color, int Alignment, int AnimationID, int Duration, vec2 Offset)
{
	CNetEvent_ModAPI_AnimatedText *pEvent = (CNetEvent_ModAPI_AnimatedText *)GameServer()->m_Events.Create(NETEVENTTYPE_MODAPI_ANIMATEDTEXT, sizeof(CNetEvent_ModAPI_AnimatedText), GenerateMask());
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

CModAPI_WorldEvent_Sound::CModAPI_WorldEvent_Sound(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_Sound::Send(vec2 Pos, int Sound)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)GameServer()->m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), GenerateMask());
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CModAPI_WorldEvent_Sound::Send(vec2 Pos, int Sound, int Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)GameServer()->m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), Mask & GenerateMask());
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

/* SPAWN EFFECT *******************************************************/

CModAPI_WorldEvent_SpawnEffect::CModAPI_WorldEvent_SpawnEffect(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_SpawnEffect::Send(vec2 Pos)
{
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)GameServer()->m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn), GenerateMask());
	if(ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

/* HAMMERHIT EFFECT ***************************************************/

CModAPI_WorldEvent_HammerHitEffect::CModAPI_WorldEvent_HammerHitEffect(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_HammerHitEffect::Send(vec2 Pos)
{
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)GameServer()->m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit), GenerateMask());
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

/* DAMAGE INDICATOR EFFECT ********************************************/

CModAPI_WorldEvent_DamageIndicator::CModAPI_WorldEvent_DamageIndicator(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_DamageIndicator::Send(vec2 Pos, float Angle, int Amount)
{
	float a = 3*pi/2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)GameServer()->m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), GenerateMask());
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f*256.0f);
		}
	}
}

/* DEATH EFFECT *******************************************************/

CModAPI_WorldEvent_DeathEffect::CModAPI_WorldEvent_DeathEffect(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_DeathEffect::Send(vec2 Pos, int ClientID)
{
	CNetEvent_Death *pEvent = (CNetEvent_Death *)GameServer()->m_Events.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death), GenerateMask());
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

/* EXPLOSION EFFECT ***************************************************/

CModAPI_WorldEvent_ExplosionEffect::CModAPI_WorldEvent_ExplosionEffect(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_ExplosionEffect::Send(vec2 Pos)
{
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)GameServer()->m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion), GenerateMask());
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

/* EXPLOSION **********************************************************/

CModAPI_WorldEvent_Explosion::CModAPI_WorldEvent_Explosion(CGameContext* pGameServer, int WorldID) :
	CModAPI_WorldEvent_ExplosionEffect(pGameServer, WorldID)
{
	
}

void CModAPI_WorldEvent_Explosion::Send(vec2 Pos, int Owner, int Weapon, int MaxDamage)
{
	CModAPI_WorldEvent_ExplosionEffect::Send(Pos);
	
	// deal damage
	CCharacter *apEnts[MAX_CLIENTS];
	float Radius = g_pData->m_Explosion.m_Radius;
	float InnerRadius = 48.0f;
	float MaxForce = g_pData->m_Explosion.m_MaxForce;
	
	if(WorldID() == MOD_WORLD_ALL)
	{
		for(int w=0; w<MOD_NUM_WORLDS; w++)
		{
			int Num = GameServer()->m_World[w].FindEntities(Pos, Radius, (CEntity**)apEnts, MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				vec2 Diff = apEnts[i]->GetPos() - Pos;
				vec2 Force(0, MaxForce);
				float l = length(Diff);
				if(l)
					Force = normalize(Diff) * MaxForce;
				float Factor = 1 - clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
				apEnts[i]->TakeDamage(Force * Factor, (int)(Factor * MaxDamage), Owner, Weapon);
			}
		}
	}
	else
	{
		int Num = GameServer()->m_World[WorldID()].FindEntities(Pos, Radius, (CEntity**)apEnts, MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->GetPos() - Pos;
			vec2 Force(0, MaxForce);
			float l = length(Diff);
			if(l)
				Force = normalize(Diff) * MaxForce;
			float Factor = 1 - clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			apEnts[i]->TakeDamage(Force * Factor, (int)(Factor * MaxDamage), Owner, Weapon);
		}
	}
}
