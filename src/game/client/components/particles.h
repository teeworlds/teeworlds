/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_PARTICLES_H
#define GAME_CLIENT_COMPONENTS_PARTICLES_H
#include <base/vmath.h>
#include <game/client/component.h>
#include <game/client/components/maplayers.h>

// particles

enum ParticleFlags //To be removed and replaced by the subclass methods
{
	PFLAG_NONE = 0, 
	PFLAG_DESTROY_IN_AIR = 1, // Particle is destroyed in air
	PFLAG_DESTROY_IN_WATER = 2, // Particle is destroyed in water tile
	PFLAG_DESTROY_ON_IMPACT = 4, //Particle is destroyed on impact with a solid tile
	PFLAG_DESTROY_IN_ANIM_WATER = 8, //Particle is destroyed in animwater water (under surface)
};

struct CPointers //inserted into each virtual function with relevant pointers
{
	CWater* pWater;
	CCollision* pCollision;
	float TimePassed;
};

struct CParticle
{
	CParticle() { SetDefault(); }
	virtual ~CParticle() {}
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
		m_Flags = PFLAG_NONE;
		m_FlowAffected = 1.0f;
		m_Color = vec4(1,1,1,1);
		m_NextPart = 0;
		m_PrevPart = 0;
		m_Elasticity = -1;
	}

	virtual bool CheckDeathConditions(CPointers* pPointers); //Contains Conditions that result in death.
	virtual bool OnDeath(CPointers* pPointers); //Return code 1 means that the particle by default is removed.
	virtual void OnUpdate(CPointers* pPointers); //Contains Other Conditions & Effects
	void Delete();


	vec2 m_Pos;
	vec2 m_Vel;

	int m_Spr;

	float m_FlowAffected;

	float m_LifeSpan;

	float m_StartSize;
	float m_EndSize;

	float m_Elasticity;

	float m_Rot;
	float m_Rotspeed;

	float m_Gravity;
	float m_Friction;

	vec4 m_Color;
	
	int m_Flags;

	// set by the particle system
	float m_Life;
	int m_PrevPart;
	int m_NextPart;
};

struct CAnimatedParticle : CParticle
{ 
	//Currently Empty.
};

struct CBubbleParticle : CAnimatedParticle
{
	CBubbleParticle() { SetDefault(); m_Water = true;  }
public:
	//int m_BubbleStage;
	int m_BubbleStage;
	bool m_Water;

	void OnUpdate(CPointers* CPointers);
	bool OnDeath(CPointers* CPointers);
	bool CheckDeathConditions(CPointers* CPointers);
};

struct CDroplet : CParticle
{
	CDroplet();
		
	bool CheckDeathConditions(CPointers* CPointers);
	void OnUpdate(CPointers* CPointers);
	bool OnDeath(CPointers* CPointers);

	float m_AlphaMultiplier;
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

	CParticle* m_aParticles[MAX_PARTICLES];
	int m_FirstFree;
	int m_aFirstPart[NUM_GROUPS];

	void RenderGroup(int Group);
	void Update(float TimePassed);
	void RemoveParticle(int Group, int Entry);

	CPointers m_Pointers;

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
