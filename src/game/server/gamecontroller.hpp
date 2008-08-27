#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.hpp>

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/
class GAMECONTROLLER
{
	vec2 spawn_points[3][64];
	int num_spawn_points[3];
protected:
	struct SPAWNEVAL
	{
		SPAWNEVAL()
		{
			got = false;
			friendly_team = -1;
			pos = vec2(100,100);
		}
			
		vec2 pos;
		bool got;
		int friendly_team;
		float score;
	};

	float evaluate_spawn_pos(SPAWNEVAL *eval, vec2 pos);
	void evaluate_spawn_type(SPAWNEVAL *eval, int type);
	bool evaluate_spawn(class PLAYER *p, vec2 *pos);

	void cyclemap();
	void resetgame();
	
	int round_start_tick;
	int game_over_tick;
	int sudden_death;
	
	int teamscore[2];
	
	int warmup;
	int round_count;
	
	
public:
	int gametype;
	bool is_teamplay;
	
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
	virtual void on_character_spawn(class CHARACTER *chr);
	
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

	//
	virtual bool can_spawn(class PLAYER *p, vec2 *pos);

	/*
	
	*/	
	virtual const char *get_team_name(int team);
	virtual int get_auto_team(int notthisid);
	virtual bool can_join_team(int team, int notthisid);
	int clampteam(int team);

	virtual void post_reset();
};

#endif
