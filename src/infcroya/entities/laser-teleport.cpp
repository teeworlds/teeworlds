/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				 */
#include <game/server/gamecontext.h>
#include "laser-teleport.h"

CLaserTeleport::CLaserTeleport(CGameWorld *pGameWorld, vec2 StartPos, vec2 EndPos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER_TELEPORT, StartPos)
{
	m_StartPos = StartPos;
	m_EndPos = EndPos;
	m_LaserFired = false;
	GameWorld()->InsertEntity(this);
}

void CLaserTeleport::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaserTeleport::Tick()
{
	if (m_LaserFired)
		GameServer()->m_World.DestroyEntity(this);
}

void CLaserTeleport::Snap(int SnappingClient)
{
	m_LaserFired = true;

	//if (Server()->GetClientAntiPing(SnappingClient))
	//	return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_EndPos.x;
	pObj->m_Y = (int)m_EndPos.y;
	pObj->m_FromX = (int)m_StartPos.x;
	pObj->m_FromY = (int)m_StartPos.y;
	pObj->m_StartTick = Server()->Tick();
}
