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

// TODO: move to seperate file
class CFlag : public CEntity
{
public:
	static const int ms_PhysSize = 14;
	CCharacter *m_pCarryingCharacter;
	vec2 m_Vel;
	vec2 m_StandPos;
	
	int m_Team;
	int m_AtStand;
	int m_DropTick;
	int m_GrabTick;
	
	CFlag(CGameWorld *pGameWorld, int Team);

	virtual void Reset();
	virtual void Snap(int SnappingClient);
};
#endif

