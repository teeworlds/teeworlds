
#include "eventhandler.hpp"
#include "gamecontroller.hpp"
#include "gameworld.hpp"

class GAMECONTEXT
{
public:
	GAMECONTEXT();
	void clear();
	
	EVENTHANDLER events;
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
