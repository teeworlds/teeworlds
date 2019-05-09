/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "growingexplosion.h"
#include "merc-bomb.h"
#include <game/server/entities/character.h>

CMercenaryBomb::CMercenaryBomb(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_MERCENARY_BOMB, Pos)
{
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);
	m_LoadingTick = Server()->TickSpeed();
	m_Owner = Owner;
	m_Damage = 0;
	
	for(int i=0; i<NUM_IDS; i++)
	{
		m_IDs[i] = Server()->SnapNewID();
	}
}

CMercenaryBomb::~CMercenaryBomb()
{
	for(int i=0; i<NUM_IDS; i++)
	{
		Server()->SnapFreeID(m_IDs[i]);
	}
}

void CMercenaryBomb::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CMercenaryBomb::IncreaseDamage()
{
	m_Damage += 2;
	if(m_Damage > g_Config.m_InfMercBombs)
		m_Damage = g_Config.m_InfMercBombs;
}

void CMercenaryBomb::Tick()
{
	if(IsMarkedForDestroy()) return;
	
	if(m_Damage >= g_Config.m_InfMercBombs && m_LoadingTick > 0)
		m_LoadingTick--;
	
	// Find other players
	bool MustExplode = false;
	for(CCharacter *p = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
	{
		if(p->IsHuman()) continue;
		//if(p->GetClass() == PLAYERCLASS_UNDEAD && p->IsFrozen()) continue;
		//if(p->GetClass() == PLAYERCLASS_VOODOO && p->m_VoodooAboutToDie) continue;

		float Len = distance(p->GetPos(), m_Pos);
		if(Len < p->GetProximityRadius()+80.0f)
		{
			MustExplode = true;
			break;
		}
	}
	
	if(MustExplode)
		Explode();
}

void CMercenaryBomb::Explode()
{
	float Factor = static_cast<float>(m_Damage)/g_Config.m_InfMercBombs;
	
	if(m_Damage > 1)
	{
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
		new CGrowingExplosion(GameWorld(), m_Pos, vec2(0.0, -1.0), m_Owner, 16.0f * Factor, GROWINGEXPLOSIONEFFECT_BOOM_INFECTED);
	}
				
	GameServer()->m_World.DestroyEntity(this);
}

bool CMercenaryBomb::ReadyToExplode()
{
	return m_LoadingTick <= 0;
}

void CMercenaryBomb::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	//CPlayer* pClient = GameServer()->m_apPlayers[SnappingClient];
	//if(pClient->IsInfected()) // invisible for zombies
	//	return;

	float AngleStart = (2.0f * pi * Server()->Tick()/static_cast<float>(Server()->TickSpeed()))/10.0f;
	float AngleStep = 2.0f * pi / CMercenaryBomb::NUM_SIDE;
	float R = 50.0f*static_cast<float>(m_Damage)/g_Config.m_InfMercBombs;
	for(int i=0; i<CMercenaryBomb::NUM_SIDE; i++)
	{
		vec2 PosStart = m_Pos + vec2(R * cos(AngleStart + AngleStep*i), R * sin(AngleStart + AngleStep*i));
			
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_IDs[i], sizeof(CNetObj_Pickup)));
		if(!pP)
			return;

		pP->m_X = (int)PosStart.x;
		pP->m_Y = (int)PosStart.y;
		pP->m_Type = PICKUP_HEALTH;
	}
	
	if(SnappingClient == m_Owner && m_LoadingTick > 0)
	{
		R = 80.0f;
		AngleStart = AngleStart*2.0f;
		for(int i=0; i<CMercenaryBomb::NUM_SIDE; i++)
		{
			vec2 PosStart = m_Pos + vec2(R * cos(AngleStart + AngleStep*i), R * sin(AngleStart + AngleStep*i));
			
			CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDs[CMercenaryBomb::NUM_SIDE+i], sizeof(CNetObj_Projectile)));
			if(pObj)
			{
				pObj->m_X = (int)PosStart.x;
				pObj->m_Y = (int)PosStart.y;
				pObj->m_VelX = 0;
				pObj->m_VelY = 0;
				pObj->m_StartTick = Server()->Tick();
				pObj->m_Type = WEAPON_HAMMER;
			}
		}
	}
}
