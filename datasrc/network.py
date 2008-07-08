from datatypes import *

Emotes = ["NORMAL", "PAIN", "HAPPY", "SURPRISE", "ANGRY", "BLINK"]
PlayerStates = ["UNKNOWN", "PLAYING", "IN_MENU", "CHATTING"]
GameTypes = ["DM", "TDM", "CTF"]

Emoticons = [str(x) for x in xrange(1,16)]

Powerups = ["ARMOR", "HEALTH", "WEAPON", "NINJA"]

RawHeader = '''
enum
{
	INPUT_STATE_MASK=0x2f,
};
'''

RawSource = '''
#include <engine/e_common_interface.h>
#include "g_protocol.hpp"
'''

Enums = [
	Enum("GAMETYPE", GameTypes),
	Enum("PLAYERSTATE", PlayerStates),
	Enum("EMOTE", Emotes),
	Enum("POWERUP", Powerups),
	Enum("EMOTICON", Emoticons)
]

Objects = [

	NetObject("Player_Input", [
		NetIntAny("direction"),
		NetIntAny("target_x"),
		NetIntAny("target_y"),
		
		NetIntAny("jump"),
		NetIntAny("fire"),
		NetIntAny("hook"),
		
		NetIntRange("player_state", 0, len(PlayerStates)),
		
		NetIntAny("wanted_weapon"),
		NetIntAny("next_weapon"),
		NetIntAny("prev_weapon"),
	]),
	
	NetObject("Projectile", [
		NetIntAny("x"),
		NetIntAny("y"),
		NetIntAny("vx"),
		NetIntAny("vy"),
		
		NetIntRange("type", 0, 'NUM_WEAPONS-1'),
		NetTick("start_tick"),
	]),

	NetObject("Laser", [
		NetIntAny("x"),
		NetIntAny("y"),
		NetIntAny("from_x"),
		NetIntAny("from_y"),
		
		NetTick("start_tick"),
	]),

	NetObject("Pickup", [
		NetIntAny("x"),
		NetIntAny("y"),
		
		NetIntRange("type", 0, 'max_int'),
		NetIntRange("subtype", 0, 'max_int'),
	]),

	NetObject("Flag", [
		NetIntAny("x"),
		NetIntAny("y"),
		
		NetIntRange("team", 0, 1),
		NetIntRange("carried_by", -2, 'MAX_CLIENTS-1')
	]),

	NetObject("Game", [
		NetIntRange("team", 0, 1),
		NetTick("round_start_tick"),
		
		NetIntRange("game_over", 0, 1),
		NetIntRange("sudden_death", 0, 1),
		NetIntRange("paused", 0, 1),
		
		NetIntRange("score_limit", 0, 'max_int'),
		NetIntRange("time_limit", 0, 'max_int'),
		NetIntRange("gametype", 0, len(GameTypes)),
		
		NetIntRange("warmup", 0, 'max_int'),

		NetIntAny("teamscore_red"),
		NetIntAny("teamscore_blue"),
	]),

	NetObject("Character_Core", [
		NetIntAny("x"),
		NetIntAny("y"),
		NetIntAny("vx"),
		NetIntAny("vy"),

		NetIntAny("angle"),
		
		NetIntRange("jumped", 0, 3),
		NetIntRange("hooked_player", 0, 'MAX_CLIENTS-1'),
		NetIntRange("hook_state", -1, 5),
		NetTick("hook_tick"),

		NetIntAny("hook_x"),
		NetIntAny("hook_y"),
		NetIntAny("hook_dx"),
		NetIntAny("hook_dy"),
	]),

	NetObject("Character:Character_Core", [
		NetIntRange("player_state", 0, 'NUM_PLAYERSTATES-1'),
		NetIntRange("wanted_direction", -1, 1),
		NetIntRange("health", 0, 10),
		NetIntRange("armor", 0, 10),
		NetIntRange("ammocount", 0, 10),
		NetIntRange("weapon", 0, 'NUM_WEAPONS-1'),
		NetIntRange("emote", 0, len(Emotes)),
		NetIntRange("attacktick", 0, 'max_int'),
	]),
	
	NetObject("Player_Info", [
		NetIntRange("local", 0, 1),
		NetIntRange("cid", 0, 'MAX_CLIENTS-1'),
		NetIntRange("team", -1, 1),
		
		NetIntAny("score"),
		NetIntAny("latency"),
		NetIntAny("latency_flux"),
	]),
	
	## Events
	
	NetEvent("Common", [
		NetIntAny("x"),
		NetIntAny("y"),
	]),
	

	NetEvent("Explosion:Common", []),
	NetEvent("Spawn:Common", []),
	
	NetEvent("Death:Common", [
		NetIntRange("cid", 0, 'MAX_CLIENTS-1'),
	]),
	
	NetEvent("AirJump:Common", []),

	NetEvent("SoundGlobal:Common", [
		NetIntRange("soundid", 0, 'NUM_SOUNDS-1'),
	]),

	NetEvent("SoundWorld:Common", [
		NetIntRange("soundid", 0, 'NUM_SOUNDS-1'),
	]),

	NetEvent("DamageInd:Common", [
		NetIntAny("angle"),
	]),
]

Messages = [

	### Server messages
	NetMessage("sv_motd", [
		NetString("message"),
	]),

	NetMessage("sv_broadcast", [
		NetString("message"),
	]),

	NetMessage("sv_chat", [
		NetIntRange("team", -1, 1),
		NetIntRange("cid", -1, 'MAX_CLIENTS-1'),
		NetString("message"),
	]),
	
	NetMessage("sv_setinfo", [
		NetIntRange("cid", 0, 'MAX_CLIENTS-1'),
		NetString("name"),
		NetString("skin"),
		NetBool("use_custom_color"),
		NetIntAny("color_body"),
		NetIntAny("color_feet"),
	]),
		
	NetMessage("sv_killmsg", [
		NetIntRange("killer", 0, 'MAX_CLIENTS-1'),
		NetIntRange("victim", 0, 'MAX_CLIENTS-1'),
		NetIntRange("weapon", 0, 'NUM_WEAPONS-1'),
		NetIntAny("mode_special"),
	]),

	NetMessage("sv_soundglobal", [
		NetIntRange("soundid", 0, 'NUM_SOUNDS-1'),
	]),
	
	NetMessage("sv_tuneparams", []),
	NetMessage("sv_extraprojectile", []),
	NetMessage("sv_readytoenter", []),

	NetMessage("sv_weaponpickup", [
		NetIntRange("weapon", 0, 'NUM_WEAPONS-1'),
	]),

	NetMessage("sv_emoticon", [
		NetIntRange("cid", 0, 'MAX_CLIENTS-1'),
		NetIntRange("emoticon", 0, 'NUM_EMOTICONS-1'),
	]),
	
	
	### Client messages
	NetMessage("cl_say", [
		NetBool("team"),
		NetString("message"),
	]),

	NetMessage("cl_setteam", [
		NetIntRange("team", -1, 1),
	]),
	
	NetMessage("cl_startinfo", [
		NetString("name"),
		NetString("skin"),
		NetBool("use_custom_color"),
		NetIntAny("color_body"),
		NetIntAny("color_feet"),
	]),	

	NetMessage("cl_changeinfo", [
		NetString("name"),
		NetString("skin"),
		NetBool("use_custom_color"),
		NetIntAny("color_body"),
		NetIntAny("color_feet"),
	]),
	
	NetMessage("cl_kill", []),

	NetMessage("cl_emoticon", [
		NetIntRange("emoticon", 0, 'NUM_EMOTICONS-1'),
	]),
	
]
