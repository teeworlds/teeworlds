/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_CTF_H
#define GAME_SERVER_GAMEMODES_CTF_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>

class CGameControllerCTF : public IGameController
{
	// balancing
	virtual bool CanBeMovedOnBalance(int ClientID) const;

	// game
	class CFlag *m_apFlags[2];

	virtual void DoWincheckMatch();

public:
	CGameControllerCTF(class CGameContext *pGameServer);
	
	// event
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnEntity(int Index, vec2 Pos);

	// general
	virtual void Snap(int SnappingClient);
	virtual void Tick();
};

#endif

