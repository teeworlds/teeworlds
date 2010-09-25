#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "door.h"

CDoor::CDoor(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length, bool Opened)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Pos = Pos;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		m_Opened[i] = false;
	}//TODO: Check this
	

	vec2 Dir = vec2(sin(Rotation), cos(Rotation));
	vec2 To = Pos + normalize(Dir)*Length;
	
	GameServer()->Collision()->IntersectNoLaser(Pos, To, &this->m_To, 0);
	GameWorld()->InsertEntity(this);
}

void CDoor::Open(int Tick, bool ActivatedTeam[])
{
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if(ActivatedTeam[i]) m_EvalTick[i] = Tick;
		m_Opened[i] = ActivatedTeam[i];
	}
}

void CDoor::Close(int Team)
{
	m_Opened[Team] = false;
}

bool CDoor::HitCharacter(int Team)
{
	std::list < CCharacter * > HitCharacters = GameServer()->m_World.IntersectedCharacters(m_Pos, m_To, 1.f, 0);
	if(HitCharacters.empty()) return false;
	for(std::list < CCharacter * >::iterator i = HitCharacters.begin(); i != HitCharacters.end(); i++)
	{
		CCharacter * Char = *i;
		if(Char->Team() == Team)
		{
			//DoDoored(Char);
			Char->m_Doored = true;
		}
	}
	return true;
}
// my problem here is that this will set the values that will tell the character not to go to a certain direction or directions
//but what will tell it that there isn't a door there if it moves in the other direction or the door opens
//need a better idea
//thinking...
bool CDoor::DoDoored(CCharacter* pChar)
{
	vec2 Pos = closest_point_on_line(m_Pos,m_To,pChar->m_Intersection);

	return true;

}

void CDoor::Reset()
{
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		m_Opened[i] = false;
	}
}

void CDoor::Tick()
{
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if(!m_Opened[i]) {
			HitCharacter(i);
		} else if (m_EvalTick[i] + 10 < Server()->Tick()) {
			Close(i);
		}
	}
}

void CDoor::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient, m_To))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;

	CCharacter * Char = GameServer()->GetPlayerChar(SnappingClient);
	if(Char == 0) return;

	if (!m_Opened[Char->Team()])
	{
		pObj->m_FromX = (int)m_To.x;
		pObj->m_FromY = (int)m_To.y;
	}
	else
	{
		pObj->m_FromX = (int)m_Pos.x;
		pObj->m_FromY = (int)m_Pos.y;
	}
	pObj->m_StartTick = Server()->Tick();
}
