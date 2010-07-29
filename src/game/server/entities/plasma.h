/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef PLASMA_TYPE
#define PLASMA_TYPE

#include <game/server/entity.h>

class CGun;

class CPlasma : public CEntity
{
	vec2 core;
	int eval_tick;
	int lifetime;

	bool hit_character();
	void move();
public:
	CPlasma(CGameWorld *pGameWorld, vec2 pos,vec2 dir);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int snapping_client);
};


#endif