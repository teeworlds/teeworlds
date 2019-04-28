#pragma once

#include <base/vmath.h>

class CroyaPlayer {
private:
	class IClass* m_pClass;
	class CPlayer* m_pPlayer;
	class CCharacter* m_pCharacter;
	class CGameContext* m_pGameServer;
	int m_ClientID;
	bool m_Infected;
public:
	CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer);
	~CroyaPlayer();
	IClass* GetClass();
	void SetClass(IClass* pClass);
	void SetCharacter(CCharacter* pCharacter);

	void OnCharacterSpawn(CCharacter* pChr);
	void OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon);

	void OnWeaponLaser(vec2 Direction);

	bool IsHuman() const;
	bool IsZombie() const;
};