#ifndef GAME_SERVER_ENTITIES_FLAG_H
#define GAME_SERVER_ENTITIES_FLAG_H

#include <game/server/entity.h>

class CFlag : public CEntity
{
public:
	static const int m_PhysSize = 14;
	class CCharacter *m_pCarryingCCharacter;
	vec2 m_Vel;
	
	int m_Team;
	
	CFlag(CGameWorld *pGameWorld, int Team, vec2 Pos, class CCharacter *pOwner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};
#endif
