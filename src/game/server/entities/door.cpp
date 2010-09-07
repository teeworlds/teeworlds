#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "door.h"

CDoor::CDoor(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length, bool Opened)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Pos = Pos;
	m_Opened = Opened;

	vec2 Dir = vec2(sin(Rotation), cos(Rotation));
	vec2 To = Pos + normalize(Dir)*Length;
	
	GameServer()->Collision()->IntersectNoLaser(Pos, To, &this->m_To, 0);
	GameWorld()->InsertEntity(this);
}

void CDoor::Open(int Tick)
{
	m_EvalTick = Tick;
	m_Opened = true;
}

void CDoor::Close()
{
	m_Opened = false;
}

bool CDoor::HitCharacter()
{
	return GameServer()->m_World.IntersectCharacters(m_Pos, m_To, 1.f, 0);
}

void CDoor::Reset()
{
	m_Opened = false;
}

void CDoor::Tick()
{
	if (!m_Opened)
		HitCharacter();
	else if (m_EvalTick + 10 < Server()->Tick())
		Close();
	return;
}

void CDoor::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient, m_To))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;

	if (!m_Opened)
	{
		pObj->m_FromX = (int)m_To.x;
		pObj->m_FromY = (int)m_To.y;
	}
	else
	{
		pObj->m_FromX = (int)m_Pos.x;
		pObj->m_FromY = (int)m_Pos.y;
	}
	pObj->m_StartTick = Server()->Tick();
}
