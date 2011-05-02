/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_PARTICLES_H
#define GAME_CLIENT_COMPONENTS_PARTICLES_H
#include <base/vmath.h>
#include <game/client/component.h>

// particles
struct CParticle
{
	void SetDefault()
	{
		m_Vel = vec2(0,0);
		m_LifeSpan = 0;
		m_StartSize = 32;
		m_EndSize = 32;
		m_Rot = 0;
		m_Rotspeed = 0;
		m_Gravity = 0;
		m_Friction = 0;
		m_FlowAffected = 1.0f;
		m_Color = vec4(1,1,1,1);
	}

	vec2 m_Pos;
	vec2 m_Vel;

	int m_Spr;

	float m_FlowAffected;

	float m_LifeSpan;

	float m_StartSize;
	float m_EndSize;

	float m_Rot;
	float m_Rotspeed;

	float m_Gravity;
	float m_Friction;

	vec4 m_Color;

	// set by the particle system
	float m_Life;
	int m_PrevPart;
	int m_NextPart;
};

class CParticles : public CComponent
{
	friend class CGameClient;
public:
	enum
	{
		GROUP_PROJECTILE_TRAIL=0,
		GROUP_EXPLOSIONS,
		GROUP_GENERAL,
		NUM_GROUPS
	};

	CParticles();

	void Add(int Group, CParticle *pPart);

	virtual void OnReset();
	virtual void OnRender();

private:

	enum
	{
		MAX_PARTICLES=1024*8,
	};

	CParticle m_aParticles[MAX_PARTICLES];
	int m_FirstFree;
	int m_aFirstPart[NUM_GROUPS];

	void RenderGroup(int Group);
	void Update(float TimePassed);

	template<int TGROUP>
	class CRenderGroup : public CComponent
	{
	public:
		CParticles *m_pParts;
		virtual void OnRender() { m_pParts->RenderGroup(TGROUP); }
	};

	CRenderGroup<GROUP_PROJECTILE_TRAIL> m_RenderTrail;
	CRenderGroup<GROUP_EXPLOSIONS> m_RenderExplosions;
	CRenderGroup<GROUP_GENERAL> m_RenderGeneral;
};
#endif
