/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "../g_game.h"
#include "../generated/gs_data.h"


extern tuning_params tuning;

void create_sound_global(int sound, int target=-1);

inline int cmask_all() { return -1; }
inline int cmask_one(int cid) { return 1<<cid; }
inline int cmask_all_except_one(int cid) { return 0x7fffffff^cmask_one(cid); }
inline bool cmask_is_set(int mask, int cid) { return (mask&cmask_one(cid)) != 0; }

//
class event_handler
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128*64;

	int types[MAX_EVENTS];  // TODO: remove some of these arrays
	int offsets[MAX_EVENTS];
	int sizes[MAX_EVENTS];
	int client_masks[MAX_EVENTS];
	char data[MAX_DATASIZE];
	
	int current_offset;
	int num_events;
public:
	event_handler();
	void *create(int type, int size, int mask = -1);
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
protected:
	int id;
public:
	float proximity_radius;
	unsigned flags;
	int objtype;
	vec2 pos;

	enum
	{
		FLAG_DESTROY=0x00000001,
		FLAG_PHYSICS=0x00000002,
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
		
	virtual bool take_damage(vec2 force, int dmg, int from, int weapon) { return true; }
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
	
	world_core core;
	
	game_world();
	~game_world();
	int find_entities(vec2 pos, float radius, entity **ents, int max);
	int find_entities(vec2 pos, float radius, entity **ents, int max, const int* types, int maxtypes);

	void insert_entity(entity *ent);
	void destroy_entity(entity *ent);
	void remove_entity(entity *ent);
	
	//
	void snap(int snapping_client);
	void tick();
};

extern game_world *world;

// game object
// TODO: should change name of this one
class gameobject : public entity
{
protected:
	void cyclemap();
	void resetgame();
	
	int round_start_tick;
	int game_over_tick;
	int sudden_death;
	
	int teamscore[2];
	
	int warmup;
	int round_count;
	
	bool is_teamplay;
	
public:
	int gametype;
	gameobject();

	void do_team_score_wincheck();
	void do_player_score_wincheck();
	
	void do_warmup(int seconds);
	
	void startround();
	void endround();
	
	bool is_friendly_fire(int cid1, int cid2);
	
	virtual bool on_entity(int index, vec2 pos);
	
	virtual void post_reset();
	virtual void tick();
	
	virtual void on_player_spawn(class player *p) {}
	virtual int on_player_death(class player *victim, class player *killer, int weapon);
	
	virtual void on_player_info_change(class player *p);
	
	virtual void snap(int snapping_client);
	virtual int get_auto_team(int notthisid);
	virtual bool can_join_team(int team, int notthisid);
	int clampteam(int team);
};

extern gameobject *gameobj;


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
	};
	
	vec2 direction;
	entity *powner; // this is nasty, could be removed when client quits
	int lifespan;
	int owner;
	int type;
	int flags;
	int damage;
	int sound_impact;
	int weapon;
	int bounce;
	float force;
	int start_tick;
	
	projectile(int type, int owner, vec2 pos, vec2 vel, int span, entity* powner,
		int damage, int flags, float force, int sound_impact, int weapon);

	vec2 get_pos(float time);
	void fill_info(NETOBJ_PROJECTILE *proj);

	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};

class laser : public entity
{
	vec2 from;
	vec2 dir;
	float energy;
	int bounces;
	int eval_tick;
	player *owner;
	
	bool hit_player(vec2 from, vec2 to);
	void do_bounce();
	
public:
	
	laser(vec2 pos, vec2 direction, float start_energy, player *owner);
	
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};

// player entity
class player : public entity
{
public:
	static const int phys_size = 28;

	// weapon info
	entity *hitobjects[10];
	int numobjectshit;
	struct weaponstat
	{
		int ammoregenstart;
		int ammo;
		int ammocost;
		bool got;
	} weapons[NUM_WEAPONS];
	
	int active_weapon;
	int last_weapon;
	int queued_weapon;
	
	int reload_timer;
	int attack_tick;
	
	int damage_taken;

	int emote_type;
	int emote_stop;

	int last_action; // last tick that the player took any action ie some input
	
	//
	int client_id;
	char skin_name[64];
	int use_custom_color;
	int color_body;
	int color_feet;

	// these are non-heldback inputs
	NETOBJ_PLAYER_INPUT latest_previnput;
	NETOBJ_PLAYER_INPUT latest_input;

	// input	
	NETOBJ_PLAYER_INPUT previnput;
	NETOBJ_PLAYER_INPUT input;
	int num_inputs;
	int jumped;
	
	int damage_taken_tick;

	int health;
	int armor;

	// ninja
	struct
	{
		vec2 activationdir;
		int activationtick;
		int currentcooldown;
		int currentmovetime;
	} ninja;

	//
	int score;
	int team;
	int player_state; // if the client is chatting, accessing a menu or so
	
	bool spawning;
	bool dead;
	int die_tick;
	vec2 die_pos;
	
	// latency calculations
	int latency_accum;
	int latency_accum_min;
	int latency_accum_max;
	int latency_avg;
	int latency_min;
	int latency_max;

	// the player core for the physics	
	player_core core;
	
	//
	int64 last_chat;

	//
	player();
	void init();
	virtual void reset();
	virtual void destroy();
		
	void try_respawn();
	void respawn();
	
	void set_team(int team);
	
	bool is_grounded();
	
	void set_weapon(int w);
	
	void handle_weaponswitch();
	void do_weaponswitch();
	
	int handle_weapons();
	int handle_ninja();
	
	void on_direct_input(NETOBJ_PLAYER_INPUT *input);
	void fire_weapon();

	virtual void tick();
	virtual void tick_defered();
	
	void die(int killer, int weapon);
	
	virtual bool take_damage(vec2 force, int dmg, int from, int weapon);
	virtual void snap(int snaping_client);
};

extern player *players;
