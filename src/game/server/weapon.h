/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_WEAPON_H
#define GAME_SERVER_WEAPON_H

class CWeapon
{
private:
	/* Identity */
	class CGameContext *m_pGameServer;
	class CCharacter *m_pOwner;
	int m_WeaponID;
	bool m_AlwaysActive;
	bool m_Holdable;
	int m_Maxammo;

	/* State */
	int m_Ammo;
	int m_AmmoRegenStart;

protected:
	/* Getters */
	class CCharacter *GetOwner()		{ return m_pOwner; }

	/* Functions */
	virtual void OnFire(vec2 Direction, vec2 ProjStartPos) {}
	virtual void OnTick() {}
	virtual void OnTickPaused() {}
	void SendProjectiles(const class CProjectile **ppProjectiles, int NumProjectiles);
	void SendProjectile(const class CProjectile *pProjectile);

public:
	/* Constructor */
	CWeapon(class CCharacter *pOwner, int WeaponID, bool AlwaysActive, bool Holdable, int Ammo);

	/* Destructor */
	virtual ~CWeapon();

	/* Getters */
	class CGameContext *GameServer()	{ return m_pGameServer; }
	class IServer *Server();
	int GetWeaponID() const				{ return m_WeaponID; }
	bool IsAlwaysActive() const			{ return m_AlwaysActive; }
	bool IsHoldable() const				{ return m_Holdable; }
	int GetMaxammo() const				{ return m_Maxammo; }
	int GetAmmo() const					{ return m_Ammo; }

	/* Functions */
	bool IncreaseAmmo(int Ammo);
	void SetActive();
	void Fire(vec2 Direction, vec2 ProjStartPos);
	void Tick();
	void TickPaused();
	virtual int SnapAmmo();
};

#endif
