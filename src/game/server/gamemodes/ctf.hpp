/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <game/server/gamecontroller.hpp>
#include <game/server/entity.hpp>

class GAMECONTROLLER_CTF : public GAMECONTROLLER
{
public:
	class FLAG *flags[2];
	
	GAMECONTROLLER_CTF();
	virtual void tick();
	
	virtual bool on_entity(int index, vec2 pos);
	virtual int on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon);
};

// TODO: move to seperate file
class FLAG : public ENTITY
{
public:
	static const int phys_size = 14;
	CHARACTER *carrying_character;
	vec2 vel;
	vec2 stand_pos;
	
	int team;
	int at_stand;
	int drop_tick;
	
	FLAG(int _team);

	virtual void reset();
	virtual void snap(int snapping_client);
};
