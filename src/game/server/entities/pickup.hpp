/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_ENTITY_PICKUP_H
#define GAME_SERVER_ENTITY_PICKUP_H

#include <game/server/entity.hpp>

// TODO: move to seperate file
class PICKUP : public ENTITY
{
public:
	static const int phys_size = 14;
	
	int type;
	int subtype; // weapon type for instance?
	int spawntick;
	PICKUP(int _type, int _subtype = 0);
	
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};

#endif
