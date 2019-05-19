#pragma once

#include <game/server/entity.h>
#include <base/tl/array.h>

class CInfCircle : public CEntity
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
	CInfCircle(CGameWorld* pGameWorld, vec2 Pos, int Owner, float Radius);
	~CInfCircle();

	void Snap(int SnappingClient) override;
	void Tick() override;
	void Reset() override;

	void Translate(float x, float y);

	float GetRadius();
	void SetRadius(float R);
};