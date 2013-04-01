from datatypes import *

Emotes = ["NORMAL", "PAIN", "HAPPY", "SURPRISE", "ANGRY", "BLINK"]
PlayerFlags = ["PLAYING", "IN_MENU", "CHATTING", "SCOREBOARD"]
GameFlags = ["TEAMS", "FLAGS"]
GameStateFlags = ["GAMEOVER", "SUDDENDEATH", "PAUSED"]

Emoticons = ["OOP", "EXCLAMATION", "HEARTS", "DROP", "DOTDOT", "MUSIC", "SORRY", "GHOST", "SUSHI", "SPLATTEE", "DEVILTEE", "ZOMG", "ZZZ", "WTF", "EYES", "QUESTION"]

Powerups = ["HEALTH", "ARMOR", "WEAPON", "NINJA"]

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
	Enum("EMOTE", Emotes),
	Enum("POWERUP", Powerups),
	Enum("EMOTICON", Emoticons)
]

Flags = [
	Flags("PLAYERFLAG", PlayerFlags),
	Flags("GAMEFLAG", GameFlags),
	Flags("GAMESTATEFLAG", GameStateFlags)
]

Objects = [

	NetObject("PlayerInput", [
		NetIntAny("m_Direction"),
		NetIntAny("m_TargetX"),
		NetIntAny("m_TargetY"),

		NetIntAny("m_Jump"),
		NetIntAny("m_Fire"),
		NetIntAny("m_Hook"),

		NetIntRange("m_PlayerFlags", 0, 256),

		NetIntAny("m_WantedWeapon"),
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

		NetIntRange("m_Type", 0, 'max_int'),
		NetIntRange("m_Subtype", 0, 'max_int'),
	]),

	NetObject("Flag", [
		NetIntAny("m_X"),
		NetIntAny("m_Y"),

		NetIntRange("m_Team", 'TEAM_RED', 'TEAM_BLUE')
	]),

	NetObject("GameInfo", [
		NetIntRange("m_GameFlags", 0, 256),
		NetIntRange("m_GameStateFlags", 0, 256),
		NetTick("m_RoundStartTick"),
		NetIntRange("m_WarmupTimer", 0, 'max_int'),

		NetIntRange("m_ScoreLimit", 0, 'max_int'),
		NetIntRange("m_TimeLimit", 0, 'max_int'),

		NetIntRange("m_RoundNum", 0, 'max_int'),
		NetIntRange("m_RoundCurrent", 0, 'max_int'),
	]),

	NetObject("GameData", [
		NetIntAny("m_TeamscoreRed"),
		NetIntAny("m_TeamscoreBlue"),

		NetIntRange("m_FlagCarrierRed", 'FLAG_MISSING', 'MAX_CLIENTS-1'),
		NetIntRange("m_FlagCarrierBlue", 'FLAG_MISSING', 'MAX_CLIENTS-1'),
	]),

	NetObject("CharacterCore", [
		NetIntAny("m_Tick"),
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
		NetIntAny("m_HookDx"),
		NetIntAny("m_HookDy"),
	]),

	NetObject("Character:CharacterCore", [
		NetIntRange("m_PlayerFlags", 0, 256),
		NetIntRange("m_Health", 0, 10),
		NetIntRange("m_Armor", 0, 10),
		NetIntRange("m_AmmoCount", 0, 10),
		NetIntRange("m_Weapon", 0, 'NUM_WEAPONS-1'),
		NetIntRange("m_Emote", 0, len(Emotes)),
		NetIntRange("m_AttackTick", 0, 'max_int'),
	]),

	NetObject("PlayerInfo", [
		NetIntRange("m_Local", 0, 1),
		NetIntRange("m_ClientID", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),

		NetIntAny("m_Score"),
		NetIntAny("m_Latency"),
	]),

	NetObject("ClientInfo", [
		# 4*4 = 16 charachters
		NetIntAny("m_Name0"), NetIntAny("m_Name1"), NetIntAny("m_Name2"),
		NetIntAny("m_Name3"),

		# 4*3 = 12 charachters
		NetIntAny("m_Clan0"), NetIntAny("m_Clan1"), NetIntAny("m_Clan2"),

		NetIntAny("m_Country"),

		# 4*6 = 24 charachters
		NetIntAny("m_Skin0"), NetIntAny("m_Skin1"), NetIntAny("m_Skin2"),
		NetIntAny("m_Skin3"), NetIntAny("m_Skin4"), NetIntAny("m_Skin5"),

		NetIntRange("m_UseCustomColor", 0, 1),

		NetIntAny("m_ColorBody"),
		NetIntAny("m_ColorFeet"),
	]),

	NetObject("SpectatorInfo", [
		NetIntRange("m_SpectatorID", 'SPEC_FREEVIEW', 'MAX_CLIENTS-1'),
		NetIntAny("m_X"),
		NetIntAny("m_Y"),
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

	NetEvent("SoundGlobal:Common", [ #TODO 0.7: remove me
		NetIntRange("m_SoundID", 0, 'NUM_SOUNDS-1'),
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

	NetMessage("Sv_Broadcast", [
		NetString("m_pMessage"),
	]),

	NetMessage("Sv_Chat", [
		NetIntRange("m_Team", 'TEAM_SPECTATORS', 'TEAM_BLUE'),
		NetIntRange("m_ClientID", -1, 'MAX_CLIENTS-1'),
		NetStringStrict("m_pMessage"),
	]),

	NetMessage("Sv_KillMsg", [
		NetIntRange("m_Killer", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_Victim", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_Weapon", -3, 'NUM_WEAPONS-1'),
		NetIntAny("m_ModeSpecial"),
	]),

	NetMessage("Sv_SoundGlobal", [
		NetIntRange("m_SoundID", 0, 'NUM_SOUNDS-1'),
	]),

	NetMessage("Sv_TuneParams", []),
	NetMessage("Sv_ExtraProjectile", []),
	NetMessage("Sv_ReadyToEnter", []),

	NetMessage("Sv_WeaponPickup", [
		NetIntRange("m_Weapon", 0, 'NUM_WEAPONS-1'),
	]),

	NetMessage("Sv_Emoticon", [
		NetIntRange("m_ClientID", 0, 'MAX_CLIENTS-1'),
		NetIntRange("m_Emoticon", 0, 'NUM_EMOTICONS-1'),
	]),

	NetMessage("Sv_VoteClearOptions", [
	]),

	NetMessage("Sv_VoteOptionListAdd", [
		NetIntRange("m_NumOptions", 1, 15),
		NetStringStrict("m_pDescription0"), NetStringStrict("m_pDescription1"),	NetStringStrict("m_pDescription2"),
		NetStringStrict("m_pDescription3"),	NetStringStrict("m_pDescription4"),	NetStringStrict("m_pDescription5"),
		NetStringStrict("m_pDescription6"), NetStringStrict("m_pDescription7"), NetStringStrict("m_pDescription8"),
		NetStringStrict("m_pDescription9"), NetStringStrict("m_pDescription10"), NetStringStrict("m_pDescription11"),
		NetStringStrict("m_pDescription12"), NetStringStrict("m_pDescription13"), NetStringStrict("m_pDescription14"),
	]),

	NetMessage("Sv_VoteOptionAdd", [
		NetStringStrict("m_pDescription"),
	]),

	NetMessage("Sv_VoteOptionRemove", [
		NetStringStrict("m_pDescription"),
	]),

	NetMessage("Sv_VoteSet", [
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
		NetStringStrict("m_pSkin"),
		NetBool("m_UseCustomColor"),
		NetIntAny("m_ColorBody"),
		NetIntAny("m_ColorFeet"),
	]),

	NetMessage("Cl_ChangeInfo", [
		NetStringStrict("m_pName"),
		NetStringStrict("m_pClan"),
		NetIntAny("m_Country"),
		NetStringStrict("m_pSkin"),
		NetBool("m_UseCustomColor"),
		NetIntAny("m_ColorBody"),
		NetIntAny("m_ColorFeet"),
	]),

	NetMessage("Cl_Kill", []),

	NetMessage("Cl_Emoticon", [
		NetIntRange("m_Emoticon", 0, 'NUM_EMOTICONS-1'),
	]),

	NetMessage("Cl_Vote", [
		NetIntRange("m_Vote", -1, 1),
	]),

	NetMessage("Cl_CallVote", [
		NetStringStrict("m_Type"),
		NetStringStrict("m_Value"),
		NetStringStrict("m_Reason"),
	]),
]
