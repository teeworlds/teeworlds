from datatypes import *

Pickups = Enum("PICKUP", ["HEALTH", "ARMOR", "GRENADE", "SHOTGUN", "LASER", "NINJA"])
Emotes = Enum("EMOTE", ["NORMAL", "PAIN", "HAPPY", "SURPRISE", "ANGRY", "BLINK"])
Emoticons = Enum("EMOTICON", ["OOP", "EXCLAMATION", "HEARTS", "DROP", "DOTDOT", "MUSIC", "SORRY", "GHOST", "SUSHI", "SPLATTEE", "DEVILTEE", "ZOMG", "ZZZ", "WTF", "EYES", "QUESTION"])
Votes = Enum("VOTE", ["UNKNOWN", "START_OP", "START_KICK", "START_SPEC", "END_ABORT", "END_PASS", "END_FAIL"])

PlayerFlags = Flags("PLAYERFLAG", ["ADMIN", "CHATTING", "SCOREBOARD", "READY", "DEAD", "WATCHING"])
GameFlags = Flags("GAMEFLAG", ["TEAMS", "FLAGS", "SURVIVAL"])
GameStateFlags = Flags("GAMESTATEFLAG", ["WARMUP", "SUDDENDEATH", "ROUNDOVER", "GAMEOVER", "PAUSED", "STARTCOUNTDOWN"])
CoreEventFlags = Flags("COREEVENTFLAG", ["GROUND_JUMP", "AIR_JUMP", "HOOK_ATTACH_PLAYER", "HOOK_ATTACH_GROUND", "HOOK_HIT_NOHOOK"])

GameMsgIDs = Enum("GAMEMSG", ["TEAM_SWAP", "SPEC_INVALIDID", "TEAM_SHUFFLE", "TEAM_BALANCE", "CTF_DROP", "CTF_RETURN",
							
							"TEAM_ALL", "TEAM_BALANCE_VICTIM", "CTF_GRAB",
							
							"CTF_CAPTURE"])


RawHeader = '''

#include <engine/message.h>

enum
{
	INPUT_STATE_MASK=0x3f
};

enum
{
	TEAM_SPECTATORS=-1,
	TEAM_RED,
	TEAM_BLUE,
	NUM_TEAMS,

	FLAG_MISSING=-3,
	FLAG_ATSTAND,
	FLAG_TAKEN,

	SPEC_FREEVIEW=-1,
};
'''

RawSource = '''
#include <engine/message.h>
#include "protocol.h"
'''

Enums = [
	Pickups,
	Emotes,
	Emoticons,
	Votes,
	GameMsgIDs,
]

Flags = [
	PlayerFlags,
	GameFlags,
	GameStateFlags,
	CoreEventFlags,
]

Objects = [

	NetObject("PlayerInput", [
		NetIntRange("m_Direction", -1, 1),
		NetIntAny("m_TargetX"),
		NetIntAny("m_TargetY"),

		NetBool("m_Jump"),
		NetIntAny("m_Fire"),
		NetBool("m_Hook"),

		NetFlag("m_PlayerFlags", PlayerFlags),

		NetIntRange("m_WantedWeapon", 0, 'NUM_WEAPONS-1'),
		NetIntAny("m_NextWeapon"),
		NetIntAny("m_PrevWeapon"),
	]),

	NetObject("Projectile", [
		NetIntAny("m_X"),
		NetIntAny("m_Y"),
		NetIntAny("m_VelX"),
		NetIntAny("m_VelY"),

		NetIntRange("m_Type", 0, 'NUM_WEAPONS-1'),
		NetTick("m_StartTick"),
	]),

	NetObject("Laser", [
		NetIntAny("m_X"),
		NetIntAny("m_Y"),
		NetIntAny("m_FromX"),
		NetIntAny("m_FromY"),

		NetTick("m_StartTick"),
	]),

	NetObject("Pickup", [
		NetIntAny("m_X"),
		NetIntAny("m_Y"),

		NetEnum("m_Type", Pickups),
	]),

	NetObject("Flag", [
		NetIntAny("m_X"),
		NetIntAny("m_Y"),

		NetIntRange("m_Team", 'TEAM_RED', 'TEAM_BLUE')
	]),

	NetObject("GameData", [
		NetTick("m_GameStartTick"),
		NetFlag("m_GameStateFlags", GameStateFlags),
		NetTick("m_GameStateEndTick"),
	]),

	NetObject("GameDataTeam", [
		NetIntAny("m_TeamscoreRed"),
		NetIntAny("m_TeamscoreBlue"),
	]),

	NetObject("GameDataFlag", [
		NetIntRange("m_FlagCarrierRed", 'FLAG_MISSING', 'MAX_CLIENTS-1'),
		NetIntRange("m_FlagCarrierBlue", 'FLAG_MISSING', 'MAX_CLIENTS-1'),
		NetTick("m_FlagDropTickRed"),
		NetTick("m_FlagDropTickBlue"),
	]),

	NetObject("CharacterCore", [
		NetTick("m_Tick"),
		NetIntAny("m_X"),
		NetIntAny("m_Y"),
		NetIntAny("m_VelX"),
		NetIntAny("m_VelY"),

		NetIntAny("m_Angle"),
		NetIntRange("m_Direction", -1, 1),

		NetIntRange("m_Jumped", 0, 3),
		NetIntRange("m_HookedPlayer", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_HookState", -1, 5),
		NetTick("m_HookTick"),

		NetIntAny("m_HookX"),
		NetIntAny("m_HookY"),
	]),

	NetObject("Character:CharacterCore", [
		NetIntRange("m_Health", 0, 10),
		NetIntRange("m_Armor", 0, 10),
		NetIntAny("m_AmmoCount"),
		NetIntRange("m_Weapon", 0, 'NUM_WEAPONS-1'),
		NetEnum("m_Emote", Emotes),
		NetTick("m_AttackTick"),
		NetFlag("m_TriggeredEvents", CoreEventFlags),
	]),

	NetObject("PlayerInfo", [
		NetFlag("m_PlayerFlags", PlayerFlags),
		NetIntAny("m_Score"),
		NetIntAny("m_Latency"),
	]),

	NetObject("SpectatorInfo", [
		NetIntRange("m_SpectatorID", 'SPEC_FREEVIEW', 'MAX_CLIENTS-1'),
		NetIntAny("m_X"),
		NetIntAny("m_Y"),
	]),

	## Demo

	NetObject("De_ClientInfo", [
		NetBool("m_Local"),
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),

		NetArray(NetIntAny("m_aName"), 4),
		NetArray(NetIntAny("m_aClan"), 3),

		NetIntAny("m_Country"),

		NetArray(NetArray(NetIntAny("m_aaSkinPartNames"), 6), 6),
		NetArray(NetBool("m_aUseCustomColors"), 6),
		NetArray(NetIntAny("m_aSkinPartColors"), 6),
	]),

	NetObject("De_GameInfo", [
		NetFlag("m_GameFlags", GameFlags),
		
		NetIntRange("m_ScoreLimit", 0, 'max_int'),
		NetIntRange("m_TimeLimit", 0, 'max_int'),

		NetIntRange("m_MatchNum", 0, 'max_int'),
		NetIntRange("m_MatchCurrent", 0, 'max_int'),
	]),

	NetObject("De_TuneParams", [
		# todo: should be done differently
		NetArray(NetIntAny("m_aTuneParams"), 33),
	]),

	## Events

	NetEvent("Common", [
		NetIntAny("m_X"),
		NetIntAny("m_Y"),
	]),


	NetEvent("Explosion:Common", []),
	NetEvent("Spawn:Common", []),
	NetEvent("HammerHit:Common", []),

	NetEvent("Death:Common", [
		NetIntRange("m_ClientID", 0, 'MAX_CLIENTS-1'),
	]),

	NetEvent("SoundWorld:Common", [
		NetIntRange("m_SoundID", 0, 'NUM_SOUNDS-1'),
	]),

	NetEvent("DamageInd:Common", [
		NetIntAny("m_Angle"),
	]),
]

Messages = [

	### Server messages
	NetMessage("Sv_Motd", [
		NetString("m_pMessage"),
	]),

	NetMessage("Sv_Chat", [
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),
		NetIntRange("m_ClientID", -1, 'MAX_CLIENTS-1'),
		NetStringStrict("m_pMessage"),
	]),

	NetMessage("Sv_Team", [
		NetIntRange("m_ClientID", -1, 'MAX_CLIENTS-1'),
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),
		NetBool("m_Silent"),
		NetTick("m_CooldownTick"),
	]),

	NetMessage("Sv_KillMsg", [
		NetIntRange("m_Killer", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_Victim", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_Weapon", -3, 'NUM_WEAPONS-1'),
		NetIntAny("m_ModeSpecial"),
	]),

	NetMessage("Sv_TuneParams", []),
	NetMessage("Sv_ExtraProjectile", []),
	NetMessage("Sv_ReadyToEnter", []),

	NetMessage("Sv_WeaponPickup", [
		NetIntRange("m_Weapon", 0, 'NUM_WEAPONS-1'),
	]),

	NetMessage("Sv_Emoticon", [
		NetIntRange("m_ClientID", 0, 'MAX_CLIENTS-1'),
		NetEnum("m_Emoticon", Emoticons),
	]),

	NetMessage("Sv_VoteClearOptions", []),

	NetMessage("Sv_VoteOptionListAdd", []),

	NetMessage("Sv_VoteOptionAdd", [
		NetStringStrict("m_pDescription"),
	]),

	NetMessage("Sv_VoteOptionRemove", [
		NetStringStrict("m_pDescription"),
	]),

	NetMessage("Sv_VoteSet", [
		NetIntRange("m_ClientID", -1, 'MAX_CLIENTS-1'),
		NetEnum("m_Type", Votes),
		NetIntRange("m_Timeout", 0, 60),
		NetStringStrict("m_pDescription"),
		NetStringStrict("m_pReason"),
	]),

	NetMessage("Sv_VoteStatus", [
		NetIntRange("m_Yes", 0, 'MAX_CLIENTS'),
		NetIntRange("m_No", 0, 'MAX_CLIENTS'),
		NetIntRange("m_Pass", 0, 'MAX_CLIENTS'),
		NetIntRange("m_Total", 0, 'MAX_CLIENTS'),
	]),

	NetMessage("Sv_ServerSettings", [
		NetBool("m_KickVote"),
		NetIntRange("m_KickMin", 0, 'MAX_CLIENTS'),
		NetBool("m_SpecVote"),
		NetBool("m_TeamLock"),
		NetBool("m_TeamBalance"),
		NetIntRange("m_PlayerSlots", 0, 'MAX_CLIENTS'),
	]),

	NetMessage("Sv_ClientInfo", [
		NetIntRange("m_ClientID", 0, 'MAX_CLIENTS-1'),
		NetBool("m_Local"),
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),
		NetStringStrict("m_pName"),
		NetStringStrict("m_pClan"),
		NetIntAny("m_Country"),
		NetArray(NetStringStrict("m_apSkinPartNames"), 6),
		NetArray(NetBool("m_aUseCustomColors"), 6),
		NetArray(NetIntAny("m_aSkinPartColors"), 6),
	]),

	NetMessage("Sv_GameInfo", [
		NetFlag("m_GameFlags", GameFlags),
		
		NetIntRange("m_ScoreLimit", 0, 'max_int'),
		NetIntRange("m_TimeLimit", 0, 'max_int'),

		NetIntRange("m_MatchNum", 0, 'max_int'),
		NetIntRange("m_MatchCurrent", 0, 'max_int'),
	]),

	NetMessage("Sv_ClientDrop", [
		NetIntRange("m_ClientID", 0, 'MAX_CLIENTS-1'),
		NetStringStrict("m_pReason"),
	]),

	NetMessage("Sv_GameMsg", []),

	## Demo messages
	NetMessage("De_ClientEnter", [
		NetStringStrict("m_pName"),
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),
	]),

	NetMessage("De_ClientLeave", [
		NetStringStrict("m_pName"),
		NetStringStrict("m_pReason"),
	]),

	### Client messages
	NetMessage("Cl_Say", [
		NetBool("m_Team"),
		NetStringStrict("m_pMessage"),
	]),

	NetMessage("Cl_SetTeam", [
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),
	]),

	NetMessage("Cl_SetSpectatorMode", [
		NetIntRange("m_SpectatorID", 'SPEC_FREEVIEW', 'MAX_CLIENTS-1'),
	]),

	NetMessage("Cl_StartInfo", [
		NetStringStrict("m_pName"),
		NetStringStrict("m_pClan"),
		NetIntAny("m_Country"),
		NetArray(NetStringStrict("m_apSkinPartNames"), 6),
		NetArray(NetBool("m_aUseCustomColors"), 6),
		NetArray(NetIntAny("m_aSkinPartColors"), 6),
	]),

	NetMessage("Cl_Kill", []),

	NetMessage("Cl_ReadyChange", []),

	NetMessage("Cl_Emoticon", [
		NetEnum("m_Emoticon", Emoticons),
	]),

	NetMessage("Cl_Vote", [
		NetIntRange("m_Vote", -1, 1),
	]),

	NetMessage("Cl_CallVote", [
		NetStringStrict("m_Type"),
		NetStringStrict("m_Value"),
		NetStringStrict("m_Reason"),
		NetBool("m_Force"),
	]),
]
