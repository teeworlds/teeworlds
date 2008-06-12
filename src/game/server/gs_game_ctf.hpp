/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

// game object
class GAMECONTROLLER_CTF : public GAMECONTROLLER
{
public:
	class FLAG *flags[2];
	
	GAMECONTROLLER_CTF();
	virtual void tick();
	
	virtual bool on_entity(int index, vec2 pos);
	
	virtual void on_player_spawn(class PLAYER *p);
	virtual int on_player_death(class PLAYER *victim, class PLAYER *killer, int weapon);
};

// TODO: move to seperate file
class FLAG : public ENTITY
{
public:
	static const int phys_size = 14;
	PLAYER *carrying_player;
	vec2 vel;
	vec2 stand_pos;
	
	int team;
	int at_stand;
	int drop_tick;
	
	FLAG(int _team);

	virtual void reset();
	virtual void snap(int snapping_client);
};
