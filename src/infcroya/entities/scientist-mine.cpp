/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "scientist-mine.h"

#include "growingexplosion.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>

CScientistMine::CScientistMine(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_SCIENTIST_MINE, Pos)
{
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);
	m_DetectionRadius = 60.0f;
	m_StartTick = Server()->Tick();
	m_Owner = Owner;
	
	for(int i=0; i<NUM_IDS; i++)
	{
		m_IDs[i] = Server()->SnapNewID();
	}
}

CScientistMine::~CScientistMine()
{
	for(int i=0; i<NUM_IDS; i++)
	{
		Server()->SnapFreeID(m_IDs[i]);
	}
}

void CScientistMine::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

int CScientistMine::GetOwner() const
{
	return m_Owner;
}

void CScientistMine::Explode(int DetonatedBy)
{
	new CGrowingExplosion(GameWorld(), m_Pos, vec2(0.0, -1.0), m_Owner, 6, GROWINGEXPLOSIONEFFECT_ELECTRIC_INFECTED);
	GameServer()->m_World.DestroyEntity(this);
	
	//Self damage
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(OwnerChar)
	{
		float Dist = distance(m_Pos, OwnerChar->GetPos());
		if(Dist < OwnerChar->GetProximityRadius()+g_Config.m_InfMineRadius)
			OwnerChar->TakeDamage(vec2(0.0f, 0.0f), m_Pos, 4, DetonatedBy, WEAPON_HAMMER);
		else if(Dist < OwnerChar->GetProximityRadius()+2*g_Config.m_InfMineRadius)
		{
			float Alpha = (Dist - g_Config.m_InfMineRadius-OwnerChar->GetProximityRadius())/g_Config.m_InfMineRadius;
			OwnerChar->TakeDamage(vec2(0.0f, 0.0f), m_Pos, 4*Alpha, DetonatedBy, WEAPON_HAMMER);
		}
	}
}

void CScientistMine::Snap(int SnappingClient)
{
	float Radius = g_Config.m_InfMineRadius;
	
	int NumSide = CScientistMine::NUM_SIDE;
	//if(Server()->GetClientAntiPing(SnappingClient))
	//	NumSide = std::min(6, NumSide);
	
	float AngleStep = 2.0f * pi / NumSide;
	
	for(int i=0; i<NumSide; i++)
	{
		vec2 PartPosStart = m_Pos + vec2(Radius * cos(AngleStep*i), Radius * sin(AngleStep*i));
		vec2 PartPosEnd = m_Pos + vec2(Radius * cos(AngleStep*(i+1)), Radius * sin(AngleStep*(i+1)));
		
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = (int)PartPosStart.x;
		pObj->m_Y = (int)PartPosStart.y;
		pObj->m_FromX = (int)PartPosEnd.x;
		pObj->m_FromY = (int)PartPosEnd.y;
		pObj->m_StartTick = Server()->Tick();
	}
	
	//if(!Server()->GetClientAntiPing(SnappingClient))
	//{
	for(int i=0; i<CScientistMine::NUM_PARTICLES; i++)
	{
		float RandomRadius = frandom()*(Radius-4.0f);
		float RandomAngle = 2.0f * pi * frandom();
		vec2 ParticlePos = m_Pos + vec2(RandomRadius * cos(RandomAngle), RandomRadius * sin(RandomAngle));
			
		CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDs[CScientistMine::NUM_SIDE+i], sizeof(CNetObj_Projectile)));
		if(pObj)
		{
			pObj->m_X = (int)ParticlePos.x;
			pObj->m_Y = (int)ParticlePos.y;
			pObj->m_VelX = 0;
			pObj->m_VelY = 0;
			pObj->m_StartTick = Server()->Tick();
			pObj->m_Type = WEAPON_HAMMER;
		}
	}
	//}
}

void CScientistMine::Tick()
{
	if(IsMarkedForDestroy()) return;

	// Find other players
	bool MustExplode = false;
	int DetonatedBy;
	for(CCharacter *p = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
	{
		if(p->IsHuman()) continue;
		//if(p->GetClass() == PLAYERCLASS_UNDEAD && p->IsFrozen()) continue;
		//if(p->GetClass() == PLAYERCLASS_VOODOO && p->m_VoodooAboutToDie) continue;

		float Len = distance(p->GetPos(), m_Pos);
		if(Len < p->GetProximityRadius()+g_Config.m_InfMineRadius)
		{
			MustExplode = true;
			CPlayer *pDetonatedBy = p->GetPlayer();
			if (pDetonatedBy)
				DetonatedBy = pDetonatedBy->GetCID();
			else
				DetonatedBy = m_Owner;
			break;
		}
	}
	
	if(MustExplode)
		Explode(DetonatedBy);
}

void CScientistMine::TickPaused()
{
	++m_StartTick;
}
