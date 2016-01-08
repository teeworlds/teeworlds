#ifndef MOD_GAMEMODES_MOD_H
#define MOD_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>

class CGameControllerMOD : public IGameController
{
public:
	CGameControllerMOD(class CGameContext *pGameServer);
};
#endif
