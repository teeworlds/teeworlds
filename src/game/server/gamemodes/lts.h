/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_LTS_H
#define GAME_SERVER_GAMEMODES_LTS_H
#include <game/server/gamecontroller.h>

class CGameControllerLTS : public IGameController
{
public:
	CGameControllerLTS(class CGameContext *pGameServer);
	
	// event
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	// game
	virtual void DoWincheckRound();
};

#endif
