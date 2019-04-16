#pragma once

class CroyaPlayer {
private:
	class IClass* m_pClass;
	class CPlayer* m_pPlayer;
	class CCharacter* m_pCharacter;
	int m_ClientID;
	bool m_Infected;
public:
	CroyaPlayer(int ClientID, CPlayer* pPlayer);
	~CroyaPlayer();
	void SetClass(IClass* pClass);
	void SetCharacter(CCharacter* pCharacter);

	void OnCharacterSpawn();

	bool IsHuman() const;
	bool IsZombie() const;
};