#pragma once

#include <base/vmath.h>
#include <unordered_map>

class CroyaPlayer {
private:
	class IClass* m_pClass;
	class CPlayer* m_pPlayer;
	class CCharacter* m_pCharacter;
	class CGameContext* m_pGameServer;
	int m_ClientID;
	bool m_Infected;
	std::unordered_map<int, class IClass*> m_Classes;
public:
	CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer, std::unordered_map<int, class IClass*> Classes);
	~CroyaPlayer();
	int GetClassNum();
	void SetClassNum(int Class);
	IClass* GetClass();
	void SetClass(IClass* pClass);
	void SetCharacter(CCharacter* pCharacter);

	void OnCharacterSpawn(CCharacter* pChr);
	void OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon);

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon);

	bool IsHuman() const;
	bool IsZombie() const;

	std::unordered_map<int, class IClass*>& GetClasses();

	void SetNextHumanClass();
	void SetPrevHumanClass();

	void SetRandomZombieClass();
};