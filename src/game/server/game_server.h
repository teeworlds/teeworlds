
//
class event_handler
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128*4;

	int types[MAX_EVENTS];  // TODO: remove some of these arrays
	int offsets[MAX_EVENTS];
	int sizes[MAX_EVENTS];
	int targets[MAX_EVENTS];
	char data[MAX_DATASIZE];
	
	int current_offset;
	int num_events;
public:
	event_handler();
	void *create(int type, int size, int target = -1);
	void clear();
	void snap(int snapping_client);
};

extern event_handler events;

// a basic entity
class entity
{
private:
	friend class game_world;
	friend class player;
	entity *prev_entity;
	entity *next_entity;

	entity *prev_type_entity;
	entity *next_type_entity;

	int index;
	static int current_id;
protected:
	int id;
	
public:
	float proximity_radius;
	unsigned flags;
	int objtype;
	baselib::vec2 pos;

	enum
	{
		FLAG_DESTROY=0x00000001,
		FLAG_ALIVE=0x00000002,
	};
	
	entity(int objtype);
	virtual ~entity();
	
	virtual void reset() {}
	virtual void post_reset() {}
	
	void set_flag(unsigned flag) { flags |= flag; }
	void clear_flag(unsigned flag) { flags &= ~flag; }

	virtual void destroy() { delete this; }
	virtual void tick() {}
	virtual void tick_defered() {}
		
	virtual void snap(int snapping_client) {}
		
	virtual bool take_damage(baselib::vec2 force, int dmg, int from, int weapon) { return true; }
};


class game_world
{
	void reset();
	void remove_entities();
public:
	enum
	{
		NUM_ENT_TYPES=10, // TODO: are more exact value perhaps? :)
	};

	// TODO: two lists seams kinda not good, shouldn't be needed
	entity *first_entity;
	entity *first_entity_types[NUM_ENT_TYPES];
	bool paused;
	bool reset_requested;
	
	game_world();
	int find_entities(baselib::vec2 pos, float radius, entity **ents, int max);
	int find_entities(baselib::vec2 pos, float radius, entity **ents, int max, const int* types, int maxtypes);

	void insert_entity(entity *ent);
	void destroy_entity(entity *ent);
	void remove_entity(entity *ent);
	
	//
	void snap(int snapping_client);
	void tick();
};

extern game_world world;

// game object
class gameobject : public entity
{
	void resetgame();
	void startround();
	void endround();
	
	int round_start_tick;
	int game_over_tick;
	int sudden_death;
	
public:
	int gametype;
	gameobject();
	virtual void post_reset();
	virtual void tick();
	virtual void tick_dm();
	virtual void tick_tdm();
	virtual void snap(int snapping_client);
	virtual int getteam(int notthisid);
};

extern gameobject gameobj;


// TODO: move to seperate file
class powerup : public entity
{
public:
	static const int phys_size = 14;
	
	int type;
	int subtype; // weapon type for instance?
	int spawntick;
	powerup(int _type, int _subtype = 0);
	
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};



// projectile entity
class projectile : public entity
{
public:
	enum
	{
		PROJECTILE_FLAGS_EXPLODE = 1 << 0,
		
		WEAPON_PROJECTILETYPE_GUN		= 0,
		WEAPON_PROJECTILETYPE_ROCKET	= 1,
		WEAPON_PROJECTILETYPE_SHOTGUN	= 2,
	};
	
	baselib::vec2 vel;
	entity *powner; // this is nasty, could be removed when client quits
	int lifespan;
	int owner;
	int type;
	int flags;
	int damage;
	int sound_impact;
	int weapon;
	float force;
	
	projectile(int type, int owner, baselib::vec2 pos, baselib::vec2 vel, int span, entity* powner,
		int damage, int flags, float force, int sound_impact, int weapon);
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};

class projectile_backpackrocket : public projectile
{
	int stage;
	int start_tick;
	int deply_ticks;
	baselib::vec2 target;
	baselib::vec2 start;
	baselib::vec2 midpoint;
	baselib::vec2 direction;
public:
	projectile_backpackrocket(baselib::vec2 pos, baselib::vec2 target, int owner, entity* powner);
	virtual void tick();
};

// player entity
class player : public entity
{
public:
	static const int phys_size = 28;
	
	enum // what are these?
	{
		MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY		= 1 << 0,
		MODIFIER_RETURNFLAGS_OVERRIDEPOSITION		= 1 << 1,
		MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY		= 1 << 2,
	};

	// weapon info
	entity* hitobjects[10];
	int numobjectshit;
	struct weaponstat
	{
		int ammoregenstart;
		int ammo;
		int ammocost;
		bool got;
	} weapons[NUM_WEAPONS];
	int active_weapon;
	int reload_timer;
	int attack_tick;
	
	// we need a defered position so we can handle the physics correctly
	baselib::vec2 defered_pos;
	baselib::vec2 vel;
	baselib::vec2 direction;

	//
	int client_id;
	char name[64];

	// input	
	player_input previnput;
	player_input input;
	int jumped;
	
	int damage_taken_tick;

	int health;
	int armor;

	// ninja
	baselib::vec2 activationdir;
	int ninjaactivationtick;
	int extrapowerflags;
	int currentcooldown;
	int currentactivation;
	int currentmovetime;

	int score;
	int team;
	int state;
	
	bool spawning;
	bool dead;
	int die_tick;
	
	int latency_accum;
	int latency_accum_min;
	int latency_accum_max;
	int latency_avg;
	int latency_min;
	int latency_max;

	// hooking stuff
	enum
	{
		HOOK_RETRACTED=-1,
		HOOK_IDLE=0,
		HOOK_FLYING,
		HOOK_GRABBED
	};
	
	int hook_state;
	int hook_tick;
	player *hooked_player;
	baselib::vec2 hook_pos;
	baselib::vec2 hook_dir;

	//
	player();
	void init();
	virtual void reset();
	virtual void destroy();
		
	void try_respawn();
	void respawn();

	bool is_grounded();

	void release_hooked();
	void release_hooks();
	
	int handle_weapons();
	int handle_ninja();

	virtual void tick();
	virtual void tick_defered();
	
	void die(int killer, int weapon);
	
	virtual bool take_damage(baselib::vec2 force, int dmg, int from, int weapon);
	virtual void snap(int snaping_client);
};

extern player players[MAX_CLIENTS];

// TODO: move to seperate file
class flag : public entity
{
public:
	static const int phys_size = 14;
	player *carrying_player;
	baselib::vec2 vel;
	
	int team;
	int spawntick;
	flag(int _team);

	bool is_grounded();
	
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};
