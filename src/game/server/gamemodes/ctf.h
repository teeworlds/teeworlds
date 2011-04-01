/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_CTF_H
#define GAME_SERVER_GAMEMODES_CTF_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>

class CGameControllerCTF : public IGameController
{
public:
	class CFlag *m_apFlags[2];
	
	CGameControllerCTF(class CGameContext *pGameServer);
	virtual bool CanBeMovedOnBalance(int ClientID);
<<<<<<< HEAD
=======
	virtual void Snap(int SnappingClient);
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	virtual void Tick();
	
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
};

#endif

