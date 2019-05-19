/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "circle.h"

#include "growingexplosion.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>

CCircle::CCircle(CGameWorld* pGameWorld, vec2 Pos, int Owner, float Radius)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_SCIENTIST_MINE, Pos)
{
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);
	m_Radius = Radius;
	m_StartTick = Server()->Tick();
	m_Owner = Owner;

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

void CCircle::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

int CCircle::GetOwner() const
{
	return m_Owner;
}

void CCircle::Snap(int SnappingClient)
{
	float Radius = m_Radius;

	int NumSide = CCircle::NUM_SIDE;
	//if(Server()->GetClientAntiPing(SnappingClient))
	//	NumSide = std::min(6, NumSide);

	float AngleStep = 2.0f * pi / NumSide;

	for (int i = 0; i < NumSide; i++)
	{
		vec2 PartPosStart = m_Pos + vec2(Radius * cos(AngleStep * i), Radius * sin(AngleStep * i));
		vec2 PartPosEnd = m_Pos + vec2(Radius * cos(AngleStep * (i + 1)), Radius * sin(AngleStep * (i + 1)));

		CNetObj_Laser * pObj = static_cast<CNetObj_Laser*>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
		if (!pObj)
			return;

		pObj->m_X = (int)PartPosStart.x;
		pObj->m_Y = (int)PartPosStart.y;
		pObj->m_FromX = (int)PartPosEnd.x;
		pObj->m_FromY = (int)PartPosEnd.y;
		pObj->m_StartTick = Server()->Tick();
	}
}

void CCircle::Tick()
{
	if (IsMarkedForDestroy()) return;
}

void CCircle::TickPaused()
{
	++m_StartTick;
}
