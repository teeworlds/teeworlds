#ifndef GAME_SERVER_GAMEMODES_TDM_H
#define GAME_SERVER_GAMEMODES_TDM_H
#include <game/server/gamecontroller.h>

class CGameControllerTDM : public IGameController
{
public:
	CGameControllerTDM(class CGameContext *pGameServer);
	
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual void Tick();
};
#endif
