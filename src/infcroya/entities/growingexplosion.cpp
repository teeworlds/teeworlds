/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "growingexplosion.h"

#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>

CGrowingExplosion::CGrowingExplosion(CGameWorld *pGameWorld, vec2 Pos, vec2 Dir, int Owner, int Radius, int ExplosionEffect)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_GROWINGEXPLOSION, Pos),
		m_pGrowingMap(NULL),
		m_pGrowingMapVec(NULL)
{
	m_MaxGrowing = Radius;
	m_GrowingMap_Length = (2*m_MaxGrowing+1);
	m_GrowingMap_Size = (m_GrowingMap_Length*m_GrowingMap_Length);
	
	m_pGrowingMap = new int[m_GrowingMap_Size];
	m_pGrowingMapVec = new vec2[m_GrowingMap_Size];
	
	m_Pos = Pos;
	m_StartTick = Server()->Tick();
	m_Owner = Owner;
	m_ExplosionEffect = ExplosionEffect;
	
	mem_zero(m_Hit, sizeof(m_Hit));

	GameWorld()->InsertEntity(this);	
	
	vec2 explosionTile = vec2(16.0f, 16.0f) + vec2(
		static_cast<float>(static_cast<int>(round(m_Pos.x))/32)*32.0,
		static_cast<float>(static_cast<int>(round(m_Pos.y))/32)*32.0);
	
	//Check is the tile is occuped, and if the direction is valide
	if(GameServer()->Collision()->CheckPoint(explosionTile) && length(Dir) <= 1.1)
	{
		m_SeedPos = vec2(16.0f, 16.0f) + vec2(
		static_cast<float>(static_cast<int>(round(m_Pos.x + 32.0f*Dir.x))/32)*32.0,
		static_cast<float>(static_cast<int>(round(m_Pos.y + 32.0f*Dir.y))/32)*32.0);
	}
	else
	{
		m_SeedPos = explosionTile;
	}
	
	m_SeedX = static_cast<int>(round(m_SeedPos.x))/32;
	m_SeedY = static_cast<int>(round(m_SeedPos.y))/32;
	
	for(int j=0; j<m_GrowingMap_Length; j++)
	{
		for(int i=0; i<m_GrowingMap_Length; i++)
		{
			vec2 Tile = m_SeedPos + vec2(32.0f*(i-m_MaxGrowing), 32.0f*(j-m_MaxGrowing));
			if(GameServer()->Collision()->CheckPoint(Tile) || distance(Tile, m_SeedPos) > m_MaxGrowing*32.0f)
			{
				m_pGrowingMap[j*m_GrowingMap_Length+i] = -2;
			}
			else
			{
				m_pGrowingMap[j*m_GrowingMap_Length+i] = -1;
			}
			
			m_pGrowingMapVec[j*m_GrowingMap_Length+i] = vec2(0.0f, 0.0f);
		}
	}
	
	m_pGrowingMap[m_MaxGrowing*m_GrowingMap_Length+m_MaxGrowing] = Server()->Tick();
	
	switch(m_ExplosionEffect)
	{
		case GROWINGEXPLOSIONEFFECT_FREEZE_INFECTED:
			if(random_prob(0.1f))
			{
				GameServer()->CreateHammerHit(m_SeedPos);
			}
			break;
		case GROWINGEXPLOSIONEFFECT_POISON_INFECTED:
			if(random_prob(0.1f))
			{
				GameServer()->CreateDeath(m_SeedPos, m_Owner);
			}
			break;
		case GROWINGEXPLOSIONEFFECT_ELECTRIC_INFECTED:
			{
				//~ GameServer()->CreateHammerHit(m_SeedPos);
					
				vec2 EndPoint = m_SeedPos + vec2(-16.0f + frandom()*32.0f, -16.0f + frandom()*32.0f);
				m_pGrowingMapVec[m_MaxGrowing*m_GrowingMap_Length+m_MaxGrowing] = EndPoint;
			}					
			break;
	}
}

CGrowingExplosion::~CGrowingExplosion()
{
	if(m_pGrowingMap)
	{
		delete[] m_pGrowingMap;
		m_pGrowingMap = NULL;
	}
		
	if(m_pGrowingMapVec)
	{
		delete[] m_pGrowingMapVec;
		m_pGrowingMapVec = NULL;
	}
}

void CGrowingExplosion::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

int CGrowingExplosion::GetOwner() const
{
	return m_Owner;
}

void CGrowingExplosion::Tick()
{
	if(IsMarkedForDestroy()) return;

	int tick = Server()->Tick();
	//~ if((tick - m_StartTick) > Server()->TickSpeed())
	if((tick - m_StartTick) > m_MaxGrowing)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}
	
	bool NewTile = false;
	
	for(int j=0; j<m_GrowingMap_Length; j++)
	{
		for(int i=0; i<m_GrowingMap_Length; i++)
		{
			if(m_pGrowingMap[j*m_GrowingMap_Length+i] == -1)
			{
				bool FromLeft = (i > 0 && m_pGrowingMap[j*m_GrowingMap_Length+i-1] < tick && m_pGrowingMap[j*m_GrowingMap_Length+i-1] >= 0);
				bool FromRight = (i < m_GrowingMap_Length-1 && m_pGrowingMap[j*m_GrowingMap_Length+i+1] < tick && m_pGrowingMap[j*m_GrowingMap_Length+i+1] >= 0);
				bool FromTop = (j > 0 && m_pGrowingMap[(j-1)*m_GrowingMap_Length+i] < tick && m_pGrowingMap[(j-1)*m_GrowingMap_Length+i] >= 0);
				bool FromBottom = (j < m_GrowingMap_Length-1 && m_pGrowingMap[(j+1)*m_GrowingMap_Length+i] < tick && m_pGrowingMap[(j+1)*m_GrowingMap_Length+i] >= 0);
				
				if(FromLeft || FromRight || FromTop || FromBottom)
				{
					m_pGrowingMap[j*m_GrowingMap_Length+i] = tick;
					NewTile = true;
					vec2 TileCenter = m_SeedPos + vec2(32.0f*(i-m_MaxGrowing) - 16.0f + frandom()*32.0f, 32.0f*(j-m_MaxGrowing) - 16.0f + frandom()*32.0f);
					const int MAX_DAMAGE = 18;
					switch(m_ExplosionEffect)
					{
						case GROWINGEXPLOSIONEFFECT_FREEZE_INFECTED:
							if(random_prob(0.1f))
							{
								GameServer()->CreateHammerHit(TileCenter);
							}
							break;
						case GROWINGEXPLOSIONEFFECT_POISON_INFECTED:
							if(random_prob(0.1f))
							{
								GameServer()->CreateDeath(TileCenter, m_Owner);
							}
							break;
						case GROWINGEXPLOSIONEFFECT_HEAL_HUMANS:
							if(random_prob(0.1f))
							{
								GameServer()->CreateDeath(TileCenter, m_Owner);
							}
							break;
						case GROWINGEXPLOSIONEFFECT_BOOM_INFECTED:
							if (random_prob(0.2f))
							{
								GameServer()->CreateExplosion(TileCenter, m_Owner, WEAPON_HAMMER, MAX_DAMAGE, true);
							}
							break;
						case GROWINGEXPLOSIONEFFECT_ELECTRIC_INFECTED:
							{
								vec2 EndPoint = m_SeedPos + vec2(32.0f*(i-m_MaxGrowing) - 16.0f + frandom()*32.0f, 32.0f*(j-m_MaxGrowing) - 16.0f + frandom()*32.0f);
								m_pGrowingMapVec[j*m_GrowingMap_Length+i] = EndPoint;
								
								int NumPossibleStartPoint = 0;
								vec2 PossibleStartPoint[4];
								
								if(FromLeft)
								{
									PossibleStartPoint[NumPossibleStartPoint] = m_pGrowingMapVec[j*m_GrowingMap_Length+i-1];
									NumPossibleStartPoint++;
								}
								if(FromRight)
								{
									PossibleStartPoint[NumPossibleStartPoint] = m_pGrowingMapVec[j*m_GrowingMap_Length+i+1];
									NumPossibleStartPoint++;
								}
								if(FromTop)
								{
									PossibleStartPoint[NumPossibleStartPoint] = m_pGrowingMapVec[(j-1)*m_GrowingMap_Length+i];
									NumPossibleStartPoint++;
								}
								if(FromBottom)
								{
									PossibleStartPoint[NumPossibleStartPoint] = m_pGrowingMapVec[(j+1)*m_GrowingMap_Length+i];
									NumPossibleStartPoint++;
								}
								
								if(NumPossibleStartPoint > 0)
								{
									int randNb = random_int_range(0, NumPossibleStartPoint-1);
									vec2 StartPoint = PossibleStartPoint[randNb];
									GameServer()->CreateLaserDotEvent(StartPoint, EndPoint, Server()->TickSpeed()/6);
								}
								
								if(random_prob(0.1f))
								{
									GameServer()->CreateSound(EndPoint, SOUND_LASER_BOUNCE);
								}
							}
							break;
					}
				}
			}
		}
	}
	
	if(NewTile)
	{
		switch(m_ExplosionEffect)
		{
			case GROWINGEXPLOSIONEFFECT_POISON_INFECTED:
				if(random_prob(0.1f))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);
				}
				break;
		}
	}
	
	// Find other players
	for(CCharacter *p = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
	{
		int tileX = m_MaxGrowing + static_cast<int>(round(p->GetPos().x))/32 - m_SeedX;
		int tileY = m_MaxGrowing + static_cast<int>(round(p->GetPos().y))/32 - m_SeedY;
		
		if(tileX < 0 || tileX >= m_GrowingMap_Length || tileY < 0 || tileY >= m_GrowingMap_Length)
			continue;
		
		if(m_Hit[p->GetPlayer()->GetCID()])
			continue;
		
		int k = tileY*m_GrowingMap_Length+tileX;

		if((m_pGrowingMap[k] >= 0) && p->IsHuman())
		{
			if(tick - m_pGrowingMap[k] < Server()->TickSpeed()/4)
			{
				switch(m_ExplosionEffect)
				{
					case GROWINGEXPLOSIONEFFECT_HEAL_HUMANS:
						p->IncreaseArmor(1);
						GameServer()->SendEmoticon(p->GetPlayer()->GetCID(), EMOTICON_EYES);
						m_Hit[p->GetPlayer()->GetCID()] = true;
						break;
				}
			}
		}

		if(p->IsHuman())
			continue;

		if(m_pGrowingMap[k] >= 0)
		{
			if(tick - m_pGrowingMap[k] < Server()->TickSpeed()/4)
			{
				switch(m_ExplosionEffect)
				{
					case GROWINGEXPLOSIONEFFECT_FREEZE_INFECTED:
						p->Freeze(3.0f, m_Owner, FREEZEREASON_FLASH);
						GameServer()->SendEmoticon(p->GetPlayer()->GetCID(), EMOTICON_QUESTION);
						m_Hit[p->GetPlayer()->GetCID()] = true;
						break;
					case GROWINGEXPLOSIONEFFECT_POISON_INFECTED:
						p->Poison(g_Config.m_InfPoisonDamage, m_Owner);
						GameServer()->SendEmoticon(p->GetPlayer()->GetCID(), EMOTICON_DROP);
						m_Hit[p->GetPlayer()->GetCID()] = true;
						break;
					case GROWINGEXPLOSIONEFFECT_ELECTRIC_INFECTED:
					{
						int Damage = 5+20*((float)(m_MaxGrowing - min(tick - m_StartTick, (int)m_MaxGrowing)))/(m_MaxGrowing);
						p->TakeDamage(normalize(p->GetPos() - m_SeedPos)*10.0f, m_Pos, Damage, m_Owner, WEAPON_HAMMER);
						m_Hit[p->GetPlayer()->GetCID()] = true;
						break;
					}
				}
			}
		}
	}
	
	// clean slug slime
	if (m_ExplosionEffect == GROWINGEXPLOSIONEFFECT_FREEZE_INFECTED) 
	{
		for(CEntity *e = (CEntity*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_SLUG_SLIME); e; e = (CEntity *)e->TypeNext())
		{
			int tileX = m_MaxGrowing + static_cast<int>(round(e->GetPos().x))/32 - m_SeedX;
			int tileY = m_MaxGrowing + static_cast<int>(round(e->GetPos().y))/32 - m_SeedY;
		
			if(tileX < 0 || tileX >= m_GrowingMap_Length || tileY < 0 || tileY >= m_GrowingMap_Length)
				continue;
				
			int k = tileY*m_GrowingMap_Length+tileX;
			if(m_pGrowingMap[k] >= 0)
			{
				if(tick - m_pGrowingMap[k] < Server()->TickSpeed()/4)
				{
					e->Reset();
				}
			}
		}
	}
}

void CGrowingExplosion::TickPaused()
{
	++m_StartTick;
}
