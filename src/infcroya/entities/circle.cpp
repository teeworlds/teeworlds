#include "circle.h"
#include <game/server/gamecontext.h>

CCircle::CCircle(CGameWorld* pGameWorld, vec2 Pos, int Owner) : CEntity(pGameWorld, CGameWorld::ENTTYPE_CIRCLE, Pos)
{
	m_Owner = Owner;
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);
	for (int i = 0; i < NUM_IDS; i++)
	{
		m_IDs[i] = Server()->SnapNewID();
	}
}

CCircle::~CCircle()
{
	for (int i = 0; i < NUM_IDS; i++)
	{
		Server()->SnapFreeID(m_IDs[i]);
	}
}

void CCircle::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;
	const float MAGIC_NUMBER = NUM_SIDE / 3.125f; // a magic number for NUM_SIDE = 24, NUM_HINT = 24, when dots stay
	float AngleStart = (2.0f * pi * Server()->Tick() / static_cast<float>(Server()->TickSpeed())) / (MAGIC_NUMBER / 8);
	float AngleStep = 2.0f * pi / NUM_SIDE;
	m_Radius = 480.0f;
	AngleStart = AngleStart * 2.0f;
	for (int i = 0; i < NUM_SIDE; i++)
	{
		vec2 PosStart = m_Pos + vec2(m_Radius * cos(AngleStart + AngleStep * i), m_Radius * sin(AngleStart + AngleStep * i));

		CNetObj_Projectile * pObj = static_cast<CNetObj_Projectile*>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDs[NUM_SIDE + i], sizeof(CNetObj_Projectile)));
		if (pObj)
		{
			pObj->m_X = (int)PosStart.x;
			pObj->m_Y = (int)PosStart.y;
			pObj->m_VelX = 0;
			pObj->m_VelY = 0;
			pObj->m_StartTick = Server()->Tick();
			pObj->m_Type = WEAPON_HAMMER;
		}
	}
}

void CCircle::Tick()
{
	if (IsMarkedForDestroy()) return;
}

void CCircle::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}
