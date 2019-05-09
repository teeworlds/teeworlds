/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <generated/protocol.h>

#include <game/gamecore.h>
#include <game/server/entity.h>
#include <base/tl/array.h> // INFCROYA RELATED

// INFCROYA BEGIN ------------------------------------------------------------
enum
{
	FREEZEREASON_FLASH = 0,
	FREEZEREASON_UNDEAD = 1
};
// INFCROYA END ------------------------------------------------------------//

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
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void PostSnap();

	bool IsGrounded();

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	void HandleWeapons();
	void HandleNinja();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon);
	bool TakeDamage(vec2 Force, vec2 Source, int Dmg, int From, int Weapon);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);

	bool GiveWeapon(int Weapon, int Ammo);
	void GiveNinja();

	void SetEmote(int Emote, int Tick);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }

	// INFCROYA BEGIN ------------------------------------------------------------
	void SetNormalEmote(int Emote);

	bool IsHuman() const;
	bool IsZombie() const;

	void SetInfected(bool Infected);

	void SetCroyaPlayer(class CroyaPlayer* CroyaPlayer);
	class CroyaPlayer* GetCroyaPlayer();

	void ResetWeaponsHealth();

	int GetActiveWeapon() const;
	void SetReloadTimer(int ReloadTimer);
	void SetNumObjectsHit(int NumObjectsHit);
	void Infect(int From);
	bool IncreaseOverallHp(int Amount);
	int GetHealthArmorSum() const;
	void SetHealthArmor(int Health, int Armor);

	CCharacterCore& GetCharacterCore();
	bool m_FirstShot;
	vec2 m_FirstShotCoord;
	int m_BarrierHintID;
	array<int> m_BarrierHintIDs;

	void Freeze(float Time, int Player, int Reason);
	void Unfreeze();
	void Poison(int Count, int From);

	void DestroyChildEntities();

	bool IsHookProtected() const;
	void SetHookProtected(bool HookProtected);

	CNetObj_PlayerInput& GetInput();

	bool FindPortalPosition(vec2 Pos, vec2& Res);
	// INFCROYA END ------------------------------------------------------------//

private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	bool m_Alive;

	// INFCROYA BEGIN ------------------------------------------------------------
	bool m_Infected;
	class CroyaPlayer* m_pCroyaPlayer;
	int m_HeartID;
	int m_NormalEmote;
	bool m_IsFrozen;
	int m_FrozenTime;
	int m_FreezeReason;
	int m_LastFreezer;

	int m_Poison;
	int m_PoisonTick;
	int m_PoisonFrom;
	bool m_HookProtected;
	// INFCROYA END ------------------------------------------------------------//

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	// INFCROYA BEGIN ------------------------------------------------------------
public:
	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		bool m_Got;

	} m_aWeapons[NUM_WEAPONS];
private:
	// INFCROYA END ------------------------------------------------------------//

	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_QueuedWeapon;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_EmoteType;
	int m_EmoteStop;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_Health;
	int m_Armor;

	int m_TriggeredEvents;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

};

#endif
