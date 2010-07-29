/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_ENTITY_DRAGER_H
#define GAME_SERVER_ENTITY_DRAGER_H

#include <game/server/entity.h>

class CCharacter;

class CDrager : public CEntity
{
	vec2 core;
	float strength;
	int eval_tick;
	void move();
	void drag();
	CCharacter *target;
	bool nw;
public:


	CDrager(CGameWorld *pGameWorld, vec2 pos, float strength, bool nw=false);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int snapping_client);
};

#endif
