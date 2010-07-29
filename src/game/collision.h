#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

class CCollision
{
	class CTile *m_pTiles;
	class CTile *m_pFTiles;
	int m_Width;
	int m_Height;
	
	bool isOneLayer;
	
	int *m_pDest[95];
	int m_Len[95];
	int m_Tele[95];
	
	int *m_pTeleporter;
	int *m_pFTeleporter; 
	
	class CLayers *m_pLayers;

	bool IsTileSolid(int x, int y);


public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
		COLFLAG_NOLASER=8,
	};
	int GetTile(int x, int y);
	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round(x), round(y)); }
	bool CheckPoint(vec2 p) { return CheckPoint(p.x, p.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round(x), round(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *Bpounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);
	
	void Set(int x, int y, int flags);
	int IntersectNolaser(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision);
	int IntersectNolaser2(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision);
	int IntersectAir(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision);
	//DDRace
	int IsSolid(int x, int y);
	int IsTeleport(int x, int y);
	int IsBegin(int x, int y);
	int IsNolaser(int x, int y);
	int IsEnd(int x, int y);
	int IsBoost(int x, int y);
	int IsFreeze(int x, int y);
	int IsUnfreeze(int x, int y);
	int IsKick(int x, int y);
	int IsCp(int x, int y);
	
	int GetIndex(int x, int y, bool flayer);
	vec2 Speed(int index);
	vec2 BoostAccel(int index);
	vec2 Teleport(int z);  
	
	vec2 CpSpeed(int index);
};

#endif
