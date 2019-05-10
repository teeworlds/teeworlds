#pragma once

#include <game/server/entity.h>
#include <base/tl/array.h>

class CCircle : public CEntity
{
private:
	enum
	{
		NUM_SIDE = 48,
		NUM_HINT = 48,
		NUM_IDS = NUM_SIDE + NUM_HINT,
	};
	int m_Owner;
	int m_IDs[NUM_IDS];
	float m_Radius;
public:
	CCircle(CGameWorld* pGameWorld, vec2 Pos, int Owner);
	~CCircle();

	void Snap(int SnappingClient) override;
	void Tick() override;
	void Reset() override;
};