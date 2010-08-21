/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_ENTITY_DRAGGER_H
#define GAME_SERVER_ENTITY_DRAGGER_H

#include <game/server/entity.h>

class CCharacter;

class CDragger : public CEntity
{
	vec2 core;
	float strength;
	int eval_tick;
	void move();
	void drag();
	CCharacter *target;
	bool nw;
public:


	CDragger(CGameWorld *pGameWorld, vec2 pos, float strength, bool nw=false);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int snapping_client);
};

#endif
