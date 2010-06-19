#ifndef GAME_SERVER_GAMEMODES_CTF_H
#define GAME_SERVER_GAMEMODES_CTF_H
#include <game/server/gamecontroller.h>

class CGameControllerFC : public IGameController
{
public:
	class CFlag *m_apFlags[2];
	
	CGameControllerFC(class CGameContext *pGameServer);
	
	bool IsOwnFlagStand(vec2 Pos, int Team);
	bool IsEnemyFlagStand(vec2 Pos, int Team);
	
	virtual bool CanBeMovedOnBalance(int Cid);
	
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool CanSpawn(class CPlayer *pPlayer, vec2 *pOutPos);
	
	virtual bool IsFastCap() { return true; }
};

#endif
