/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "entity.h"
#include "gamecontext.h"
#include "player.h"
#include "game/collision.h"

CEntity::CEntity(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int ProximityRadius)
{
	m_pGameWorld = pGameWorld;

	m_pPrevTypeEntity = 0;
	m_pNextTypeEntity = 0;

	m_ID = Server()->SnapNewID();
	m_ObjType = ObjType;

	m_ProximityRadius = ProximityRadius;

	m_MarkedForDestroy = false;
	m_MarkForHarpoonDeallocation = false;
	m_Pos = Pos;

	m_HarpoonVel = vec2(0,0);
}

CEntity::~CEntity()
{
	GameWorld()->RemoveEntity(this);
	Server()->SnapFreeID(m_ID);
}

int CEntity::NetworkClipped(int SnappingClient)
{
	return NetworkClipped(SnappingClient, m_Pos);
}

int CEntity::NetworkClipped(int SnappingClient, vec2 CheckPos)
{
	if(SnappingClient == -1)
		return 0;

	float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x-CheckPos.x;
	float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y-CheckPos.y;

	if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
		return 1;

	if(distance(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, CheckPos) > 1100.0f)
		return 1;
	return 0;
}

bool CEntity::GameLayerClipped(vec2 CheckPos)
{
	int rx = round_to_int(CheckPos.x) / 32;
	int ry = round_to_int(CheckPos.y) / 32;
	return (rx < -200 || rx >= GameServer()->Collision()->GetWidth()+200)
			|| (ry < -200 || ry >= GameServer()->Collision()->GetHeight()+200);
}

void CEntity::ApplyHarpoonDrag()
{
	vec2 NewPos = m_Pos;
	GameServer()->Collision()->MoveBox(&NewPos, &m_HarpoonVel, vec2(6.0f, 6.0f), 0, 0);

	m_Pos = NewPos;

	m_HarpoonVel = vec2(0, 0);
}

void CEntity::ApplyHarpoonVel(vec2 Vel)
{
	Vel *= GameServer()->Tuning()->m_HarpoonEntityMultiplier;
	float DragSpeed = GameServer()->Tuning()->m_HarpoonEntityDragSpeed;
	float MinimumSpeed = GameServer()->Tuning()->m_HarpoonEntityMinSpeed;
	if (length(Vel) < MinimumSpeed)
	{
		Vel = normalize(Vel);
		Vel *= MinimumSpeed;
	}
	m_HarpoonVel.x += clamp(Vel.x, -DragSpeed, DragSpeed);
	m_HarpoonVel.y += clamp(Vel.y, -DragSpeed, DragSpeed);
}

bool CEntity::IsValidForHarpoon(CHarpoon* pHarpoon)
{
	return true;
}