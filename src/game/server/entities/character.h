#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>
#include <game/gamecore.h>

class CGameTeams;

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

enum 
{
	RACE_NONE = 0,
	RACE_STARTED,
	RACE_CHEAT // no time and won't start again unless ordered by a mod or death
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()
	
public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);
	
	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void Snap(int SnappingClient);
		
	bool IsGrounded();
	//bool m_Paused;

	CGameTeams* Teams();

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();
	
	void HandleWeapons();
	void HandleNinja();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void FireWeapon();

	void Die(int Killer, int Weapon);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);	

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();
	
	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);
	
	bool GiveWeapon(int Weapon, int Ammo);
	void GiveNinja();
	
	void ResetPos();
	
	bool Freeze(int Time);
	bool Freeze();
	bool UnFreeze();
	
	void GiveAllWeapons();  
	
	void SetEmote(int Emote, int Tick);
	
	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }
		// the player core for the physics	
	CCharacterCore m_Core;
	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		int m_Ammocost;
		bool m_Got;
		
	} m_aWeapons[NUM_WEAPONS];
	int m_ActiveWeapon;
	int m_LastWeapon;
	// player controlling this character
	class CPlayer *m_pPlayer;

	int m_RaceState;
	
	void OnFinish();
	int Team();
	
		struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		
	} m_Ninja;
	
	int m_HammerType;
	bool m_Super;

	//DDRace
	int m_FreezeTime;
	int m_FreezeTick;
	
	int m_Doored;

	vec2 m_OldPos;
	vec2 m_OlderPos;
	
	bool m_Alive;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;
	
	
	

	int m_QueuedWeapon;
	
	int m_ReloadTimer;
	int m_AttackTick;
	
	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;
	
	// last tick that the player took any action ie some input
	int m_LastAction;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input	
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;
	
	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;

	// ninja


	int m_PlayerState;// if the client is chatting, accessing a menu or so
	
	bool m_IsWater;
	bool m_DoSplash;
	int m_LastMove;
	// DDRace var
	int m_StartTime;
	int m_RefreshTime;
	
	int m_LastBooster;
	vec2 m_PrevPos;

	// checkpoints
	int m_CpTick;
	int m_CpActive;
	float m_CpCurrent[25];

	int m_BroadTime;
	int m_BroadCast;

	/*int m_CurrentTile;
	int m_CurrentFTile;*/
	int m_Stopped;
	enum
	{
		STOPPED_LEFT=1,
		STOPPED_RIGHT=2,
		STOPPED_BOTTOM=4,
		STOPPED_TOP=8
	};
	vec2 m_Intersection;
	bool m_EyeEmote;
	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core
};

#endif
