/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef CGun_TYPE
#define CGun_TYPE

#include <game/server/entity.h>
#include <game/gamecore.h>

class CCharacter;

class CGun : public CEntity
{
	int eval_tick;

	vec2 core;

	void fire();
	int DELAY;
	
public:
	CGun(CGameWorld *pGameWorld, vec2 pos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};


#endif
