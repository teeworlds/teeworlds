/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_ENTITY_CHARACTER_H
#define GAME_SERVER_ENTITY_CHARACTER_H

#include <game/server/entity.hpp>
#include <game/generated/gs_data.hpp>
#include <game/generated/g_protocol.hpp>

#include <game/gamecore.hpp>

class CHARACTER : public ENTITY
{
	/*static CHARACTER pool_data[MAX_CLIENTS];
	static int pool_used[MAX_CLIENTS];*/
public:
/*
	void operator delete(void *character)
	{
		(CHARACTER *)character 
		int id = (CHARACTER *)character - (CHARACTER *)pool_data;
		dbg_assert(pool_used[id], "");
		pool_used[id] = 0;
		mem_zero(&pool_data[id], sizeof(CHARACTER));
	}*/

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
	
	// info for dead reckoning
	int reckoning_tick; // tick that we are performing dead reckoning from
	CHARACTER_CORE sendcore; // core that we should send
	CHARACTER_CORE reckoningcore; // the dead reckoning core

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

#endif
