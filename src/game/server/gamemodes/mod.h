/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H
#include <game/server/gamecontroller.h>
#include <array>
#include <unordered_map>
#include <infcroya/croyaplayer.h>

// you can subclass GAMECONTROLLER_CTF, GAMECONTROLLER_TDM etc if you want
// todo a modification with their base as well.
class CGameControllerMOD : public IGameController
{
private:
	std::array<CroyaPlayer*, 64> players{};
	std::unordered_map<int, class IClass*> classes;
	class Geolocation* geolocation;
public:
	CGameControllerMOD(class CGameContext *pGameServer);
	~CGameControllerMOD() override;
	void Snap(int SnappingClient) override;
	virtual void Tick();
	
	void OnCharacterSpawn(class CCharacter* pChr) override;
	int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) override;

	bool IsFriendlyFire(int ClientID1, int ClientID2) const override;

	bool IsEveryoneInfected() const;

	void OnPlayerDisconnect(class CPlayer* pPlayer) override;
	void OnPlayerConnect(class CPlayer* pPlayer) override;

	void SetLanguageByCountry(int Country, int ClientID);
};
#endif
