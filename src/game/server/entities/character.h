/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <generated/protocol.h>

#include <game/gamecore.h>
#include <game/server/entity.h>
#include <game/server/weapon.h>


class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

private:
	/* Types */
	struct CInputCount
	{
		int m_Presses;
		int m_Releases;
	};

	/* Objects */
	class CPlayer *m_pPlayer;

	/* State (status) */
	bool m_Alive;
	int m_Health;
	int m_Armor;

	/* State (misc) */
	int m_DamageTaken;
	int m_DamageTakenTick;
	int m_EmoteType;
	int m_EmoteStopTick;

	/* State (weapons) */
	CWeapon *m_apWeapons[NUM_WEAPONS];
	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_QueuedWeapon;
	int m_ReloadTimer;
	int m_AttackTick;
	int m_LastNoAmmoSound;

	/* State (input) */
	int m_LastAction; // last tick that the player took any action ie some input
	CNetObj_PlayerInput m_LatestPrevInput; // non-heldback input
	CNetObj_PlayerInput m_LatestInput; // non-heldback input
	CNetObj_PlayerInput m_Input;
	int m_InputsCount;

	/* State (core) */
	CCharacterCore m_Core; // the character core for the physics
	int m_TriggeredEvents;

	/* State (dead reckoning) */
	int m_ReckoningTick; // tick that we are performing dead reckoning from
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

	/* Functions (weapons) */
	CWeapon *GetActiveWeapon();
	void DoWeaponSwitch();
	void HandleWeaponSwitch();
	void FireWeapon();

public:
	/* Constants */
	static const int ms_PhysSize = 28;

	/* Constructor */
	CCharacter(CGameWorld *pWorld, class CPlayer *pPlayer);

	/* Destructor */
	~CCharacter();

	/* Getters */
	class CPlayer *GetPlayer()	{ return m_pPlayer; }
	CCharacterCore *GetCore()	{ return &m_Core; }
	bool IsAlive() const		{ return m_Alive; }
	bool IsReloading() const	{ return m_ReloadTimer > 0; }

	/* CEntity functions */
	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void PostSnap();

	/* Functions (status) */
	void Spawn(vec2 Pos);
	void Die(int Killer, int Weapon);
	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);

	/* Functions (misc) */
	void SetEmote(int Emote, int DurationTicks);

	/* Functions (weapons) */
	bool GotWeapon(int Weapon) const;
	bool GiveWeapon(int Weapon, int Ammo);
	void RemoveWeapon(int Weapon);
	bool SetWeapon(int Weapon);
	void SetReloadTimer(int ReloadTimer);

	/* Functions (input) */
	static CInputCount CountInput(int Prev, int Cur);
	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
};

#endif
