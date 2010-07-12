// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
	m_pTele = 0;
	m_pSpeedup = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	if(m_pLayers->TeleLayer())
		m_pTele = static_cast<CTeleTile *>(m_pLayers->Map()->GetData(m_pLayers->TeleLayer()->m_Tele));
	if(m_pLayers->SpeedupLayer())
		m_pSpeedup = static_cast<CSpeedupTile *>(m_pLayers->Map()->GetData(m_pLayers->SpeedupLayer()->m_Speedup));
	
	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;
		
		if(Index > 128)
			continue;
		
		switch(Index)
		{
		case TILE_DEATH:
			m_pTiles[i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pTiles[i].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}
		
		// race tiles
		if(Index >= 28 && Index <= 59)
			m_pTiles[i].m_Index = Index;
	}
}

int CCollision::GetTile(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	
	if(m_pTiles[ny*m_Width+nx].m_Index == COLFLAG_SOLID || m_pTiles[ny*m_Width+nx].m_Index == (COLFLAG_SOLID|COLFLAG_NOHOOK) || m_pTiles[ny*m_Width+nx].m_Index == COLFLAG_DEATH)
		return m_pTiles[ny*m_Width+nx].m_Index;
	else
		return 0;
}

bool CCollision::IsTileSolid(int x, int y)
{
	return GetTile(x,y)&COLFLAG_SOLID;
}

// race
int CCollision::GetIndex(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	
	return m_pTiles[ny*m_Width+nx].m_Index;
}

int CCollision::IsTeleport(int x, int y)
{
	if(!m_pTele)
		return 0;
	
	int nx = clamp(x/32, 0, m_pLayers->TeleLayer()->m_Width-1);
	int ny = clamp(y/32, 0, m_pLayers->TeleLayer()->m_Height-1);
	
	/*int z = m_pTiles[ny*m_Width+nx].m_Index-1;
	if(z > 34 && z <= 34 + 50 && z&1)
		return z;
	return 0;*/
	
	int Tele = 0;
	if(m_pTele[ny*m_pLayers->TeleLayer()->m_Width+nx].m_Type == TILE_TELEIN)
		Tele = m_pTele[ny*m_pLayers->TeleLayer()->m_Width+nx].m_Number;
		
	return Tele;
}

int CCollision::IsCheckpoint(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	
	int z = m_pTiles[ny*m_Width+nx].m_Index;
	if(z >= 35 && z <= 59)
		return z-35;
	return -1;
}

bool CCollision::IsSpeedup(int x, int y)
{
	if(!m_pSpeedup)
		return false;
	
	dbg_msg("test", "test");
	int nx = clamp(x/32, 0, m_pLayers->SpeedupLayer()->m_Width-1);
	int ny = clamp(y/32, 0, m_pLayers->SpeedupLayer()->m_Height-1);
	
	if(m_pSpeedup[ny*m_pLayers->SpeedupLayer()->m_Width+nx].m_Force > 0)
		return true;
		
	return false;
}

void CCollision::GetSpeedup(int x, int y, vec2 *Dir, int *Force)
{
	int nx = clamp(x/32, 0, m_pLayers->SpeedupLayer()->m_Width-1);
	int ny = clamp(y/32, 0, m_pLayers->SpeedupLayer()->m_Height-1);
	
	vec2 Direction = vec2(1, 0);
	float Angle = m_pSpeedup[ny*m_pLayers->SpeedupLayer()->m_Width+nx].m_Angle * (3.14159265f/180.0f);
	*Force = m_pSpeedup[ny*m_pLayers->SpeedupLayer()->m_Width+nx].m_Force;
	
	vec2 TmpDir;
	TmpDir.x = (Direction.x*cos(Angle)) - (Direction.y*sin(Angle));
	TmpDir.y = (Direction.x*sin(Angle)) + (Direction.y*cos(Angle));
	*Dir = TmpDir;
}
	
// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float d = distance(Pos0, Pos1);
	vec2 Last = Pos0;
	
	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;
	
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;			
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;			
			Affected++;
		}
		
		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	
	float Distance = length(Vel);
	int Max = (int)Distance;
	
	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;
			
			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice
			
			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;
				
				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}
				
				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}
				
				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}
			
			Pos = NewPos;
		}
	}
	
	*pInoutPos = Pos;
	*pInoutVel = Vel;
}
