/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

// game object
class gameobject_ctf : public gameobject
{
public:
	class flag *flags[2];
	
	gameobject_ctf();
	virtual void tick();
	
	virtual bool on_entity(int index, vec2 pos);
	
	virtual void on_player_spawn(class player *p);
	virtual int on_player_death(class player *victim, class player *killer, int weapon);
};

// TODO: move to seperate file
class flag : public entity
{
public:
	static const int phys_size = 14;
	player *carrying_player;
	vec2 vel;
	vec2 stand_pos;
	
	int team;
	int at_stand;
	int drop_tick;
	
	flag(int _team);

	virtual void reset();
	virtual void snap(int snapping_client);
};
