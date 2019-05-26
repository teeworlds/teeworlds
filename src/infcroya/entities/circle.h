#pragma once

#include <game/server/entity.h>

class CCircle : public CEntity
{
public:
	enum
	{
		NUM_SIDE = 12,
		NUM_PARTICLES = 12,
		NUM_IDS = NUM_SIDE + NUM_PARTICLES,
	};

public:
	CCircle(CGameWorld* pGameWorld, vec2 Pos, int Owner, float Radius, float MinRadius, float ShrinkSpeed);
	virtual ~CCircle();

	virtual void Snap(int SnappingClient);
	virtual void Reset();
	virtual void TickPaused();
	virtual void Tick();

	int GetOwner() const;
	float GetRadius() const;
	void SetRadius(float Radius);

	float GetMinRadius() const;
	void SetMinRadius(float MinRadius);

	float GetShrinkSpeed() const;
	void SetShrinkSpeed(float ShrinkSpeed);

private:
	int m_IDs[NUM_IDS];
	float m_Radius;
	float m_MinRadius;
	float m_ShrinkSpeed;

public:
	int m_StartTick;
	int m_Owner;
};