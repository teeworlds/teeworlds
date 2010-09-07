#ifndef GAME_SERVER_ENTITY_DOOR_H
#define GAME_SERVER_ENTITY_DOOR_H

#include <game/server/entity.h>

class CTrigger;

class CDoor : public CEntity
{
	vec2 m_To;
	int m_EvalTick;
	bool m_Opened;
	bool HitCharacter();
	bool m_QueueForClose;

public:

	void Open(int Tick);
	void Close();
	CDoor(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length, bool Opened);
	
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
