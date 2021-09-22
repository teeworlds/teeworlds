/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

enum CollisionAlgorithmFlags
{
	CLAFLAG_SOLID_WATER = 1, //react to water as if it was a solid tile
	CLAFLAG_SEPARATE_AIR_TILE = 2, //functions will return true if COLFLAG_AIR is put into them
	CLAFLAG_FULLY_SUBMERGED = 4, //check if the object is fully submerged

};
class CCollision
{
	struct CTile *m_pTiles;

	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;
	class CGameClient* m_pWater;

	bool IsTile(int x, int y, int Flag=COLFLAG_SOLID) const;
	//bool IsAirTile(int x, int y, int Flag = COLFLAG_SOLID) const;
	int GetTile(int x, int y) const;
public:
	struct CTile* m_pCollisionTiles;
	enum
	{
		COLFLAG_AIR = 0,
		COLFLAG_SOLID = 1,
		COLFLAG_DEATH = 2,
		COLFLAG_NOHOOK = 4,
		COLFLAG_WATER = 8,
	};

	CCollision();
	void Init(class CLayers* pLayers, void (*ptr)(float, float, float) = 0x0);
	//void InitializeWater();
	void (*pointer)(float,float,float);
	bool CheckPoint(float x, float y, int Flag = COLFLAG_SOLID) const;
	bool CheckPoint(vec2 Pos, int Flag = COLFLAG_SOLID) const;
	//bool CheckWaterPoint(vec2 Pos, int Flag = COLFLAG_SOLID) const { return CheckPoint(Pos.x, Pos.y, Flag); }
	int GetCollisionAt(float x, float y) const { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWaterCollisionAt(float x, float y, int Flag) const { return IsTile(round_to_int(x), round_to_int(y), Flag); }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }

	bool IntersectWater(vec2 Pos0, vec2 Pos1, vec2* Position, vec2 Size);

	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	int IntersectLineWithWater(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int Flag) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void Diffract(vec2* pInoutPos, vec2* pInoutVel, float Elasticity, int* pBounces, int Flag) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath=0, int CLAFLAG = 0) const;
	void MoveWaterBox(vec2* pInoutPos, vec2* pInoutVel, vec2 Size, float Elasticity, bool* pDeath = 0, float Severity = 0.95) const;
	bool TestBox(vec2 Pos, vec2 Size, int Flag=COLFLAG_SOLID, int CLAFlag = 0) const;
};

#endif
