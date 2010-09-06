#ifndef GAME_SERVER_GAMEMODES_CTF_H
#define GAME_SERVER_GAMEMODES_CTF_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>

class CGameControllerCTF : public IGameController
{
public:
	class CFlag *m_apFlags[2];
	
	CGameControllerCTF(class CGameContext *pGameServer);
	virtual bool CanBeMovedOnBalance(int Cid);
	virtual void Tick();
	
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
};

#endif

