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
	int m_OldClassNum; // set to old class on medic revive
public:
	CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer, CGameControllerMOD* pGameController, std::unordered_map<int, class IClass*> Classes);
	~CroyaPlayer();

	int GetClassNum();
	void SetClassNum(int Class, bool DrawPurpleThing = false);

	CCharacter* GetCharacter();
	void SetCharacter(CCharacter* pCharacter);

	void OnCharacterSpawn(CCharacter* pChr);
	void OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon);
	void OnKill(int Victim);

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon); // called in CCharacter::FireWeapon
	void OnButtonF3(); // called in CGameContext::OnMessage { ... else if(MsgID == NETMSGTYPE_CL_VOTE) }
	void OnMouseWheelDown(); // called in CCharacter::HandleWeaponSwitch
	void OnMouseWheelUp(); // called in CCharacter::HandleWeaponSwitch

	bool IsHuman() const;
	bool IsZombie() const;

	std::unordered_map<int, class IClass*>& GetClasses();

	void TurnIntoNextHumanClass();
	void TurnIntoPrevHumanClass();
	void TurnIntoRandomZombie();

	bool IsHookProtected() const;
	void SetHookProtected(bool HookProtected);

	int GetOldClassNum() const;
	void SetOldClassNum(int Class);

	const char* GetLanguage() const;
	void SetLanguage(const char* Language);

	IClass* GetClass();
	void SetClass(IClass* pClass, bool DrawPurpleThing = false);
};