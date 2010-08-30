#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "door.h"

CDoor::CDoor(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length, bool Opened)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Pos = Pos;
	m_Opened = Opened;

	vec2 Dir = vec2(sin(Rotation), cos(Rotation));
	vec2 To = Pos + normalize(Dir)*Length;
	
	GameServer()->Collision()->IntersectNoLaser(Pos, To, &this->m_To, 0);
	GameWorld()->InsertEntity(this);
}

void CDoor::Open(int Tick)
{
	m_EvalTick = Tick;
	m_Opened = true;
}

void CDoor::Close()
{
	m_Opened = false;
}

bool CDoor::HitCharacter()
{
	vec2 Points[36];
	vec2 At;
	for(int i=1;i<=36;i++)
	{
		Points[i-1].x = m_Pos.x * (1 - i/38.0) + m_To.x * i / 38.0;
		Points[i-1].y = m_Pos.y * (1 - i/38.0) + m_To.y * i / 38.0;
	}
	CCharacter *Hit = GameServer()->m_World.IntersectCharacter(m_Pos, Points[0], 1.f, At, 0);
	if(Hit)
		Hit->m_Doored = true;
	Hit = 0;
	Hit = GameServer()->m_World.IntersectCharacter(Points[0], m_Pos, 1.f, At, 0);
	if(Hit)
		Hit->m_Doored = true;
	Hit = 0;
	for(int i = 0; i < 36; i++)
	{
		Hit = GameServer()->m_World.IntersectCharacter(Points[i], Points[i+1], 1.f, At, 0);
		if(Hit)
			Hit->m_Doored = true;
		Hit = 0;
		Hit = GameServer()->m_World.IntersectCharacter(Points[i+1], Points[i], 1.f, At, 0);
		if(Hit)
			Hit->m_Doored = true;
		Hit = 0;
	}
	Hit = GameServer()->m_World.IntersectCharacter(Points[35], m_To, 1.f, At, 0);
	if(Hit)
		Hit->m_Doored = true;
	Hit = 0;
	Hit = GameServer()->m_World.IntersectCharacter(m_To, Points[35], 1.f, At, 0);
	if(Hit)
		Hit->m_Doored = true;
	Hit = 0;
	//hit->reset_pos();
	return true;
}

void CDoor::Reset()
{
	m_Opened = false;
}

void CDoor::Tick()
{
	if (!m_Opened)
		HitCharacter();
	else if (m_EvalTick + 10 < Server()->Tick())
		Close();
	return;
}

void CDoor::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient, m_To))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;

	if (!m_Opened)
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
