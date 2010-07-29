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
	m_pFTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
	isOneLayer = false;
}
int CCollision::IsSolid(int x, int y)
{
	return (GetTile(x,y)&COLFLAG_SOLID);
} 

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	if(m_pLayers->FGameLayer() != 0) {
		m_pFTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->FGameLayer()->m_Data));
		isOneLayer = false;
	} else {
		m_pFTiles = 0;
		isOneLayer = true;
	}
	//DDRace
	mem_zero(&m_Len, sizeof(m_Len));
	mem_zero(&m_Tele, sizeof(m_Tele));
	m_pTeleporter = new int[m_Width * m_Height];
	m_pFTeleporter = new int[m_Width * m_Height];
	
	
	for(int i = m_Width * m_Height - 1; i >= 0; i--)
	{
		if(m_pTiles[i].m_Index > 34 && m_pTiles[i].m_Index < 190)
		{
			if(m_pTiles[i].m_Index & 1)
				m_Len[m_pTiles[i].m_Index >> 1]++;
			else if(!(m_pTiles[i].m_Index & 1))
				m_Tele[(m_pTiles[i].m_Index - 1) >> 1]++;
		}
	}
	for(int i = 0; i < 95; i++)
	{
		m_pDest[i] = new int[m_Len[i]];
		m_Len[i] = 0;
	}
	for(int i = m_Width * m_Height - 1; i >= 0; i--)
	{
		if(m_pTiles[i].m_Index & 1 && m_pTiles[i].m_Index > 34 && m_pTiles[i].m_Index < 190)
			m_pDest[m_pTiles[i].m_Index >> 1][m_Len[m_pTiles[i].m_Index >> 1]++] = i;
	}  
	
	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;
		int FIndex = 0; 
		if(!isOneLayer)
			FIndex = m_pFTiles[i].m_Index;
		if (FIndex!=0)
			dbg_msg ("flayer", "tile found at (%d, %d)",(i - (i/ m_Width) * m_Width) ,(i / m_Width));
		if(Index > 190)
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
			m_pTeleporter[i] = Index;
		}
		m_pFTeleporter[i] = FIndex;
	}
}

int CCollision::GetIndex(int x, int y, bool flayer) {
   CTile *tiles = (flayer) ? m_pFTiles: m_pTiles;  		 
   int index = tiles[y*m_Width+x].m_Index;  		 
   return index-ENTITY_OFFSET;  		 
} 


int CCollision::GetTile(int x, int y)
{
	int nx = clamp(x/32, 0, m_Width-1);
	int ny = clamp(y/32, 0, m_Height-1);
	
	return m_pTiles[ny*m_Width+nx].m_Index > 128 ? 0 : m_pTiles[ny*m_Width+nx].m_Index;
}

bool CCollision::IsTileSolid(int x, int y)
{
	return (GetTile(x,y)&COLFLAG_SOLID);
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

int CCollision::IsNolaser(int x, int y)  		 
{  		 
   return (CCollision::GetTile(x,y) & COLFLAG_NOLASER);  		 
}  		 
 		 
//DDRace  		 
int CCollision::IsTeleport(int x, int y)  		 
{  		 
	int nx = x/32;  		 
	int ny = y/32;  		 
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)  		 
		return 0;  		 
	int z = m_pTeleporter[ny * m_Width + nx]-1;
	if(z > 34 && z < 190 && z & 1)
		return z >> 1;
	return 0;  		 
}  		 
 		 
int CCollision::IsBegin(int x, int y)  		 
{  		 
   int nx = x/32;  		 
   int ny = y/32;  		 
   if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)  		 
       return 0;  		 
 		 
   return m_pTeleporter[ny*m_Width+nx] == TILE_BEGIN;  		 
}

int CCollision::IsEnd(int x, int y)  		 
{  		 
	int nx = x/32;
	int ny = y/32;
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)
		return 0;
	return m_pTeleporter[ny * m_Width + nx] == TILE_END;  		 
}  		 
 		 
int CCollision::IsFreeze(int x, int y)  		 
{  		 
	int nx = x/32;
	int ny = y/32;
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)
		return 0;
   return (m_pTeleporter[ny * m_Width + nx] == TILE_FREEZE) || (m_pFTeleporter[ny * m_Width + nx] == TILE_FREEZE);
}  		 
 		 
int CCollision::IsUnfreeze(int x, int y)  		 
{  		 
	int nx = x/32;
	int ny = y/32;
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)
		return 0;
   return (m_pTeleporter[ny * m_Width + nx] == TILE_UNFREEZE) || (m_pFTeleporter[ny * m_Width + nx] == TILE_UNFREEZE);
}   		 
 		 
int CCollision::IsKick(int x, int y)  		 
{  		 
	int nx = x/32;
	int ny = y/32;
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)
		return 0;
   return (m_pTeleporter[ny * m_Width + nx] == TILE_KICK) || (m_pFTeleporter[ny * m_Width + nx] == TILE_KICK);
} 		 
 		 
int CCollision::IsCp(int x, int y)  		 
{  		 
	int nx = x/32;
	int ny = y/32;
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)
		return 0;
	int ind = m_pTeleporter[ny * m_Width + nx];
	if (ind >= TILE_CP_D && ind <= TILE_CP_L_F)
		return ind;
	return 0;
}

int CCollision::IsBoost(int x, int y)  		 
{  		 
	int nx = x/32;
	int ny = y/32;
	if(y < 0 || nx < 0 || nx >= m_Width || ny >= m_Height)
		return 0;
   if ((m_pTeleporter[ny * m_Width + nx] >= TILE_BOOST_L && m_pTeleporter[ny * m_Width + nx] <= TILE_BOOST_U) || (m_pTeleporter[ny * m_Width + nx] >= TILE_BOOST_L2 && m_pTeleporter[ny * m_Width + nx] <= TILE_BOOST_U2))  		 
       return m_pTeleporter[ny * m_Width + nx];  		 
   else if ((m_pFTeleporter[ny * m_Width + nx] >= TILE_BOOST_L && m_pFTeleporter[ny * m_Width + nx]<= TILE_BOOST_U) || (m_pFTeleporter[ny * m_Width + nx] >= TILE_BOOST_L2 && m_pFTeleporter[ny * m_Width + nx] <= TILE_BOOST_U2))  		 
       return m_pFTeleporter[ny * m_Width + nx];  		 
   return 0;  		 
}  		 
 		 
vec2 CCollision::CpSpeed(int index)  		 
{  		 
 		 
   vec2 target;  		 
 		 
   switch(index)  		 
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
   if (index>=TILE_CP_D_F && index<=TILE_CP_L_F)  		 
       target*=4;  		 
   return target;  		 
 		 
}  		 
 		 
 		 
vec2 CCollision::BoostAccel(int index)  		 
{  		 
   if (index==TILE_BOOST_L)  		 
       return vec2(-3,0);  		 
   else if(index==TILE_BOOST_R)  		 
       return vec2(3,0);  		 
   else if(index==TILE_BOOST_D)  		 
       return vec2(0,2);  		 
   else if(index==TILE_BOOST_U)  		 
       return vec2(0,-2);  		 
   else if(index==TILE_BOOST_L2)  		 
       return vec2(-15,0);  		 
   else if(index==TILE_BOOST_R2)  		 
       return vec2(15,0);  		 
   else if(index==TILE_BOOST_D2)  		 
       return vec2(0,15);  		 
   else if(index==TILE_BOOST_U2)  		 
       return vec2(0,-15);  		 
 		 
   return vec2(0,0);  		 
}  		 
 		 
 		 
vec2 CCollision::Teleport(int a)  		 
{  		 
   if(m_Len[a] > 0)  		 
   {  		 
       int r = rand()%m_Len[a];  		 
       int x = (m_pDest[a][r] % m_Width)<<5;  		 
       int y = (m_pDest[a][r] / m_Width)<<5;  		 
       return vec2((float)x+16.0, (float)y+16.0);  		 
   }  		 
   else  		 
       return vec2(0, 0);  		 
}
 		 
void CCollision::Set(int x, int y, int flag)  		 
{  		 
   int nx = clamp(x/32, 0, m_Width-1);  		 
   int ny = clamp(y/32, 0, m_Height-1);  		 
 		 
   m_pTiles[ny * m_Width + nx].m_Index = flag;  		 
} 

 		 
int CCollision::IntersectNolaser2(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision)  		 
{  		 
   float d = distance(pos0, pos1);  		 
   vec2 last = pos0;  		 
 		 
   for(float f = 0; f < d; f++)  		 
   {  		 
       float a = f/d;  		 
       vec2 pos = mix(pos0, pos1, a);  		 
       if(CCollision::IsNolaser(round(pos.x), round(pos.y)))  		 
       {  		 
           if(out_collision)  		 
               *out_collision = pos;  		 
           if(out_before_collision)  		 
               *out_before_collision = last;  		 
           return GetTile(round(pos.x), round(pos.y));  		 
       }  		 
       last = pos;  		 
   }  		 
   if(out_collision)  		 
       *out_collision = pos1;  		 
   if(out_before_collision)  		 
       *out_before_collision = pos1;  		 
   return 0;  		 
}  		 
 		 
int CCollision::IntersectNolaser(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision)  		 
{  		 
   float d = distance(pos0, pos1);  		 
   vec2 last = pos0;  		 
 		 
   for(float f = 0; f < d; f++)  		 
   {  		 
       float a = f/d;  		 
       vec2 pos = mix(pos0, pos1, a);  		 
       if(IsNolaser(round(pos.x), round(pos.y)) || IsSolid(round(pos.x), round(pos.y)))  		 
       {  		 
           if(out_collision)  		 
               *out_collision = pos;  		 
           if(out_before_collision)  		 
               *out_before_collision = last;  		 
           return GetTile(round(pos.x), round(pos.y));  		 
       }  		 
       last = pos;  		 
   }  		 
   if(out_collision)  		 
       *out_collision = pos1;  		 
   if(out_before_collision)  		 
       *out_before_collision = pos1;  		 
   return 0;  		 
}  		 
 		 
int CCollision::IntersectAir(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision)  		 
{  		 
   float d = distance(pos0, pos1);  		 
   vec2 last = pos0;  		 
 		 
   for(float f = 0; f < d; f++)  		 
   {  		 
       float a = f/d;  		 
       vec2 pos = mix(pos0, pos1, a);  		 
       if(IsSolid(round(pos.x), round(pos.y)) || !GetTile(round(pos.x), round(pos.y)))  		 
       {  		 
           if(out_collision)  		 
               *out_collision = pos;  		 
           if(out_before_collision)  		 
               *out_before_collision = last;  		 
           if(!GetTile(round(pos.x), round(pos.y)))  		 
               return -1;  		 
           else  		 
               return GetTile(round(pos.x), round(pos.y));  		 
       }  		 
       last = pos;  		 
   }  		 
   if(out_collision)  		 
       *out_collision = pos1;  		 
   if(out_before_collision)  		 
       *out_before_collision = pos1;  		 
   return 0;  		 
}  		 
 
