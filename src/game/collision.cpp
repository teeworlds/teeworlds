/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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
	m_pTeleporter = 0;
	m_MainTiles = false;
	m_StopTiles = false;
}

CCollision::~CCollision()
{
	if(m_pTeleporter)
		delete[] m_pTeleporter;
}

void CCollision::Init(class CLayers *pLayers)
{
	// reset race specific pointers
	m_pTele = 0;
	m_pSpeedup = 0;
	m_MainTiles = false;
	m_StopTiles = false;

	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	if(m_pLayers->TeleLayer())
	{
		unsigned Size = m_pLayers->Map()->GetUncompressedDataSize(m_pLayers->TeleLayer()->m_Tele);
		if(Size == m_Width*m_Height * sizeof(CTeleTile))
			m_pTele = static_cast<CTeleTile *>(m_pLayers->Map()->GetData(m_pLayers->TeleLayer()->m_Tele));
	}

	if(m_pLayers->SpeedupLayer())
	{
		unsigned Size = m_pLayers->Map()->GetUncompressedDataSize(m_pLayers->SpeedupLayer()->m_Speedup);
		if(Size == m_Width*m_Height * sizeof(CSpeedupTile))
			m_pSpeedup = static_cast<CSpeedupTile *>(m_pLayers->Map()->GetData(m_pLayers->SpeedupLayer()->m_Speedup));
	}

	InitTeleporter();

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
		if(Index >= TILE_STOPL && Index <= 59)
			m_pTiles[i].m_Index = Index;
		if(Index >= TILE_BEGIN && Index <= 59)
			m_MainTiles = true;
		if(Index >= TILE_STOPL && Index <= TILE_STOPT)
			m_StopTiles = true;
	}
}

void CCollision::InitTeleporter()
{
	if(m_pTeleporter)
		delete[] m_pTeleporter;
	
	int ArraySize = 0;
	if(m_pTele)
	{
		for(int i = 0; i < m_pLayers->TeleLayer()->m_Width * m_pLayers->TeleLayer()->m_Height; i++)
		{
			// get the array size
			if(m_pTele[i].m_Number > ArraySize)
				ArraySize = m_pTele[i].m_Number;
		}
	}

	if(!ArraySize)
	{
		m_pTeleporter = 0x0;
		return;
	}

	m_pTeleporter = new vec2[ArraySize];
	mem_zero(m_pTeleporter, ArraySize*sizeof(vec2));

	// assign the values
	for(int i = 0; i < m_pLayers->TeleLayer()->m_Width * m_pLayers->TeleLayer()->m_Height; i++)
	{
		if(m_pTele[i].m_Type == TILE_TELEOUT && m_pTele[i].m_Number > 0)
			m_pTeleporter[m_pTele[i].m_Number - 1] = vec2(i % m_pLayers->TeleLayer()->m_Width * 32 + 16, i / m_pLayers->TeleLayer()->m_Width * 32 + 16);
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	if(m_pTiles[Ny*m_Width+Nx].m_Index == COLFLAG_SOLID || m_pTiles[Ny*m_Width+Nx].m_Index == (COLFLAG_SOLID|COLFLAG_NOHOOK) || m_pTiles[Ny*m_Width+Nx].m_Index == COLFLAG_DEATH)
		return m_pTiles[Ny*m_Width+Nx].m_Index;
	else
		return 0;
}

bool CCollision::IsTileSolid(int x, int y)
{
	return GetTile(x, y)&COLFLAG_SOLID;
}

// race
int CCollision::GetTilePos(vec2 Pos)
{
	int Nx = clamp((int)Pos.x/32, 0, m_Width-1);
	int Ny = clamp((int)Pos.y/32, 0, m_Height-1);
	
	return Ny*m_Width+Nx;
}

vec2 CCollision::GetPos(int TilePos)
{
	int x = TilePos%m_Width;
	int y = TilePos/m_Width;
	
	return vec2(x*32+16, y*32+16);
}

int CCollision::GetIndex(vec2 Pos)
{
	return m_pTiles[GetTilePos(Pos)].m_Index;
}

int CCollision::GetIndex(int TilePos)
{
	if(TilePos < 0)
		return -1;
	return m_pTiles[TilePos].m_Index;
}

bool CCollision::IsRaceTile(int TilePos, int Mask)
{
	if(Mask&RACECHECK_TILES_MAIN && m_pTiles[TilePos].m_Index >= TILE_BEGIN && m_pTiles[TilePos].m_Index <= 59)
		return true;
	if(Mask&RACECHECK_TILES_STOP && m_pTiles[TilePos].m_Index >= TILE_STOPL && m_pTiles[TilePos].m_Index <= TILE_STOPT)
		return true;
	if(Mask&RACECHECK_TELE && m_pTele[TilePos].m_Type == TILE_TELEIN)
		return true;
	if(Mask&RACECHECK_SPEEDUP && m_pSpeedup[TilePos].m_Force > 0)
		return true;
	return false;
}

// TODO: handle multiple tiles at once
int CCollision::CheckRaceTile(vec2 PrevPos, vec2 Pos, int Mask)
{
	if(Mask&RACECHECK_TILES_MAIN && !m_MainTiles)
		Mask ^= RACECHECK_TILES_MAIN;
	if(Mask&RACECHECK_TILES_STOP && !m_StopTiles)
		Mask ^= RACECHECK_TILES_STOP;
	if(Mask&RACECHECK_TELE && !m_pTele)
		Mask ^= RACECHECK_TELE;
	if(Mask&RACECHECK_SPEEDUP && !m_pSpeedup)
		Mask ^= RACECHECK_SPEEDUP;

	if(!Mask)
		return -1;

	float Distance = distance(PrevPos, Pos);
	int End = Distance+1;

	for(int i = 0; i <= End; i++)
	{
		float a = i/float(End);
		vec2 Tmp = mix(PrevPos, Pos, a);
		int TilePos = GetTilePos(Tmp);
		if(IsRaceTile(TilePos, Mask))
			return TilePos;
	}

	return -1;
}

int CCollision::CheckTeleport(int TilePos)
{
	if(!m_pTele || TilePos < 0 || m_pTele[TilePos].m_Type != TILE_TELEIN)
		return 0;
	return m_pTele[TilePos].m_Number;
}

vec2 CCollision::GetTeleportDestination(int Number)
{
	if(m_pTeleporter && Number > 0)
		return m_pTeleporter[Number - 1];
	return vec2(0,0);
}

int CCollision::CheckSpeedup(int TilePos)
{
	if(!m_pSpeedup || TilePos < 0)
		return -1;
	if(m_pSpeedup[TilePos].m_Force > 0)
		return TilePos;
	return -1;
}

void CCollision::GetSpeedup(int SpeedupPos, vec2 *Dir, int *Force)
{
	float Angle = m_pSpeedup[SpeedupPos].m_Angle * (pi/180.0f);
	*Force = m_pSpeedup[SpeedupPos].m_Force;
	*Dir = vec2(cos(Angle), sin(Angle));
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
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
