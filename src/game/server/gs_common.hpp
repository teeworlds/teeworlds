/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "../g_game.hpp"
#include "../generated/gs_data.hpp"

extern TUNING_PARAMS tuning;

inline int cmask_all() { return -1; }
inline int cmask_one(int cid) { return 1<<cid; }
inline int cmask_all_except_one(int cid) { return 0x7fffffff^cmask_one(cid); }
inline bool cmask_is_set(int mask, int cid) { return (mask&cmask_one(cid)) != 0; }



//
class EVENT_HANDLER
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
	EVENT_HANDLER();
	void *create(int type, int size, int mask = -1);
	void clear();
	void snap(int snapping_client);
};

/*
	Class: Entity
		Basic entity class.
*/
class ENTITY
{
private:
	friend class GAMEWORLD; // thy these?
	ENTITY *prev_entity;
	ENTITY *next_entity;

	ENTITY *prev_type_entity;
	ENTITY *next_type_entity;
protected:
	bool marked_for_destroy;
	int id;
	int objtype;
public:
	
	ENTITY(int objtype);
	virtual ~ENTITY();
	
	ENTITY *typenext() { return next_type_entity; }
	ENTITY *typeprev() { return prev_type_entity; }

	/*
		Function: destroy
		Destorys the entity.
	*/
	virtual void destroy() { delete this; }
		
	/*
		Function: reset
		Called when the game resets the map. Puts the entity
		back to it's starting state or perhaps destroys it.
	*/
	virtual void reset() {}
	
	/*
		Function: tick
		Called progress the entity to the next tick. Updates
		and moves the entity to it's new state and position.
	*/
	virtual void tick() {}

	/*
		Function: tick
		Called after all entities tick() function has been called.
	*/
	virtual void tick_defered() {}
	
	/*
		Function: snap
		Called when a new snapshot is being generated for a specific
		client.
		
		Arguments:
			snapping_client - ID of the client which snapshot is
				being generated. Could be -1 to create a complete
				snapshot of everything in the game for demo
				recording.
	*/
	virtual void snap(int snapping_client) {}

	/*
		Variable: proximity_radius
		Contains the physical size of the entity.
	*/
	float proximity_radius;
	
	/*
		Variable: pos
		Contains the current posititon of the entity.
	*/
	vec2 pos;
};


/*
	Class: Game World
		Tracks all entities in the game. Propagates tick and
		snap calls to all entities.
*/
class GAMEWORLD
{
	void reset();
	void remove_entities();

	enum
	{
		NUM_ENT_TYPES=10, // TODO: are more exact value perhaps? :)
	};

	// TODO: two lists seams kinda not good, shouldn't be needed
	ENTITY *first_entity;
	ENTITY *first_entity_types[NUM_ENT_TYPES];

public:
	bool reset_requested;
	bool paused;
	WORLD_CORE core;
	
	GAMEWORLD();
	~GAMEWORLD();
	
	ENTITY *find_first() { return first_entity; }
	ENTITY *find_first(int type);
	
	/*
		Function: find_entities
			Finds entities close to a position and returns them in a list.
		Arguments:
			pos - Position.
			radius - How close the entities have to be.
			ents - Pointer to a list that should be filled with the pointers
				to the entities.
			max - Number of entities that fits into the ents array.
			type - Type of the entities to find. -1 for all types.
		Returns:
			Number of entities found and added to the ents array.
	*/
	int find_entities(vec2 pos, float radius, ENTITY **ents, int max, int type = -1);
	
	/*
		Function: interserct_character
			Finds the closest character that intersects the line.
		Arguments:
			pos0 - Start position
			pos2 - End position
			radius - How for from the line the character is allowed to be.
			new_pos - Intersection position
			notthis - Entity to ignore intersecting with
		Returns:
			Returns a pointer to the closest hit or NULL of there is no intersection.
	*/
	class CHARACTER *intersect_character(vec2 pos0, vec2 pos1, float radius, vec2 &new_pos, class ENTITY *notthis = 0);
	
	/*
		Function: closest_character
			Finds the closest character to a specific point.
		Arguments:
			pos - The center position.
			radius - How far off the character is allowed to be
			notthis - Entity to ignore
		Returns:
			Returns a pointer to the closest character or NULL if no character is close enough.
	*/
	class CHARACTER *closest_character(vec2 pos, float radius, ENTITY *notthis);

	/*
		Function: insert_entity
			Adds an entity to the world.
		Arguments:
			entity - Entity to add
	*/
	void insert_entity(ENTITY *entity);

	/*
		Function: remove_entity
			Removes an entity from the world.
		Arguments:
			entity - Entity to remove
	*/
	void remove_entity(ENTITY *entity);

	/*
		Function: destroy_entity
			Destroys an entity in the world.
		Arguments:
			entity - Entity to destroy
	*/
	void destroy_entity(ENTITY *entity);
	
	/*
		Function: snap
			Calls snap on all the entities in the world to create
			the snapshot.
		Arguments:
			snapping_client - ID of the client which snapshot
			is being created.
	*/
	void snap(int snapping_client);
	
	/*
		Function: tick
			Calls tick on all the entities in the world to progress
			the world to the next tick.
		
	*/
	void tick();
};

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/
class GAMECONTROLLER
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
	GAMECONTROLLER();

	void do_team_score_wincheck();
	void do_player_score_wincheck();
	
	void do_warmup(int seconds);
	
	void startround();
	void endround();
	
	bool is_friendly_fire(int cid1, int cid2);

	/*
	
	*/	
	virtual void tick();
	
	virtual void snap(int snapping_client);
	
	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.
		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.
		Returns:
			bool?
	*/
	virtual bool on_entity(int index, vec2 pos);
	
	/*
		Function: on_character_spawn
			Called when a character spawns into the game world.
		Arguments:
			chr - The character that was spawned.
	*/
	virtual void on_character_spawn(class CHARACTER *chr) {}
	
	/*
		Function: on_character_death
			Called when a character in the world dies.
		Arguments:
			victim - The character that died.
			killer - The player that killed it.
			weapon - What weapon that killed it. Can be -1 for undefined
				weapon when switching team or player suicides.
	*/
	virtual int on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon);

	virtual void on_player_info_change(class PLAYER *p);

	/*
	
	*/	
	virtual const char *get_team_name(int team);
	virtual int get_auto_team(int notthisid);
	virtual bool can_join_team(int team, int notthisid);
	int clampteam(int team);

	virtual void post_reset();
};

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

// projectile entity
class PROJECTILE : public ENTITY
{
public:
	enum
	{
		PROJECTILE_FLAGS_EXPLODE = 1 << 0,
	};
	
	vec2 direction;
	ENTITY *powner; // this is nasty, could be removed when client quits
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
	
	PROJECTILE(int type, int owner, vec2 pos, vec2 vel, int span, ENTITY* powner,
		int damage, int flags, float force, int sound_impact, int weapon);

	vec2 get_pos(float time);
	void fill_info(NETOBJ_PROJECTILE *proj);

	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};

class LASER : public ENTITY
{
	vec2 from;
	vec2 dir;
	float energy;
	int bounces;
	int eval_tick;
	CHARACTER *owner;
	
	bool hit_character(vec2 from, vec2 to);
	void do_bounce();
	
public:
	
	LASER(vec2 pos, vec2 direction, float start_energy, CHARACTER *owner);
	
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};


class CHARACTER : public ENTITY
{
public:
	// player controlling this character
	class PLAYER *player;
	
	bool alive;

	// weapon info
	ENTITY *hitobjects[10];
	int numobjectshit;
	struct WEAPONSTAT
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

	// TODO: clean this up
	char skin_name[64];
	int use_custom_color;
	int color_body;
	int color_feet;
	
	int last_action; // last tick that the player took any action ie some input

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
	//int score;
	int team;
	int player_state; // if the client is chatting, accessing a menu or so

	// the player core for the physics	
	CHARACTER_CORE core;

	//
	CHARACTER();
	
	virtual void reset();
	virtual void destroy();
		
	bool is_grounded();
	
	void set_weapon(int w);
	
	void handle_weaponswitch();
	void do_weaponswitch();
	
	int handle_weapons();
	int handle_ninja();

	void on_predicted_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_direct_input(NETOBJ_PLAYER_INPUT *new_input);
	void fire_weapon();

	void die(int killer, int weapon);

	bool take_damage(vec2 force, int dmg, int from, int weapon);	

	
	bool spawn(PLAYER *player, vec2 pos, int team);
	//bool init_tryspawn(int team);
	bool remove();

	static const int phys_size = 28;

	virtual void tick();
	virtual void tick_defered();
	virtual void snap(int snaping_client);
	
	bool increase_health(int amount);
	bool increase_armor(int amount);
};

// player object
class PLAYER
{
public:
	PLAYER();

	// TODO: clean this up
	char skin_name[64];
	int use_custom_color;
	int color_body;
	int color_feet;
	
	//
	bool spawning;
	int client_id;
	int team;
	int score;

	//
	int64 last_chat;

	// network latency calculations	
	struct
	{
		int accum;
		int accum_min;
		int accum_max;
		int avg;
		int min;
		int max;	
	} latency;
	
	CHARACTER character;
	
	// this is used for snapping so we know how we can clip the view for the player
	vec2 view_pos;

	void init(int client_id);
	
	CHARACTER *get_character();
	
	void kill_character();

	void try_respawn();
	void respawn();
	void set_team(int team);
	
	void tick();
	void snap(int snaping_client);

	void on_direct_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_predicted_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_disconnect();
};

class GAMECONTEXT
{
public:
	GAMECONTEXT();
	void clear();
	
	EVENT_HANDLER events;
	PLAYER players[MAX_CLIENTS];
	
	GAMECONTROLLER *controller;
	GAMEWORLD world;

	void tick();
	void snap(int client_id);

	// helper functions
	void create_damageind(vec2 p, float angle_mod, int amount);
	void create_explosion(vec2 p, int owner, int weapon, bool bnodamage);
	void create_smoke(vec2 p);
	void create_playerspawn(vec2 p);
	void create_death(vec2 p, int who);
	void create_sound(vec2 pos, int sound, int mask=-1);
	void create_sound_global(int sound, int target=-1);	

	// network
	void send_chat(int cid, int team, const char *text);
	void send_emoticon(int cid, int emoticon);
	void send_weapon_pickup(int cid, int weapon);
	void send_broadcast(const char *text, int cid);
	void send_info(int who, int to_who);
};

extern GAMECONTEXT game;

enum
{
	CHAT_ALL=-2,
	CHAT_SPEC=-1,
	CHAT_RED=0,
	CHAT_BLUE=1
};
