/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/engine.h>

#include <engine/shared/config.h>

#include <game/generated/client_data.h>

#include <game/client/components/particles.h>
#include <game/client/components/skins.h>
#include <game/client/components/flow.h>
#include <game/client/components/damageind.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>

#include "effects.h"

inline vec2 RandomDir() { return normalize(vec2(frandom()-0.5f, frandom()-0.5f)); }

CEffects::CEffects()
{
	m_Add50hz = false;
	m_Add100hz = false;
}

void CEffects::AirJump(vec2 Pos)
{
	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_AIRJUMP;
	p.m_Pos = Pos + vec2(-6.0f, 16.0f);
	p.m_Vel = vec2(0, -200);
	p.m_LifeSpan = 0.5f;
	p.m_StartSize = 48.0f;
	p.m_EndSize = 0;
	p.m_Rot = frandom()*pi*2;
	p.m_Rotspeed = pi*2;
	p.m_Gravity = 500;
	p.m_Friction = 0.7f;
	p.m_FlowAffected = 0.0f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);

	p.m_Pos = Pos + vec2(6.0f, 16.0f);
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);

	m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, Pos);
}

void CEffects::DamageIndicator(vec2 Pos, vec2 Dir)
{
	m_pClient->m_pDamageind->Create(Pos, Dir);
}

void CEffects::PowerupShine(vec2 Pos, vec2 size)
{
	if(!m_Add50hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SLICE;
	p.m_Pos = Pos + vec2((frandom()-0.5f)*size.x, (frandom()-0.5f)*size.y);
	p.m_Vel = vec2(0, 0);
	p.m_LifeSpan = 0.5f;
	p.m_StartSize = 16.0f;
	p.m_EndSize = 0;
	p.m_Rot = frandom()*pi*2;
	p.m_Rotspeed = pi*2;
	p.m_Gravity = 500;
	p.m_Friction = 0.9f;
	p.m_FlowAffected = 0.0f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
}

void CEffects::SmokeTrail(vec2 Pos, vec2 Vel)
{
	if(!m_Add50hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SMOKE;
	p.m_Pos = Pos;
	p.m_Vel = Vel + RandomDir()*50.0f;
	p.m_LifeSpan = 0.5f + frandom()*0.5f;
	p.m_StartSize = 12.0f + frandom()*8;
	p.m_EndSize = 0;
	p.m_Friction = 0.7f;
	p.m_Gravity = frandom()*-500.0f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_PROJECTILE_TRAIL, &p);
}


void CEffects::SkidTrail(vec2 Pos, vec2 Vel)
{
	if(!m_Add100hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SMOKE;
	p.m_Pos = Pos;
	p.m_Vel = Vel + RandomDir()*50.0f;
	p.m_LifeSpan = 0.5f + frandom()*0.5f;
	p.m_StartSize = 24.0f + frandom()*12;
	p.m_EndSize = 0;
	p.m_Friction = 0.7f;
	p.m_Gravity = frandom()*-500.0f;
	p.m_Color = vec4(0.75f,0.75f,0.75f,1.0f);
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
}

void CEffects::BulletTrail(vec2 Pos)
{
	if(!m_Add100hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_BALL;
	p.m_Pos = Pos;
	p.m_LifeSpan = 0.25f + frandom()*0.25f;
	p.m_StartSize = 8.0f;
	p.m_EndSize = 0;
	p.m_Friction = 0.7f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_PROJECTILE_TRAIL, &p);
}

void CEffects::PlayerSpawn(vec2 Pos)
{
	for(int i = 0; i < 32; i++)
	{
		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_SHELL;
		p.m_Pos = Pos;
		p.m_Vel = RandomDir() * (powf(frandom(), 3)*600.0f);
		p.m_LifeSpan = 0.3f + frandom()*0.3f;
		p.m_StartSize = 64.0f + frandom()*32;
		p.m_EndSize = 0;
		p.m_Rot = frandom()*pi*2;
		p.m_Rotspeed = frandom();
		p.m_Gravity = frandom()*-400.0f;
		p.m_Friction = 0.7f;
		p.m_Color = vec4(0xb5/255.0f, 0x50/255.0f, 0xcb/255.0f, 1.0f);
		m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);

	}
	m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_PLAYER_SPAWN, 1.0f, Pos);
}

void CEffects::PlayerDeath(vec2 Pos, int ClientID)
{
	vec3 BloodColor(1.0f,1.0f,1.0f);

	if(ClientID >= 0)
	{
		if(m_pClient->m_aClients[ClientID].m_aUseCustomColors[CSkins::SKINPART_BODY])
			BloodColor = m_pClient->m_pSkins->GetColorV3(m_pClient->m_aClients[ClientID].m_aSkinPartColors[CSkins::SKINPART_BODY]);
		else
		{
			const CSkins::CSkinPart *s = m_pClient->m_pSkins->GetSkinPart(CSkins::SKINPART_BODY, m_pClient->m_aClients[ClientID].m_SkinPartIDs[CSkins::SKINPART_BODY]);
			if(s)
				BloodColor = s->m_BloodColor;
		}
	}

	for(int i = 0; i < 64; i++)
	{
		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_SPLAT01 + (rand()%3);
		p.m_Pos = Pos;
		p.m_Vel = RandomDir() * ((frandom()+0.1f)*900.0f);
		p.m_LifeSpan = 0.3f + frandom()*0.3f;
		p.m_StartSize = 24.0f + frandom()*16;
		p.m_EndSize = 0;
		p.m_Rot = frandom()*pi*2;
		p.m_Rotspeed = (frandom()-0.5f) * pi;
		p.m_Gravity = 800.0f;
		p.m_Friction = 0.8f;
		vec3 c = BloodColor * (0.75f + frandom()*0.25f);
		p.m_Color = vec4(c.r, c.g, c.b, 0.75f);
		m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
	}
}


void CEffects::Explosion(vec2 Pos)
{
	// add to flow
	for(int y = -8; y <= 8; y++)
		for(int x = -8; x <= 8; x++)
		{
			if(x == 0 && y == 0)
				continue;

			float a = 1 - (length(vec2(x,y)) / length(vec2(8,8)));
			m_pClient->m_pFlow->Add(Pos+vec2(x,y)*16, normalize(vec2(x,y))*5000.0f*a, 10.0f);
		}

	// add the explosion
	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_EXPL01;
	p.m_Pos = Pos;
	p.m_LifeSpan = 0.4f;
	p.m_StartSize = 150.0f;
	p.m_EndSize = 0;
	p.m_Rot = frandom()*pi*2;
	m_pClient->m_pParticles->Add(CParticles::GROUP_EXPLOSIONS, &p);

	// add the smoke
	for(int i = 0; i < 24; i++)
	{
		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_SMOKE;
		p.m_Pos = Pos;
		p.m_Vel = RandomDir() * ((1.0f + frandom()*0.2f) * 1000.0f);
		p.m_LifeSpan = 0.5f + frandom()*0.4f;
		p.m_StartSize = 32.0f + frandom()*8;
		p.m_EndSize = 0;
		p.m_Gravity = frandom()*-800.0f;
		p.m_Friction = 0.4f;
		p.m_Color = mix(vec4(0.75f,0.75f,0.75f,1.0f), vec4(0.5f,0.5f,0.5f,1.0f), frandom());
		m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
	}
}


void CEffects::HammerHit(vec2 Pos)
{
	// add the explosion
	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_HIT01;
	p.m_Pos = Pos;
	p.m_LifeSpan = 0.3f;
	p.m_StartSize = 120.0f;
	p.m_EndSize = 0;
	p.m_Rot = frandom()*pi*2;
	m_pClient->m_pParticles->Add(CParticles::GROUP_EXPLOSIONS, &p);
	m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_HAMMER_HIT, 1.0f, Pos);
}

void CEffects::OnRender()
{
	static int64 LastUpdate100hz = 0;
	static int64 LastUpdate50hz = 0;

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();

		if(time_get()-LastUpdate100hz > time_freq()/(100*pInfo->m_Speed))
		{
			m_Add100hz = true;
			LastUpdate100hz = time_get();
		}
		else
			m_Add100hz = false;

		if(time_get()-LastUpdate50hz > time_freq()/(100*pInfo->m_Speed))
		{
			m_Add50hz = true;
			LastUpdate50hz = time_get();
		}
		else
			m_Add50hz = false;

		if(m_Add50hz)
			m_pClient->m_pFlow->Update();

		return;
	}

	if(time_get()-LastUpdate100hz > time_freq()/100)
	{
		m_Add100hz = true;
		LastUpdate100hz = time_get();
	}
	else
		m_Add100hz = false;

	if(time_get()-LastUpdate50hz > time_freq()/100)
	{
		m_Add50hz = true;
		LastUpdate50hz = time_get();
	}
	else
		m_Add50hz = false;

	if(m_Add50hz)
		m_pClient->m_pFlow->Update();
}
