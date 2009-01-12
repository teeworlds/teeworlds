#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include "eventhandler.hpp"
#include "gamecontroller.hpp"
#include "gameworld.hpp"
#include "player.hpp"

/*
	Tick
		Game Context (GAMECONTEXT::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (PLAYER::tick)
			

	Snap
		Game Context (GAMECONTEXT::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (PLAYER::snap)

*/
class GAMECONTEXT
{
public:
	GAMECONTEXT();
	~GAMECONTEXT();
	
	void clear();
	
	EVENTHANDLER events;
	PLAYER *players[MAX_CLIENTS];
	
	GAMECONTROLLER *controller;
	GAMEWORLD world;

	void tick();
	void snap(int client_id);
	
	// helper functions
	class CHARACTER *get_player_char(int client_id);
	
	// voting
	void start_vote(const char *desc, const char *command);
	void end_vote();
	void send_vote_set(int cid);
	void send_vote_status(int cid);
	void abort_vote_kick_on_disconnect(int client_id);
	int vote_creator;
	int64 vote_closetime;
	char vote_description[512];
	char vote_command[512];
	int vote_enforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};

	// helper functions
	void create_damageind(vec2 p, float angle_mod, int amount);
	void create_explosion(vec2 p, int owner, int weapon, bool bnodamage);
	void create_smoke(vec2 p);
	void create_hammerhit(vec2 p);
	void create_playerspawn(vec2 p);
	void create_death(vec2 p, int who);
	void create_sound(vec2 pos, int sound, int mask=-1);
	void create_sound_global(int sound, int target=-1);	


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void send_chat_target(int to, const char *text);
	void send_chat(int cid, int team, const char *text);
	void send_emoticon(int cid, int emoticon);
	void send_weapon_pickup(int cid, int weapon);
	void send_broadcast(const char *text, int cid);

};

extern GAMECONTEXT game;

// MISC stuff, move to a better place later on

extern TUNING_PARAMS tuning;
inline int cmask_all() { return -1; }
inline int cmask_one(int cid) { return 1<<cid; }
inline int cmask_all_except_one(int cid) { return 0x7fffffff^cmask_one(cid); }
inline bool cmask_is_set(int mask, int cid) { return (mask&cmask_one(cid)) != 0; }
#endif
