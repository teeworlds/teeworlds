#ifndef GAME_SERVER_ENTITIES_LASER_TELEPORT_H
#define GAME_SERVER_ENTITIES_LASER_TELEPORT_H

#include <game/server/entity.h>

class CLaserTeleport : public CEntity
{

public:
	CLaserTeleport(CGameWorld *pGameWorld, vec2 StartPos, vec2 EndPos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_StartPos;
	vec2 m_EndPos;
	bool m_LaserFired;

};

#endif
