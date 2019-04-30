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
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

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
	}
}

int CCollision::GetTile(int x, int y) const
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
}

bool CCollision::IsTileSolid(int x, int y) const
{
	return GetTile(x, y)&COLFLAG_SOLID;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i <= End; i++)
	{
		float a = i/float(End);
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

bool CCollision::TestBox(vec2 Pos, vec2 Size) const
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

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity) const
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

bool CCollision::HitTileDeath(const vec2& Pos, const vec2& Newpos, vec2* pDeathpos, float radius)
{

    vec2 Dir, Orth, Deathtilepos1, Deathtilepos2;

    //calculathe vector, that is orthogonal to the movement direction
    Dir = Pos-Newpos;
    Orth = normalize(vec2(Dir.y, -Dir.x)) * radius;
    Dir = normalize(Dir) * radius;

    //check if a hit with a deathtile appears within a line right and left parallel to the movement direction
    if(CheckDeath(Pos+Orth, Newpos+Orth+Dir, &Deathtilepos1, radius) || CheckDeath(Pos-Orth, Newpos-Orth-Dir, &Deathtilepos2, radius))
    {
    	//Calulate exact deathposition for up to 2 deathtiles
    	vec2 Deathpos1;
    	vec2 Deathpos2;
		CalcDeathPos(Pos, Newpos, Deathtilepos1, &Deathpos1, radius);
		CalcDeathPos(Pos, Newpos, Deathtilepos2, &Deathpos2, radius);

        //The Closer hit kills the Tee
        if(distance(Pos, Deathpos1) > distance(Pos, Deathpos2))
            *pDeathpos = Deathpos2;
        else
            *pDeathpos = Deathpos1;
        return true;
    }
    return false;
}

// finds any deathtile between pos1 and pos2 using digital differential analyser
bool CCollision::CheckDeath(const vec2& Pos1, const vec2& Pos2, vec2* pDeathtilepos, float radius)
{
	float dx = (Pos2.x - Pos1.x);
	float dy = (Pos2.y - Pos1.y);

	float Step;
	if(fabs(dx) >= fabs(dy))
		Step = fabs(dx);
	else
		Step = fabs(dy);
	if(Step == 0.0)
		return false;

	dx = dx / Step;
	dy = dy / Step;

	float x = Pos1.x;
	float y = Pos1.y;

	int i = 0;

	while(i <= Step)
	{
		//Check if Collision with Deathtile accured
		if(GetCollisionAt(x, y)&COLFLAG_DEATH) {
			*pDeathtilepos = vec2(x, y);
			return true;
		}
		x = x + dx;
		y = y + dy;

		++i;
	}
	*pDeathtilepos = Pos2;
	return false;
}

bool CCollision::CalcDeathPos(const vec2& Playerpos, const vec2& Nextplayerpos, const vec2& PosDeathtile, vec2* pDeathpos, float radius)
{
	/* Do a box around the death tile with line segments with distance 'radius'
	*  DT = Deathtile, P = Top left corner of Deathtile
	*   C0...........C1
	*   |            |
	*   |	P^^^^*   |
	*   |	- DT -   |
	*   |	*vvvv*   |
	*   |            |
	*   C3...........C2
	*/

	//Calculate top right corner of deathtile
	int TileX = round_to_int(PosDeathtile.x)/32;
	int TileY = round_to_int(PosDeathtile.y)/32;
	vec2 P = vec2(TileX*32.0f, TileY*32.0f);

	//Calculate corners
	vec2 Corner[4];
	Corner[0] = vec2(P.x-radius, P.y-radius);
	Corner[1] = vec2(P.x+radius+32.0f, P.y-radius);
	Corner[2] = vec2(P.x+radius+32.0f, P.y+radius+32.0f);
	Corner[3] = vec2(P.x-radius, P.y+radius+32.0f);

	//Calculate intersections
	bool IntersectionFound = false;
	vec2 ClosestIntersection(0, 0);
	vec2 CurrentIntersection;

	//Iterate over all edges
	for(int i = 0; i < 4; ++i)
	{
		//There may be multiple intersections between the Tee and the Deathtile. The closest is the first the Tee hits
		if(LineLineIntersection(Corner[i], Corner[(i+1)%4], Playerpos, Nextplayerpos, &CurrentIntersection)
			&& (!IntersectionFound || distance(Playerpos, CurrentIntersection) < distance(Playerpos, ClosestIntersection)))
		{
			ClosestIntersection = CurrentIntersection;
			IntersectionFound = true;
		}
	}
	if(!IntersectionFound)
		return false;
	*pDeathpos = ClosestIntersection;
	return true;
}

bool CCollision::LineLineIntersection(const vec2& LineStart1, const vec2& LineEnd1, const vec2& LineStart2, const vec2& LineEnd2, vec2* Intersection)
{
	//Store in own values for readability
	float x1 = LineStart1.x;
	float x2 = LineEnd1.x;
	float x3 = LineStart2.x;
	float x4 = LineEnd2.x;

	float y1 = LineStart1.y;
	float y2 = LineEnd1.y;
	float y3 = LineStart2.y;
	float y4 = LineEnd2.y;

	float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

	// If d is zero, there is no intersection (parallel lines)
	if (d == 0.0f)
		return false;

	// Get the x and y
	float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
	float x = ( pre * (x3 - x4) - (x1 - x2) * post ) / d;
	float y = ( pre * (y3 - y4) - (y1 - y2) * post ) / d;

	// Return the point of intersection
	Intersection->x = x;
	Intersection->y = y;

	return true;
}
