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
	m_Length = Length;

	m_Direction = vec2(sin(Rotation), cos(Rotation));
	vec2 To = Pos + normalize(m_Direction) * m_Length;
	
	GameServer()->Collision()->IntersectNoLaser(Pos, To, &this->m_To, 0);
	ResetCollision();
	GameWorld()->InsertEntity(this);
}

void CDoor::Open(int Tick, bool ActivatedTeam[])
{
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if(ActivatedTeam[i]) m_EvalTick[i] = Tick;
		m_Opened[i] = ActivatedTeam[i];
		if(ActivatedTeam[i]) Open(i);
	}
}


void CDoor::ResetCollision()
{
	for(int i=0;i<m_Length;i++)
	{
		GameServer()->Collision()->SetDCollisionAt(m_Pos.x + (m_Direction.x * i), m_Pos.y + (m_Direction.y * i), TILE_STOPA, 99);
	}
}

void CDoor::Open(int Team)
{
	m_Opened[Team] = true;

	for(int i=0;i<m_Length;i++)
	{
		GameServer()->Collision()->SetDTile(m_Pos.x + (m_Direction.x * i), m_Pos.y + (m_Direction.y * i), Team, false);
	}
}

void CDoor::Close(int Team)
{
	m_Opened[Team] = false;

	for(int i=0;i<m_Length;i++)
	{
		GameServer()->Collision()->SetDTile(m_Pos.x + (m_Direction.x * i), m_Pos.y + (m_Direction.y * i), Team, true);
	}
}

void CDoor::Reset()
{
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		m_Opened[i] = false;
		Close(i);
	}
}

void CDoor::Tick()
{
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (m_EvalTick[i] + 10 < Server()->Tick()) {
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
