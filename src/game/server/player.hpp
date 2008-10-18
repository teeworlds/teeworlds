#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.hpp"

// player object
class PLAYER
{
	MACRO_ALLOC_POOL_ID()
private:
	CHARACTER *character;
public:
	PLAYER(int client_id);
	~PLAYER();

	// TODO: clean this up
	char skin_name[64];
	int use_custom_color;
	int color_body;
	int color_feet;
	
	int respawn_tick;
	//
	bool spawning;
	int client_id;
	int team;
	int score;
	bool force_balanced;
	
	//
	int vote;
	int64 last_votecall;

	//
	int64 last_chat;
	int64 last_setteam;
	int64 last_changeinfo;
	int64 last_emote;
	int64 last_kill;

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
	
	// this is used for snapping so we know how we can clip the view for the player
	vec2 view_pos;

	void init(int client_id);
	
	CHARACTER *get_character();
	
	void kill_character();

	void try_respawn();
	void respawn();
	void set_team(int team);
	
	void tick();
	void snap(int snapping_client);

	void on_character_death();
	void on_direct_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_predicted_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_disconnect();
};

#endif
