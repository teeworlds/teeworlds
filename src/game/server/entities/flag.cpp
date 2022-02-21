/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>

#include "character.h"
#include "flag.h"
#include "game/server/player.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team, vec2 StandPos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG, StandPos, ms_PhysSize)
{
	m_Team = Team;
	m_StandPos = StandPos;

	GameWorld()->InsertEntity(this);

	Reset();
}

void CFlag::Reset()
{
	m_pCarrier = 0;
	m_AtStand = true;
	m_Pos = m_StandPos;
	m_Vel = vec2(0, 0);
	m_GrabTick = 0;
	if (m_pHarpoon)
		m_pHarpoon->RemoveHarpoon();
	m_pHarpoon = 0x0;
	m_MarkForHarpoonDeallocation = false;
	m_HarpoonVel = vec2(0, 0);
}

void CFlag::Grab(CCharacter *pChar)
{
	m_pCarrier = pChar;
	if(m_AtStand)
		m_GrabTick = Server()->Tick();
	m_AtStand = false;
}

void CFlag::Drop()
{
	m_pCarrier = 0;
	m_Vel = vec2(0, 0);
	m_DropTick = Server()->Tick();
	m_MarkForHarpoonDeallocation = false;
}

void CFlag::TickDefered()
{
	if(m_pCarrier)
	{
		// update flag position
		m_Pos = m_pCarrier->GetPos();
		m_MarkForHarpoonDeallocation = true;
		if (m_pHarpoon)
			m_pHarpoon->RemoveHarpoon();
	}
	else
	{
		// flag hits death-tile or left the game layer, reset it
		if((((GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y) & CCollision::COLFLAG_DEATH )&&!m_pHarpoon))
			|| GameLayerClipped(m_Pos))
		{
			Reset();
			GameServer()->m_pController->OnFlagReturn(this);
		}

		if(!m_AtStand)
		{
			if(Server()->Tick() > m_DropTick + Server()->TickSpeed()*30)
			{
				Reset();
				GameServer()->m_pController->OnFlagReturn(this);
			}
			else
			{
				if (m_pHarpoon)
				{
					m_DropTick++;
					return;
				}
				m_Vel.y += GameServer()->Collision()->TestBox(m_Pos, vec2(ms_PhysSize, ms_PhysSize),8) ? -GameWorld()->m_Core.m_Tuning.m_Gravity : GameWorld()->m_Core.m_Tuning.m_Gravity;
				GameServer()->Collision()->MoveWaterBox(&m_Pos, &m_Vel, vec2(ms_PhysSize, ms_PhysSize), 0.5f, 0 , GameWorld()->m_Core.m_Tuning.m_LiquidFlagResistance);
			}
		}
	}
}

void CFlag::TickPaused()
{
	m_DropTick++;
	if(m_GrabTick)
		m_GrabTick++;
}

void CFlag::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_Team, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = round_to_int(m_Pos.x);
	pFlag->m_Y = round_to_int(m_Pos.y);
	pFlag->m_Team = m_Team;
}

void CFlag::ApplyHarpoonVel(vec2 Vel)
{
	Vel *= GameServer()->Tuning()->m_HarpoonEntityMultiplier;
	float MaximumSpeed = GameServer()->Tuning()->m_HarpoonFlagDragSpeed;
	float MinimumSpeed = GameServer()->Tuning()->m_HarpoonFlagMinSpeed;
	float Length = length(Vel);
	if (Length < MinimumSpeed)
	{
		Vel = normalize(Vel);
		Vel *= MinimumSpeed;
	}
	else if (Length > MaximumSpeed)
	{
		Vel = normalize(Vel);
		Vel *= MaximumSpeed;
	}

	m_HarpoonVel += Vel;
}

void CFlag::ApplyHarpoonDrag()
{
	vec2 NewPos = m_Pos;
	GameServer()->Collision()->MoveBox(&NewPos, &m_HarpoonVel, vec2(ms_PhysSize, ms_PhysSize), 0, 0);

	m_Pos = NewPos;

	m_HarpoonVel = vec2(0, 0);
}

void CFlag::AllocateHarpoon(CHarpoon* pHarpoon)
{
	/*if (m_pHarpoon->GetOwner()->GetPlayer()->GetTeam() == GetTeam())
	{
		m_pHarpoon->DeallocateVictim();
		return;
	}*/
	m_DropTick = Server()->Tick();
	m_AtStand = false;
	m_pHarpoon = pHarpoon;
	
	GameServer()->SendGameMsg(GAMEMSG_CTF_GRAB, m_Team, -1);
}

void CFlag::DeallocateHarpoon()
{
	m_pHarpoon = 0x0;
	m_MarkForHarpoonDeallocation = false;
}

bool CFlag::IsValidForHarpoon(CHarpoon* pHarpoon)
{
	if (pHarpoon->GetOwner()->GetPlayer()&&pHarpoon->GetOwner()->GetPlayer()->GetTeam() == m_Team)
		return false;
	if (m_pCarrier)
		return false;
	return true;
}