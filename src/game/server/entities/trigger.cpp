/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "trigger.h"
#include "door.h"

//////////////////////////////////////////////////
// DOOR
//////////////////////////////////////////////////

CTrigger::CTrigger(CGameWorld *pGameWorld, vec2 Pos, CEntity *Target)
: CEntity(pGameWorld, NETOBJTYPE_PICKUP)
{
	m_Pos = Pos;
	m_Target = Target;

	GameWorld()->InsertEntity(this);
}

bool CTrigger::HitCharacter()
{
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	return (!pChr) ? false : true;
}

void CTrigger::OpenDoor(int Tick)
{
	CDoor *ConnectedDoor = (CDoor*) m_Target;
	ConnectedDoor->Open(Tick);
}

void CTrigger::Reset()
{
	return;
}

void CTrigger::Tick()
{
	if (Server()->Tick() % 10 == 0)
	{
		if (HitCharacter())
			OpenDoor(Server()->Tick());
	}
	return;
}

void CTrigger::Snap(int SnappingClient)
{
	return;
}
