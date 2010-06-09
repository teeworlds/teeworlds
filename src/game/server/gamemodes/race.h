/* copyright (c) 2007 rajh and gregwar. Score stuff */
#ifndef GAME_SERVER_GAMEMODES_RACE_H
#define GAME_SERVER_GAMEMODES_RACE_H

#include <game/server/gamecontroller.h>

class CGameControllerRACE : public IGameController
{
public:
	
	CGameControllerRACE(class CGameContext *pGameServer);
	~CGameControllerRACE();
	
	vec2 *m_pTeleporter;
	
	void InitTeleporter();
	
	virtual void Tick();
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
};

#endif
