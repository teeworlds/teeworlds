/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
// NOTE: Be very careful when editing this file as it will change the network version

#ifndef GAME_PROTOCOL_H
#define GAME_PROTOCOL_H

#include <game/generated/g_protocol.h>

// Network stuff
/*
enum
{
	OBJTYPE_NULL=0,
	OBJTYPE_GAME,
	OBJTYPE_PLAYER_INFO,
	OBJTYPE_PLAYER_CHARACTER, // use this if you are searching for the player entity
	OBJTYPE_PROJECTILE,
	OBJTYPE_LASER,
	OBJTYPE_POWERUP,
	OBJTYPE_FLAG,
	EVENT_EXPLOSION,
	EVENT_DAMAGEINDICATION,
	EVENT_SOUND_WORLD,
	EVENT_SMOKE,
	EVENT_PLAYERSPAWN,
	EVENT_DEATH,
	EVENT_AIR_JUMP,
	
	EVENT_DUMMY
};

enum
{
	MSG_NULL=0,
	MSG_SAY, // client -> server
	MSG_CHAT, // server -> client
	MSG_SETINFO, // server -> client - contains name, skin and color info
	MSG_KILLMSG, // server -> client
	MSG_SETTEAM,
	MSG_JOIN,
	MSG_QUIT,
	MSG_EMOTICON,
	MSG_STARTINFO, // client -> server
	MSG_CHANGEINFO, // client -> server
	MSG_READY_TO_ENTER, // server -> client
    MSG_WEAPON_PICKUP,
    MSG_SOUND_GLOBAL,
    MSG_TUNE_PARAMS,
	MSG_KILL,
    MSG_EXTRA_PROJECTILE, // server -> client
	
};

enum
{
	EMOTE_NORMAL=0,
	EMOTE_PAIN,
	EMOTE_HAPPY,
	EMOTE_SURPRISE,
	EMOTE_ANGRY,
	EMOTE_BLINK,
};

enum
{
	INPUT_STATE_MASK=0x1f,
};

enum
{
	PLAYERSTATE_UNKNOWN=0,
	PLAYERSTATE_PLAYING,
	PLAYERSTATE_IN_MENU,
	PLAYERSTATE_CHATTING,

	GAMETYPE_DM=0,
	GAMETYPE_TDM,
	GAMETYPE_CTF,
};

struct player_input
{
	int left;
	int right;

	int target_x;
	int target_y;

	int jump;
	int fire;
	int hook;
	int blink;

	int player_state;

	int wanted_weapon;
	int next_weapon;
	int prev_weapon;
};

struct ev_common
{
	int x, y;
};

struct ev_explosion : public ev_common
{
};

struct ev_spawn : public ev_common
{
};

struct ev_death : public ev_common
{
};

struct ev_sound : public ev_common
{
	int sound;
};

struct ev_damageind : public ev_common
{
	int angle;
};

struct obj_game
{
	int round_start_tick;
	int game_over;
	int sudden_death;
	int paused;

	int score_limit;
	int time_limit;
	int gametype;

	int warmup;

	int teamscore[2];
};

struct obj_projectile
{
	int x, y;
	int vx, vy; // should be an angle instead
	int type;
	int start_tick;
};

struct obj_laser
{
	int x, y;
	int from_x, from_y;
	int eval_tick;
};

struct obj_powerup
{
	int x, y;
	int type; // why do we need two types?
	int subtype;
};

struct obj_flag
{
	int x, y;
	int team;
	int carried_by; // is set if the local player has the flag
};

// core object needed for physics
struct obj_player_core
{
	int x, y;
	int vx, vy;
	int angle;
	int jumped;

	int hooked_player;
	int hook_state;
	int hook_tick;
	int hook_x, hook_y;
	int hook_dx, hook_dy;
};

// info about the player that is only needed when it's on screen
struct obj_player_character : public obj_player_core
{
	int player_state;

	int health;
	int armor;
	int ammocount;
	int weaponstage;

	int weapon; // current active weapon

	int emote;

	int attacktick; // num attack ticks left of current attack
};

// information about the player that is always needed
struct obj_player_info
{
	int local;
	int clientid;

	int team;
	int score;
	int latency;
	int latency_flux;
};*/

#endif
