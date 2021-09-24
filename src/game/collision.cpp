/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/server/entities/harpoon.h>
#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

CCollision::CCollision()
{
	m_pCollisionTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers, void (*ptr)(float,float,float))
{
	//m_pWater = pWater;
	//pointer = ptr;
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile*>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	m_pCollisionTiles = new CTile[m_Width * m_Height];
	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;
		//if (Index > 128)
			//continue;
		switch(Index)
		{
		case TILE_DEATH:
			m_pCollisionTiles[i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pCollisionTiles[i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pCollisionTiles[i].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		case TILE_WATER:
			m_pCollisionTiles[i].m_Index = COLFLAG_WATER;
			break;
		default:
			m_pCollisionTiles[i].m_Index = 0;
		}
	}
	if (m_pLayers->WaterLayer())
	{
		CTile* m_pWaterTiles = static_cast<CTile*>(m_pLayers->Map()->GetData(m_pLayers->WaterLayer()->m_Data));
		for (int i = 0; i < m_Width * m_Height; i++)
		{
			int Index = m_pWaterTiles[i].m_Index;
			switch (Index)
			{
			case TILE_SOLID:
				m_pCollisionTiles[i].m_Index |= COLFLAG_WATER;
				break;
			default:
				m_pCollisionTiles[i].m_Index |= 0;
			}
		}
	}
}

int CCollision::GetTile(int x, int y) const
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pCollisionTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pCollisionTiles[Ny*m_Width+Nx].m_Index;
}

bool CCollision::IsTile(int x, int y, int Flag) const
{
	if (Flag == COLFLAG_AIR)
	{
		return !GetTile(x, y);
	}
	return GetTile(x, y) & Flag;
	//return GetTile(x, y)&Flag;
}

bool CCollision::CheckPoint(float x, float y, int Flag) const
{
	int Tx = round_to_int(x);
	int Ty = round_to_int(y);

	if (Flag == 8)
	{
		if (IsTile(Tx, Ty - 32, 8)) //water above carry on
		{
		}
		else
		{ //boi u done goofed
			if (Ty % 32 < 16)
			{
				return false;
			}
		}
	}
	else if (Flag == 0)
	{
		if ((!IsTile(Tx, Ty - 32, 8)) && IsTile(Tx, Ty, 8)) //water is here but not above
		{
			if (Ty % 32 < 16)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
		}
	}
	return IsTile(Tx, Ty, Flag);
}

bool CCollision::CheckPoint(vec2 Pos, int Flag) const
{
	return CheckPoint(Pos.x, Pos.y, Flag);
}
// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const
{
	const int End = distance(Pos0, Pos1)+1;
	const float InverseEnd = 1.0f/End;
	vec2 Last = Pos0;

	for(int i = 0; i <= End; i++)
	{
		vec2 Pos = mix(Pos0, Pos1, i*InverseEnd);
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
int CCollision::IntersectLineWithWater(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int Flag) const
{
	const int End = distance(Pos0, Pos1) + 1;
	const float InverseEnd = 1.0f / End;
	vec2 Last = Pos0;

	for (int i = 0; i <= End; i++)
	{
		vec2 Pos = mix(Pos0, Pos1, i * InverseEnd);
		if (CheckPoint(Pos.x, Pos.y))
		{
			if (pOutCollision)
				*pOutCollision = Pos;
			if (pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return 1;
				//GetCollisionAt(Pos.x, Pos.y);
		}
		else if (CheckPoint(Pos.x, Pos.y, Flag))
		{
			if (pOutCollision)
				*pOutCollision = Pos;
			if (pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return 2;
			//GetWaterCollisionAt(Pos.x, Pos.y, Flag);
		}
		Last = Pos;
	}
	if (pOutCollision)
		*pOutCollision = Pos1;
	if (pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const
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
void CCollision::Diffract(vec2* pInoutPos, vec2* pInoutVel, float Elasticity, int* pBounces, int Flag) const
{
	if (pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if (CheckPoint(Pos + Vel, Flag))
	{
		int Affected = 0;
		if (CheckPoint(Pos.x + Vel.x, Pos.y, Flag))
		{
			pInoutVel->x *= -Elasticity;
			pInoutPos->x += clamp(-Elasticity * Vel.x, -1.0f, 1.0f);
			if (pBounces)
				(*pBounces)++;
			Affected++;
		}

		if (CheckPoint(Pos.x, Pos.y + Vel.y, Flag))
		{
			pInoutVel->y *= -Elasticity;
			pInoutPos->y += clamp(-Elasticity * Vel.y, -1.0f, 1.0f);
			if (pBounces)
				(*pBounces)++;
			Affected++;
		}

		if (Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
			pInoutPos->y += clamp(-Elasticity * Vel.y, -1.0f, 1.0f);
			pInoutPos->x += clamp(-Elasticity * Vel.x, -1.0f, 1.0f);
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}


bool CCollision::TestBox(vec2 Pos, vec2 Size, int Flag, int CLAFlag) const
{
	Size *= 0.5f;
	if (CLAFlag & CLAFLAG_SOLID_WATER)
		Flag += COLFLAG_WATER;
	if (CLAFlag & CLAFLAG_FULLY_SUBMERGED)
	{
		if (CheckPoint(Pos.x - Size.x, Pos.y - Size.y, Flag) && CheckPoint(Pos.x + Size.x, Pos.y - Size.y, Flag) && CheckPoint(Pos.x - Size.x, Pos.y + Size.y, Flag) && CheckPoint(Pos.x + Size.x, Pos.y + Size.y, Flag))
			return true;
		return false;
	}
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y, Flag))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y, Flag))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y, Flag))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y, Flag))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath, int CLAFlag) const
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	const float Distance = length(Vel);
	const int Max = (int)Distance;

	if(pDeath)
		*pDeath = false;

	if(Distance > 0.00001f)
	{
		const float Fraction = 1.0f/(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			//You hit a deathtile, congrats to that :)
			//Deathtiles are a bit smaller
			if(pDeath && TestBox(vec2(NewPos.x, NewPos.y), Size*(2.0f/3.0f), COLFLAG_DEATH))
			{
				*pDeath = true;
			}

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
			if (pointer)
			{
				if (!TestBox(vec2(Pos.x, Pos.y), Size, COLFLAG_WATER) && TestBox(vec2(NewPos.x, NewPos.y), Size, COLFLAG_WATER))
				{
				//	m_pWater->HitWater(NewPos.x, NewPos.y, -pInoutVel->y);
					//CGameClient::WaterSplash(NewPos.x, NewPos.y, -pInoutVel->y, m_pWater);
					(*pointer)(NewPos.x, NewPos.y, -pInoutVel->y);
				}
				else if (TestBox(vec2(Pos.x, Pos.y), Size, COLFLAG_WATER) && !TestBox(vec2(NewPos.x, NewPos.y), Size, COLFLAG_WATER))
				{
					//m_pWater->HitWater(NewPos.x, NewPos.y, pInoutVel->y);
					(*pointer)(NewPos.x, NewPos.y, pInoutVel->y);
				}
			}
			
			Pos = NewPos;
		}
	}
	
	*pInoutPos = Pos;
	*pInoutVel = Vel;
}
void CCollision::MoveWaterBox(vec2* pInoutPos, vec2* pInoutVel, vec2 Size, float Elasticity, bool* pDeath, float Severity) const
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if (pDeath)
		*pDeath = false;

	if (Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f / (float)(Max + 1);
		for (int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel * Fraction; // TODO: this row is not nice

			//You hit a deathtile, congrats to that :)
			//Deathtiles are a bit smaller
			if (pDeath && TestBox(vec2(NewPos.x, NewPos.y), Size * (2.0f / 3.0f), COLFLAG_DEATH))
			{
				*pDeath = true;
			}
			//You are in water
			if (TestBox(vec2(NewPos.x, NewPos.y), Size * (2.0f / 3.0f), COLFLAG_WATER))
			{
				Vel.x *= Severity;
				Vel.y *= Severity;
			}
			if (TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if (TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if (TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if (Hits == 0)
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

/*
	function: Intersect Water
		Returns Position of where an entity hit water
*/

bool CCollision::IntersectWater(vec2 Pos0, vec2 Pos1, vec2* Position, vec2 Size)
{
	//const int End = distance(Pos0, Pos1) + 1;
	//const float InverseEnd = 1.0f / End;
	//vec2 Last = Pos0;

	const int End = distance(Pos0, Pos1) + 1;
	//float Distance = distance(Pos0, Pos1) + 1;
	//int Max = (int)Distance;
	//float Inverse = 1.0f / Max+1;
	const float InverseEnd = 1.0f / End;
	for (int i = 0; i <= End +1; i++)
	{
		vec2 NewPos = mix(Pos0, Pos1, i * InverseEnd);

		if (TestBox(vec2(NewPos.x, NewPos.y), Size * (2.0f / 3.0f), COLFLAG_WATER))
		{
			*Position = NewPos;
			return true;
		}
	}
	return false;
}