/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_ENTITY_TRIGGER_H
#define GAME_SERVER_ENTITY_TRIGGER_H

#include <game/server/entity.hpp>

//class DOOR;

class CTrigger : public CEntity
{	
	CEntity *m_Target;

	bool HitCharacter();
	void OpenDoor(int Tick);

public:

	CTrigger(CGameWorld *pGameWorld, vec2 Pos, CEntity *Target);
	
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
