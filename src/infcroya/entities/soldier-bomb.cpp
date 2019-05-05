/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "soldier-bomb.h"
#include <game/server/entities/character.h>

CSoldierBomb::CSoldierBomb(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_SOLDIER_BOMB, Pos)
{
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);
	m_DetectionRadius = 60.0f;
	m_StartTick = Server()->Tick();
	m_Owner = Owner;
	m_nbBomb = g_Config.m_InfSoldierBombs;
	
	m_IDBomb.set_size(g_Config.m_InfSoldierBombs);
	for(int i=0; i<m_IDBomb.size(); i++)
	{
		m_IDBomb[i] = Server()->SnapNewID();
	}
}

CSoldierBomb::~CSoldierBomb()
{
	for(int i=0; i<m_IDBomb.size(); i++)
		Server()->SnapFreeID(m_IDBomb[i]);
}

void CSoldierBomb::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CSoldierBomb::Explode()
{
	const int MAX_DAMAGE = 20;
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(!OwnerChar)
		return;
		
	vec2 dir = normalize(OwnerChar->GetPos() - m_Pos);
	
	GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
	GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_HAMMER, MAX_DAMAGE);
	for(int i=0; i<6; i++)
	{
		float angle = static_cast<float>(i)*2.0*pi/6.0;
		vec2 expPos = m_Pos + vec2(90.0*cos(angle), 90.0*sin(angle));
		GameServer()->CreateExplosion(expPos, m_Owner, WEAPON_HAMMER, MAX_DAMAGE);
	}
	for(int i=0; i<12; i++)
	{
		float angle = static_cast<float>(i)*2.0*pi/12.0;
		vec2 expPos = vec2(180.0*cos(angle), 180.0*sin(angle));
		if(dot(expPos, dir) <= 0)
		{
			GameServer()->CreateExplosion(m_Pos + expPos, m_Owner, WEAPON_HAMMER, MAX_DAMAGE);
		}
	}
	
	m_nbBomb--;
	
	if(m_nbBomb == 0)
	{
		GameServer()->m_World.DestroyEntity(this);
	}
}

void CSoldierBomb::Snap(int SnappingClient)
{
	float time = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	float angle = fmodf(time*pi/2, 2.0f*pi);
	
	for(int i=0; i<m_nbBomb; i++)
	{
		if(NetworkClipped(SnappingClient))
			return;
		
		float shiftedAngle = angle + 2.0*pi*static_cast<float>(i)/static_cast<float>(m_IDBomb.size());
		
		CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDBomb[i], sizeof(CNetObj_Projectile)));
		pProj->m_X = (int)(m_Pos.x + m_DetectionRadius*cos(shiftedAngle));
		pProj->m_Y = (int)(m_Pos.y + m_DetectionRadius*sin(shiftedAngle));
		pProj->m_VelX = (int)(0.0f);
		pProj->m_VelY = (int)(0.0f);
		pProj->m_StartTick = Server()->Tick();
		pProj->m_Type = WEAPON_GRENADE;
	}
}

void CSoldierBomb::TickPaused()
{
	++m_StartTick;
}

bool CSoldierBomb::AddBomb()
{
	if(m_nbBomb < m_IDBomb.size())
	{
		m_nbBomb++;
		return true;
	}
	else return false;
}
