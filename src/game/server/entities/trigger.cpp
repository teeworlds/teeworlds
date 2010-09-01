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
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_TeamActivated[i] = false;
	}
	

	GameWorld()->InsertEntity(this);
}

bool CTrigger::HitCharacter()
{
	CCharacter *Ents[16];
	
	Reset();
	
	int Num = GameServer()->m_World.FindEntities(m_Pos, 20.0f, (CEntity**)Ents, 16, NETOBJTYPE_CHARACTER);
	if(Num <= 0) {
		return false;
	} else {
		for (int i = 0; i < Num; i++)
		{
			m_TeamActivated[Ents[i]->Team()] = true;
		}
	}
	return true;
}

void CTrigger::OpenDoor(int Tick)
{
	CDoor *ConnectedDoor = (CDoor*) m_Target;
	ConnectedDoor->Open(Tick, m_TeamActivated);
}

void CTrigger::Reset()
{
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_TeamActivated[i] = false;
	}
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
