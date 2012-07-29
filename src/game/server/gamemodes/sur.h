/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_SUR_H
#define GAME_SERVER_GAMEMODES_SUR_H
#include <game/server/gamecontroller.h>

class CGameControllerSUR : public IGameController
{
public:
	CGameControllerSUR(class CGameContext *pGameServer);

	// game
	virtual void DoWincheckRound();
};

#endif
