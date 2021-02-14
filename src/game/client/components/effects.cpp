/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/engine.h>

#include <engine/shared/config.h>

#include <generated/client_data.h>

#include <game/client/components/particles.h>
#include <game/client/components/skins.h>
#include <game/client/components/flow.h>
#include <game/client/components/damageind.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>

#include "effects.h"

inline vec2 RandomDir() { return normalize(vec2(frandom()-0.5f, frandom()-0.5f)); }

struct CDamageVariation
{
	float m_Angle;
	float m_Length;
};

static CDamageVariation s_aDamageVariations[] = {
	{0, 1.0f}, {-0.30f, 1.0f}, {0.30f, 1.0f}, {0.6f, 1.0f}, {-0.6f, 1.0f},
	{0, 1.2f}, {-0.25f, 1.2f}, {0.25f, 1.2f}, {0.5f, 1.2f}, {-0.5f, 1.2f},
	{0, 0.8f}, {-0.35f, 0.8f}, {0.35f, 0.8f}, {0.7f, 0.8f}, {-0.7f, 0.8f}
};

void CEffects::CreateDamageIndicator(CDamageIndInfo *pDamage, int ClientID)
{
	// ignore if there is no damage
	int Amount = pDamage->m_HealthAmount + pDamage->m_ArmorAmount;
	if(Amount <= 0 || pDamage->m_NumEvents == 0)
		return;

	int VariationIndex = 0;
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		CDamageIndState *pState = &m_aClientDamageState[ClientID];
		int CurrentTick = Client()->GameTick();
		if(pState->m_LastDamageTick + 20 < CurrentTick)
			pState->m_Variation = 0;
		else if(pState->m_LastDamage % 2 == Amount % 2) // make two-tick shotgun damage look good
			pState->m_Variation = (pState->m_Variation + 1) % (sizeof(s_aDamageVariations) / sizeof(CDamageVariation));

		pState->m_LastDamageTick = CurrentTick;
		pState->m_LastDamage = Amount;
		VariationIndex = pState->m_Variation;
	}
	CDamageVariation Variation = s_aDamageVariations[VariationIndex];

	float Angle = atan2f(pDamage->m_SumSin / pDamage->m_NumEvents, pDamage->m_SumCos / pDamage->m_NumEvents) + Variation.m_Angle;
	vec2 Pos = pDamage->m_Pos / pDamage->m_NumEvents;

	float s = Angle-pi/3;
	float e = Angle+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+1));
		m_pClient->m_pDamageind->Create(Pos, direction(f)*-1*Variation.m_Length, ClientID);
	}
}

CEffects::CEffects()
{
	m_Add50hz = false;
	m_Add100hz = false;
	mem_zero(m_aClientDamage, sizeof(m_aClientDamage));
	mem_zero(m_aClientDamageState, sizeof(m_aClientDamageState));
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

void CEffects::DamageIndicator(vec2 Pos, int HealthAmount, int ArmorAmount, float Angle, int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
	{
		CDamageIndInfo Damage;
		Damage.m_Pos = Pos;
		Damage.m_HealthAmount = HealthAmount;
		Damage.m_ArmorAmount = ArmorAmount;
		Damage.m_SumSin = sinf(Angle);
		Damage.m_SumCos = cosf(Angle);
		Damage.m_NumEvents = 1;
		CreateDamageIndicator(&Damage, -1);
	}
	else
	{
		m_aClientDamage[ClientID].m_HealthAmount += HealthAmount;
		m_aClientDamage[ClientID].m_ArmorAmount += ArmorAmount;
		m_aClientDamage[ClientID].m_SumSin += sinf(Angle);
		m_aClientDamage[ClientID].m_SumCos += cosf(Angle);
		m_aClientDamage[ClientID].m_Pos += Pos;
		m_aClientDamage[ClientID].m_NumEvents++;
	}
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
		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		{
			int ColorVal = m_pClient->m_pSkins->GetTeamColor(m_pClient->m_aClients[ClientID].m_aUseCustomColors[SKINPART_BODY], m_pClient->m_aClients[ClientID].m_aSkinPartColors[SKINPART_BODY],
																m_pClient->m_aClients[ClientID].m_Team, SKINPART_BODY);
			BloodColor = m_pClient->m_pSkins->GetColorV3(ColorVal);
		}
		else
		{
			if(m_pClient->m_aClients[ClientID].m_aUseCustomColors[SKINPART_BODY])
				BloodColor = m_pClient->m_pSkins->GetColorV3(m_pClient->m_aClients[ClientID].m_aSkinPartColors[SKINPART_BODY]);
			else
			{
				const CSkins::CSkinPart *s = m_pClient->m_pSkins->GetSkinPart(SKINPART_BODY, m_pClient->m_aClients[ClientID].m_SkinPartIDs[SKINPART_BODY]);
				if(s)
					BloodColor = s->m_BloodColor;
			}
		}
	}

	for(int i = 0; i < 64; i++)
	{
		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_SPLAT01 + (random_int()%3);
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

void CEffects::ApplyDamageIndicators()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		CreateDamageIndicator(&m_aClientDamage[i], i);
	mem_zero(m_aClientDamage, sizeof(m_aClientDamage));
}

void CEffects::OnReset()
{
	mem_zero(m_aClientDamage, sizeof(m_aClientDamage));
	mem_zero(m_aClientDamageState, sizeof(m_aClientDamageState));
}

void CEffects::OnRender()
{
	static int64 s_LastUpdate100hz = 0;
	static int64 s_LastUpdate50hz = 0;

	const float Speed = GetEffectsSpeed();
	const int64 Now = time_get();
	const int64 Freq = time_freq();

	if(Now-s_LastUpdate100hz > Freq/(100*Speed))
	{
		m_Add100hz = true;
		s_LastUpdate100hz = Now;
	}
	else
		m_Add100hz = false;

	if(Now-s_LastUpdate50hz > Freq/(50*Speed))
	{
		m_Add50hz = true;
		s_LastUpdate50hz = Now;
	}
	else
		m_Add50hz = false;

	if(m_Add50hz)
		m_pClient->m_pFlow->Update();
}

float CEffects::GetEffectsSpeed()
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return DemoPlayer()->BaseInfo()->m_Speed;
	return 1.0f;
}
