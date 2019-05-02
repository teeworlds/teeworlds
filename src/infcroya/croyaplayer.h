#pragma once

#include <base/vmath.h>
#include <unordered_map>

class CroyaPlayer {
private:
	class IClass* m_pClass;
	class CPlayer* m_pPlayer;
	class CCharacter* m_pCharacter;
	class CGameContext* m_pGameServer;
	class CGameControllerMOD* m_pGameController;
	int m_ClientID;
	bool m_Infected;
	bool m_HookProtected;
	std::unordered_map<int, class IClass*> m_Classes;
	std::string m_Language;
public:
	CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer, CGameControllerMOD* pGameController, std::unordered_map<int, class IClass*> Classes);
	~CroyaPlayer();
	int GetClassNum();
	void SetClassNum(int Class, bool DrawPurpleThing = false);
	IClass* GetClass();
	void SetClass(IClass* pClass, bool DrawPurpleThing = false);
	void SetCharacter(CCharacter* pCharacter);

	void OnCharacterSpawn(CCharacter* pChr);
	void OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon);
	void OnKill(int Killer);

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon);
	void OnButtonF3();

	bool IsHuman() const;
	bool IsZombie() const;

	std::unordered_map<int, class IClass*>& GetClasses();

	void SetNextHumanClass();
	void SetPrevHumanClass();

	void TurnIntoRandomZombie();

	bool IsHookProtected() const;
	void SetHookProtected(bool HookProtected);

	const char* GetLanguage() const;
	void SetLanguage(const char* Language);
};