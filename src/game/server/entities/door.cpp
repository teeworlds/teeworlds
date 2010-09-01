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
		if(!m_Opened[i] && ActivatedTeam[i]) m_EvalTick[i] = Tick;
		m_Opened[i] = ActivatedTeam[i];
	}
}

void CDoor::Close(int Team)
{
	m_Opened[Team] = false;
}

bool CDoor::HitCharacter(int Team)
{
	vec2 At;
	std::list < CCharacter * > hittedCharacters = GameServer()->m_World.IntersectedCharacters(m_Pos, m_To, 1.f, At, 0);
	if(hittedCharacters.empty()) return false;
	for(std::list < CCharacter * >::iterator i = hittedCharacters.begin(); i != hittedCharacters.end(); i++) {
		CCharacter * Char = *i;
		if(Char->Team() == Team)
			Char = false;
	}
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
		} else if (m_EvalTick[i] + 10 < Server()->Tick())
			Close(i);
	}
}

void CDoor::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient, m_To))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;

	if (!m_Opened[SnappingClient])
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
