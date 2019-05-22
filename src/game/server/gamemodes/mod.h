/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H
#include <game/server/gamecontroller.h>
#include <array>
#include <vector>
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
	class CGameWorld* m_pGameWorld;
	std::vector<int> humans;
	class LuaLoader* lua;
	std::vector<class CCircle*> circles;
	std::vector<class CInfCircle*> inf_circles;

	bool m_ExplosionStarted;
	bool m_InfectedStarted;
	int m_MapWidth;
	int m_MapHeight;
	int* m_GrowingMap;
public:
	CGameControllerMOD(class CGameContext *pGameServer);
	~CGameControllerMOD() override;
	void Snap(int SnappingClient) override;
	void Tick() override;

	bool IsCroyaWarmup(); // First 10 secs after game start
	bool RoundJustStarted();

	void OnRoundStart();
	void StartInitialInfection();
	void TurnDefaultIntoRandomHuman();

	void OnRoundEnd();
	bool DoWincheckMatch() override;
	void DoWincheckRound() override;
	
	void OnCharacterSpawn(class CCharacter* pChr) override;
	int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) override;

	bool IsFriendlyFire(int ClientID1, int ClientID2) const override;

	int GetZombieCount() const;
	int GetHumanCount() const;
	bool IsEveryoneInfected() const;

	void OnPlayerDisconnect(class CPlayer* pPlayer) override;
	void OnPlayerConnect(class CPlayer* pPlayer) override;

	std::array<CroyaPlayer*, 64> GetCroyaPlayers();
	void ResetFinalExplosion();

	bool IsExplosionStarted() const;

	std::vector<class CCircle*>& GetCircles();
	std::vector<class CInfCircle*>& GetInfCircles();

	void SetLanguageByCountry(int Country, int ClientID);
};

enum // infclass 0.6 copypaste
{
	INFWEAPON_NONE,
	INFWEAPON_HAMMER,
	INFWEAPON_GUN,
	INFWEAPON_SHOTGUN,
	INFWEAPON_GRENADE,
	INFWEAPON_RIFLE,
	INFWEAPON_NINJA,

	INFWEAPON_ENGINEER_RIFLE,

	INFWEAPON_SNIPER_RIFLE,

	INFWEAPON_SOLDIER_GRENADE,

	INFWEAPON_SCIENTIST_GRENADE,
	INFWEAPON_SCIENTIST_RIFLE,

	INFWEAPON_MEDIC_GRENADE,
	INFWEAPON_MEDIC_RIFLE,
	INFWEAPON_MEDIC_SHOTGUN,

	INFWEAPON_HERO_GRENADE,
	INFWEAPON_HERO_RIFLE,
	INFWEAPON_HERO_SHOTGUN,

	INFWEAPON_BIOLOGIST_SHOTGUN,
	INFWEAPON_BIOLOGIST_RIFLE,

	INFWEAPON_LOOPER_RIFLE,
	INFWEAPON_LOOPER_GRENADE,

	INFWEAPON_NINJA_HAMMER,
	INFWEAPON_NINJA_GRENADE,

	INFWEAPON_MERCENARY_GUN,
	INFWEAPON_MERCENARY_GRENADE,

	NB_INFWEAPON
};
#endif
