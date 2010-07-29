/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_CEntity_CLight_H
#define GAME_SERVER_CEntity_CLight_H

#include <game/server/entity.h>

class CLight : public CEntity
{
	float rotation;
	vec2 to;
	vec2 core;

	int eval_tick;
	
	int TICK;

	bool hit_character();
	void move();
	void step();
public:
	int cur_length;
	int length_l;
	float ang_speed;
	int speed;
	int length;
	

	CLight(CGameWorld *pGameWorld, vec2 pos, float rotation, int length);
	
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
