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
	m_pFront = 0;
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
	if(m_pLayers->SwitchLayer())
		m_pSwitch = static_cast<CTeleTile *>(m_pLayers->Map()->GetData(m_pLayers->SwitchLayer()->m_Switch));
	if(m_pLayers->FrontLayer())
	{
		m_pFront = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->FrontLayer()->m_Front));
		for(int i = 0; i < m_Width*m_Height; i++)
			{
				int Index = m_pFront[i].m_Index;
				if(Index > TILE_NPH)
					continue;

				switch(Index)
				{
				case TILE_DEATH:
					m_pFront[i].m_Index = COLFLAG_DEATH;
					break;
				case TILE_SOLID:
					m_pFront[i].m_Index = 0;
					break;
				case TILE_NOHOOK:
					m_pFront[i].m_Index = 0;
					break;
				case TILE_NOLASER:
					m_pFront[i].m_Index = COLFLAG_NOLASER;
					break;
				default:
					m_pFront[i].m_Index = 0;
				}

				// DDRace tiles
				if(Index >= 6 && Index<=TILE_NPH)
					m_pFront[i].m_Index = Index;
			}
	}

	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;
		if(Index > TILE_NPH)
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
		case TILE_NOLASER:
			m_pTiles[i].m_Index = COLFLAG_NOLASER;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}

		// DDRace tiles
		if(Index >= 6 && Index<=TILE_NPH)
			m_pTiles[i].m_Index = Index;
	}
}

int CCollision::GetMapIndex(vec2 PrevPos, vec2 Pos)
{
	int Index = 0;
	float d = distance(PrevPos, Pos);

	if(!d)
	{
		int nx = clamp((int)Pos.x/32, 0, m_Width-1);
		int ny = clamp((int)Pos.y/32, 0, m_Height-1);
		/*if (m_pTele && (m_pTele[ny*m_Width+nx].m_Type == TILE_TELEIN)) dbg_msg("m_pTele && TELEIN","ny*m_Width+nx %d",ny*m_Width+nx);
		else if (m_pTele && m_pTele[ny*m_Width+nx].m_Type==TILE_TELEOUT) dbg_msg("TELEOUT","ny*m_Width+nx %d",ny*m_Width+nx);
		else dbg_msg("GetMapIndex(","ny*m_Width+nx %d",ny*m_Width+nx);//REMOVE */

		if((m_pTiles[ny*m_Width+nx].m_Index >= TILE_THROUGH && m_pTiles[ny*m_Width+nx].m_Index < TILE_TELEIN) ||
			((m_pTiles[ny*m_Width+nx].m_Index >TILE_BOOST)&&(m_pTiles[ny*m_Width+nx].m_Index <= TILE_NPH) ) ||
			(m_pFront && (m_pFront[ny*m_Width+nx].m_Index >= TILE_THROUGH && m_pFront[ny*m_Width+nx].m_Index < TILE_TELEIN)) ||
			(m_pFront && ((m_pFront[ny*m_Width+nx].m_Index >TILE_BOOST)&&(m_pFront[ny*m_Width+nx].m_Index <= TILE_NPH))) ||
			(m_pTele && (m_pTele[ny*m_Width+nx].m_Type == TILE_TELEIN || m_pTele[ny*m_Width+nx].m_Type == TILE_TELEOUT)) ||
			(m_pSpeedup && m_pSpeedup[ny*m_Width+nx].m_Force > 0))
		{
			return ny*m_Width+nx;
		}
	}

	float a = 0.0f;
	vec2 Tmp = vec2(0, 0);
	int nx = 0;
	int ny = 0;

	for(float f = 0; f < d; f++)
	{
		a = f/d;
		Tmp = mix(PrevPos, Pos, a);
		nx = clamp((int)Tmp.x/32, 0, m_Width-1);
		ny = clamp((int)Tmp.y/32, 0, m_Height-1);
		if((m_pTiles[ny*m_Width+nx].m_Index >= TILE_THROUGH && m_pTiles[ny*m_Width+nx].m_Index < TILE_TELEIN) ||
			((m_pTiles[ny*m_Width+nx].m_Index >TILE_BOOST)&&(m_pTiles[ny*m_Width+nx].m_Index <= TILE_NPH) ) ||
			(m_pFront && (m_pFront[ny*m_Width+nx].m_Index >= TILE_THROUGH && m_pFront[ny*m_Width+nx].m_Index < TILE_TELEIN)) ||
			(m_pFront && ((m_pFront[ny*m_Width+nx].m_Index >TILE_BOOST)&&(m_pFront[ny*m_Width+nx].m_Index <= TILE_NPH))) ||
			(m_pTele && (m_pTele[ny*m_Width+nx].m_Type == TILE_TELEIN || m_pTele[ny*m_Width+nx].m_Type == TILE_TELEOUT)) ||
			(m_pSpeedup && m_pSpeedup[ny*m_Width+nx].m_Force > 0))
		{
			return ny*m_Width+nx;
		}
	}

	return -1;
}

vec2 CCollision::GetPos(int Index)
{
	int x = Index%m_Width;
	int y = Index/m_Width;

	return vec2(x, y);
}

int CCollision::GetCollisionDDRace(int Index)
{
	/*dbg_msg("GetCollisionDDRace","m_pTiles[%d].m_Index = %d",Index,m_pTiles[Index].m_Index);//Remove*/
	if(Index < 0)
		return 0;
	return m_pTiles[Index].m_Index;
}
int CCollision::GetFCollisionDDRace(int Index)
{
	/*dbg_msg("GetFCollisionDDRace","m_pFront[%d].m_Index = %d",Index,m_pFront[Index].m_Index);//Remove*/

	if(Index < 0 || !m_pFront)
		return 0;
	return m_pFront[Index].m_Index;
}

int CCollision::GetTile(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	/*dbg_msg("GetTile","m_Index %d",m_pTiles[ny*m_Width+nx].m_Index);//Remove */
	if(m_pTiles[ny*m_Width+nx].m_Index == COLFLAG_SOLID
		|| m_pTiles[ny*m_Width+nx].m_Index == (COLFLAG_SOLID|COLFLAG_NOHOOK)
		|| m_pTiles[ny*m_Width+nx].m_Index == COLFLAG_DEATH
		|| m_pTiles[ny*m_Width+nx].m_Index == COLFLAG_NOLASER)
		return m_pTiles[ny*m_Width+nx].m_Index;
	else
		return 0;
}

int CCollision::GetFTile(int x, int y)
{
	if(!m_pFront)
	return 0;
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	/*dbg_msg("GetFTile","m_Index %d",m_pFront[ny*m_Width+nx].m_Index);//Remove */
	if(m_pFront[ny*m_Width+nx].m_Index == COLFLAG_DEATH
		|| m_pFront[ny*m_Width+nx].m_Index == COLFLAG_NOLASER)
		return m_pFront[ny*m_Width+nx].m_Index;
	else
		return 0;
}

int CCollision::Entity(int x, int y, bool Front)
{
	//if(0 < x || x >= m_Width) return 0;
	//if(0 < y || y >= m_Height) return 0;
	int Index = Front?m_pFront[y*m_Width+x].m_Index:m_pTiles[y*m_Width+x].m_Index;
	return Index-ENTITY_OFFSET;
}
void CCollision::SetCollisionAt(float x, float y, int flag)
{
   int nx = clamp(round(x)/32, 0, m_Width-1);
   int ny = clamp(round(y)/32, 0, m_Height-1);

   m_pTiles[ny * m_Width + nx].m_Index = flag;
}

// TODO: rewrite this smarter!
void ThroughOffset(vec2 Pos0, vec2 Pos1, int *Ox, int *Oy)
{
	float x = Pos0.x - Pos1.x;
	float y = Pos0.y - Pos1.y;
	if (fabs(x) > fabs(y))
	{
		if (x < 0)
		{
			*Ox = -32;
			*Oy = 0;
		}
		else
		{
			*Ox = 32;
			*Oy = 0;
		}
	}
	else
	{
		if (y < 0)
		{
			*Ox = 0;
			*Oy = -32;
		}
		else
		{
			*Ox = 0;
			*Oy = 32;
		}
	}
}

int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool AllowThrough)
{
	float d = distance(Pos0, Pos1);
	int End(d+1);
	vec2 Last = Pos0;
	int ix, iy; // Temporary position for checking collision
	int dx, dy; // Offset for checking the "through" tile
	if (AllowThrough)
		{
		ThroughOffset(Pos0, Pos1, &dx, &dy);
		}
	for(int i = 0; i < End; i++)
	{
		float a = i/d;
		vec2 Pos = mix(Pos0, Pos1, a);
		ix = round(Pos.x);
		iy = round(Pos.y);
		if(CheckPoint(ix, iy) && !(AllowThrough && IsThrough(ix + dx, iy + dy)))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(ix, iy);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

int CCollision::IntersectNoLaser(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float d = distance(Pos0, Pos1);
	vec2 Last = Pos0;

	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(IsSolid(round(Pos.x), round(Pos.y)) || (IsNoLaser(round(Pos.x), round(Pos.y)) || IsFNoLaser(round(Pos.x), round(Pos.y))))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			if (IsFNoLaser(round(Pos.x), round(Pos.y)))	return GetFCollisionAt(Pos.x, Pos.y);
			else return GetCollisionAt(Pos.x, Pos.y);
			
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

int CCollision::IntersectNoLaserNW(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float d = distance(Pos0, Pos1);
	vec2 Last = Pos0;

	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(IsNoLaser(round(Pos.x), round(Pos.y)) || IsFNoLaser(round(Pos.x), round(Pos.y)))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			if(IsNoLaser(round(Pos.x), round(Pos.y))) return GetCollisionAt(Pos.x, Pos.y);
			else return  GetFCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

int CCollision::IntersectAir(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float d = distance(Pos0, Pos1);
	vec2 Last = Pos0;

	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(IsSolid(round(Pos.x), round(Pos.y)) || (!GetTile(round(Pos.x), round(Pos.y)) && !GetFTile(round(Pos.x), round(Pos.y))))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			if(!GetTile(round(Pos.x), round(Pos.y)) && !GetFTile(round(Pos.x), round(Pos.y)))
				return -1;
			else
				if (!GetTile(round(Pos.x), round(Pos.y))) return GetTile(round(Pos.x), round(Pos.y));
				else return GetFTile(round(Pos.x), round(Pos.y));
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

int CCollision::IsSolid(int x, int y)
{
	return (GetTile(x,y)&COLFLAG_SOLID);
}

int CCollision::IsThrough(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	int Index = m_pTiles[ny*m_Width+nx].m_Index;
	int Findex;
	if (m_pFront)
		Findex = m_pFront[ny*m_Width+nx].m_Index;
	if (Index == TILE_THROUGH)
		return Index;
	else if (Findex == TILE_THROUGH)
		return Findex;
	else
		return 0;
}

int CCollision::IsNoLaser(int x, int y)
{
   return (CCollision::GetTile(x,y) & COLFLAG_NOLASER);
}

int CCollision::IsFNoLaser(int x, int y)
{
   return (CCollision::GetFTile(x,y) & COLFLAG_NOLASER);
}

//DDRace
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

bool CCollision::IsSpeedup(int x, int y)
{
	if(!m_pSpeedup)
		return false;

	/*dbg_msg("test", "test");//REMOVE*/
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

int CCollision::IsCp(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	int Index = m_pTiles[ny*m_Width+nx].m_Index;
	if (Index >= TILE_CP_D && Index <= TILE_CP_L_F)
		return Index;
	else
		return 0;
}

int CCollision::IsCheckpoint(int Index)
{
	if(Index < 0)
		return -1;
		
	int z = m_pTiles[Index].m_Index;
	if(z >= 35 && z <= 59)
		return z-35;
	return -1;
}

vec2 CCollision::CpSpeed(int Index)
{

   vec2 target;

   switch(Index)
   {
   case TILE_CP_U:
   case TILE_CP_U_F:
       target.x=0;
       target.y=-4;
       break;
   case TILE_CP_R:
   case TILE_CP_R_F:
       target.x=4;
       target.y=0;
       break;
   case TILE_CP_D:
   case TILE_CP_D_F:
       target.x=0;
       target.y=4;
       break;
   case TILE_CP_L:
   case TILE_CP_L_F:
       target.x=-4;
       target.y=0;
       break;
   default:
       target=vec2(0,0);
       break;
   }
   if (Index>=TILE_CP_D_F && Index<=TILE_CP_L_F)
       target*=4;
   return target;

}
