#include <generated/server_data.h>
#include <game/server/gamecontext.h>

#include "character.h"
#include <game/server/player.h>
#include "harpoon.h"
#include "pickup.h"
#include "flag.h"
#include <game/server/entity.h>

CHarpoon::CHarpoon(CGameWorld* pGameWorld, vec2 Pos, vec2 Direction, int Owner, CCharacter* This)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, Pos)
{
	m_Pos = Pos;
	m_pOwnerChar = This;
	m_Owner = Owner;
	m_pVictim = 0x0;
	m_DeathTick = GameServer()->Server()->TickSpeed()*10; //10 Seconds of flight time
	m_Direction = Direction;
	m_Status = HARPOON_FLYING;
	m_SpawnTick = GameServer()->Server()->Tick();
	m_StartTick = GameServer()->Server()->Tick();
	m_Size = 16.0f;
	GameWorld()->InsertEntity(this);
}

void CHarpoon::Tick()
{
	if ((!m_pOwnerChar) || (m_pOwnerChar->GetActiveWeapon()!=WEAPON_HARPOON))
	{
		RemoveHarpoon();
		return;
	}
	if (m_DeathTick > 0)
	{
		m_DeathTick--;
		if (!m_DeathTick)
		{
			RemoveHarpoon();
			return;
		}
	}

	if (m_pVictimEntity&& m_pVictimEntity->m_MarkForHarpoonDeallocation)
	{
		RemoveHarpoon();
		return;
	}



	if (!m_Status)
	{
		float Pt = (Server()->Tick() - m_StartTick - 1) / (float)Server()->TickSpeed();
		float Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();

		vec2 NewPos = GetPos(Ct, m_Pos, m_Direction, GameServer()->Tuning()->m_HarpoonCurvature, GameServer()->Tuning()->m_HarpoonSpeed);
		vec2 OldPos = GetPos(Pt, m_Pos, m_Direction, GameServer()->Tuning()->m_HarpoonCurvature, GameServer()->Tuning()->m_HarpoonSpeed);

		vec2 Dir = normalize(NewPos - OldPos);

		int Collide = GameServer()->Collision()->IntersectLine(OldPos, NewPos, &NewPos, 0);
		if (GameLayerClipped(NewPos))
		{
			m_pOwnerChar->m_HarpoonReloadTimer = GameServer()->Server()->TickSpeed() * GameServer()->Tuning()->m_HarpoonReloadTime;
			RemoveHarpoon();
			return;
		}
		CCharacter* TargetChr = GameWorld()->IntersectCharacter(OldPos, NewPos, m_Size, NewPos, m_pOwnerChar, this);
		CEntity* TargetPickup = GameWorld()->IntersectEntity(OldPos, NewPos, m_Size, NewPos, CGameWorld::ENTTYPE_PICKUP, 0x0, this);
		CEntity* TargetFlag = GameWorld()->IntersectEntity(OldPos, NewPos, m_Size, NewPos, CGameWorld::ENTTYPE_FLAG, 0x0, this);
		if (TargetChr)
		{
			m_pVictim = TargetChr;
			m_Status = HARPOON_IN_CHARACTER;
			m_Angle = Dir;
			TargetChr->TakeDamage(Dir, Dir * -1, GameServer()->Tuning()->m_HarpoonDamage, m_Owner, WEAPON_HARPOON);
			TargetChr->AllocateHarpoon(this);
			m_DeathTick = GameServer()->Server()->TickSpeed() * 5; //5 seconds of automatic drag
		}
		else if (Collide)
		{
			if (m_pOwnerChar->m_HarpoonReloadTimer == 0)
				m_pOwnerChar->m_HarpoonReloadTimer = GameServer()->Server()->TickSpeed() * GameServer()->Tuning()->m_HarpoonReloadTime;
			RemoveHarpoon();
			return;
		}
		else if (TargetPickup)
		{
			m_pVictimEntity = TargetPickup;
			m_Status = HARPOON_IN_ENTITY;
			m_DeathTick = GameServer()->Server()->TickSpeed() * 5; //5 seconds of automatic drag
			m_pVictimEntity->AllocateHarpoon(this);
			m_pOwnerChar->m_HarpoonReloadTimer = GameServer()->Server()->TickSpeed() * GameServer()->Tuning()->m_HarpoonReloadTime;
			m_Angle = Dir;
		}
		else if (TargetFlag)
		{
			m_pVictimEntity = TargetFlag;
			m_Status = HARPOON_IN_FLAG;
			m_pVictimEntity->AllocateHarpoon(this);
			m_DeathTick = GameServer()->Server()->TickSpeed() * 5; //5 seconds of automatic drag
			m_Angle = Dir;
		}
	}

	if (m_pVictim&&m_pOwnerChar)
	{
		if (!m_pVictim->IsAlive())
		{
			RemoveHarpoon();
			return;
		}
		vec2 Dir = normalize(m_pOwnerChar->GetPos() - m_pVictim->GetPos());
		m_pVictim->HarpoonDrag(Dir * distance(m_pOwnerChar->GetPos(), m_pVictim->GetPos()) / 500.0f * GameServer()->Tuning()->m_HarpoonMultiplier);
		if(GameServer()->Server()->TickSpeed() * 5 != m_DeathTick)
			if (((GameServer()->Server()->TickSpeed() * 5 - m_DeathTick) % (int)(GameServer()->Tuning()->m_HarpoonDelay * GameServer()->Server()->TickSpeed())) == 0)
				m_pVictim->TakeDamage(Dir, Dir * -1, 1, m_Owner, WEAPON_HARPOON);
	}
	if (m_pVictimEntity && m_pOwnerChar)
	{
		if (distance(m_pVictimEntity->GetPos(), m_pOwnerChar->GetPos()) < 16.0f&&m_Status==HARPOON_IN_ENTITY)
		{
			RemoveHarpoon();
			return;
		}
		vec2 Dir = normalize(m_pOwnerChar->GetPos() - m_pVictimEntity->GetPos());
		m_Pos = m_pVictimEntity->GetPos();
		m_pVictimEntity->ApplyHarpoonVel(Dir * distance(m_pOwnerChar->GetPos(), m_pVictimEntity->GetPos()) / 500.0f);
	}

	if (GameServer()->Server()->Tick() % 10 == 0)
	{
 		GameServer()->CreateSound(m_pOwnerChar->GetPos(), SOUND_HOOK_LOOP);
	}
}

vec2 CHarpoon::GetPos(float Tick, vec2 Pos, vec2 Direction, int Curvature, int Speed)
{
	return CalcPos(Pos, Direction, Curvature, Speed, Tick);
}

void CHarpoon::RemoveHarpoon()
{
	if (m_pVictimEntity)
		m_pVictimEntity->DeallocateHarpoon();
	if(m_pOwnerChar)
		m_pOwnerChar->DeallocateHarpoon();
	if (m_pVictim)
		m_pVictim->DeallocateVictimHarpoon();
	m_pVictimEntity = 0x0;
	m_pVictim = 0x0;
	m_pOwnerChar = 0x0;
	m_Status = HARPOON_FLYING;
	//GameWorld()->RemoveEntity(this);
	GameWorld()->DestroyEntity(this);
	//delete this;
}

void CHarpoon::DeallocateOwner()
{
	m_pOwnerChar = 0x0;
}
void CHarpoon::DeallocateVictim()
{
	m_pVictim = 0x0;
	m_pVictimEntity = 0x0;
	m_Status = HARPOON_FLYING;
}

void CHarpoon::TickPaused()
{
	m_SpawnTick++;
}

void CHarpoon::Reset()
{
	RemoveHarpoon();
}

void CHarpoon::Snap(int SnappingClient)
{
	if (m_Status)
	{
		CNetObj_HarpoonDragPlayer* pHarpoon2 = static_cast<CNetObj_HarpoonDragPlayer*>(Server()->SnapNewItem(NETOBJTYPE_HARPOONDRAGPLAYER, GetID(), sizeof(CNetObj_HarpoonDragPlayer)));
		if (pHarpoon2)
			FillPlayerDragInfo(pHarpoon2, SnappingClient);
	}
	else if(m_pOwnerChar->IsAlive())
	{
		CNetObj_Harpoon* pHarpoon = static_cast<CNetObj_Harpoon*>(Server()->SnapNewItem(NETOBJTYPE_HARPOON, GetID(), sizeof(CNetObj_Harpoon)));
		if (pHarpoon)
			FillInfo(pHarpoon, SnappingClient);
	}
}

void CHarpoon::FillInfo(CNetObj_Harpoon* pHarpoon, int SnappingClient)
{
	pHarpoon->m_X = round_to_int(m_Pos.x);
	pHarpoon->m_Y = round_to_int(m_Pos.y);
	pHarpoon->m_Dir_X = round_to_int(m_Direction.x*100.0f);
	pHarpoon->m_Dir_Y = round_to_int(m_Direction.y*100.0f);
	pHarpoon->m_SpawnTick = m_SpawnTick;
	if (NetworkClipped(SnappingClient, m_pOwnerChar->GetPos()))
	{
		pHarpoon->m_Owner_X = m_pOwnerChar->GetPos().x * 100.0f;
		pHarpoon->m_Owner_Y = m_pOwnerChar->GetPos().y * 100.0f;
		pHarpoon->m_OwnerID = -1;
	}
	else
	{
		pHarpoon->m_OwnerID = m_pOwnerChar->GetPlayer()->GetCID();
	}
}

void CHarpoon::FillPlayerDragInfo(CNetObj_HarpoonDragPlayer* pHarpoon, int SnappingClient)
{
	pHarpoon->m_X = 0;
	pHarpoon->m_Y = 0;
	pHarpoon->m_OwnerID = 0;
	pHarpoon->m_VictimID = 0;
	if (m_Status == HARPOON_IN_CHARACTER && m_pOwnerChar && m_pVictim) //data needed to send 
	{
		if (NetworkClipped(SnappingClient, m_pVictim->GetPos()))
		{
			pHarpoon->m_X = m_pVictim->GetPos().x * 100.0f;
			pHarpoon->m_Y = m_pVictim->GetPos().y * 100.0f;
			pHarpoon->m_VictimID = -1;
		}
		else
		{
			pHarpoon->m_VictimID = m_pVictim->GetPlayer()->GetCID();
		}

		if (NetworkClipped(SnappingClient, m_pOwnerChar->GetPos()))
		{
			pHarpoon->m_Owner_X = m_pOwnerChar->GetPos().x * 100.0f;
			pHarpoon->m_Owner_Y = m_pOwnerChar->GetPos().y * 100.0f;
			pHarpoon->m_OwnerID = -1;
		}
		else
		{
			pHarpoon->m_OwnerID = m_pOwnerChar->GetPlayer()->GetCID();
		}
	}
	else if (m_Status > HARPOON_IN_CHARACTER)
	{
		if (NetworkClipped(SnappingClient, m_pOwnerChar->GetPos()))
		{
			pHarpoon->m_Owner_X = m_pOwnerChar->GetPos().x * 100.0f;
			pHarpoon->m_Owner_Y = m_pOwnerChar->GetPos().y * 100.0f;
			pHarpoon->m_OwnerID = -1;
		}
		else
		{
			pHarpoon->m_OwnerID = m_pOwnerChar->GetPlayer()->GetCID();
		}
		pHarpoon->m_X = round_to_int(m_pVictimEntity->GetPos().x*100);
		pHarpoon->m_Y = round_to_int(m_pVictimEntity->GetPos().y*100);
	}

	pHarpoon->m_Type = m_Status;
}