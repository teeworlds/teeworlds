/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_TDM_H
#define GAME_SERVER_GAMEMODES_TDM_H
#include <game/server/gamecontroller.h>

class CGameControllerTDM : public IGameController
{
public:
	CGameControllerTDM(class CGameContext *pGameServer);

	// event
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
};

#endif
