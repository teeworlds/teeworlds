/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/gamecontext.h>

#include "game/server/entities/character.h"
#include "game/server/player.h"
#include "medic-laser.h"
#include <infcroya/croyaplayer.h>
#include <infcroya/classes/class.h>

CMedicLaser::CMedicLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos)
{
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
	DoBounce();
}


bool CMedicLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	if(!pHit)
		return false;

	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	if (pOwnerChar && pOwnerChar->GetCroyaPlayer()->GetClassNum() == Class::MEDIC) { // Revive zombie
		const int MIN_ZOMBIES = 4;
		const int DAMAGE_ON_REVIVE = 17;
		int OldClass = pHit->GetCroyaPlayer()->GetOldClassNum();
		auto medic = pOwnerChar;
		auto zombie = pHit;
		CGameContext* pGameServer = medic->GameServer();

		if (medic && medic->GetHealthArmorSum() <= DAMAGE_ON_REVIVE) {
			int HealthArmor = DAMAGE_ON_REVIVE + 1;
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "You need at least %d hp", HealthArmor);
			pGameServer->SendBroadcast(aBuf, medic->GetPlayer()->GetCID());
		}
		else if (GameServer()->GetZombieCount() <= MIN_ZOMBIES) {
			int MinZombies = MIN_ZOMBIES + 1;
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Too few zombies (less than %d)", MinZombies);
			GameServer()->SendBroadcast(aBuf, m_Owner);
		}
		else {
			zombie->GetCroyaPlayer()->SetClassNum(OldClass, true);
			if (zombie->GetPlayer()->GetCharacter()) {
				zombie->GetPlayer()->GetCharacter()->SetHealthArmor(1, 0);
				zombie->Unfreeze();
				medic->TakeDamage(vec2(0.f, 0.f), medic->GetPos(), DAMAGE_ON_REVIVE * 2, m_Owner, WEAPON_LASER);

				const char* MedicName = Server()->ClientName(medic->GetPlayer()->GetCID());
				const char* ZombieName = Server()->ClientName(zombie->GetPlayer()->GetCID());
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "Medic %s revived %s", MedicName, ZombieName);
				GameServer()->SendChatTarget(-1, aBuf);
				//Server()->RoundStatistics()->OnScoreEvent(ClientID, SCOREEVENT_MEDIC_REVIVE, medic->GetClass(), Server()->ClientName(ClientID), GameServer()->Console());
				medic->GetCroyaPlayer()->OnKill(zombie->GetPlayer()->GetCID());
			}
		}
	}
	else {
		pHit->TakeDamage(vec2(0.f, 0.f), normalize(To - From), g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage, m_Owner, WEAPON_LASER);
	}
	return true;
}

void CMedicLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir * m_Energy;

	if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			// intersected
			m_From = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;

			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
			m_Pos = TempPos;
			m_Dir = normalize(TempDir);

			m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
			m_Bounces++;

			if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
				m_Energy = -1;

			GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE);
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}
}

void CMedicLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CMedicLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CMedicLaser::TickPaused()
{
	++m_EvalTick;
}

void CMedicLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient) && NetworkClipped(SnappingClient, m_From))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
