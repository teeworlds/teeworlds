/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <list>

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;
	class CTeleTile *m_pTele;
	class CSpeedupTile *m_pSpeedup;
	class CTile *m_pFront;
	class CSwitchTile *m_pSwitch;
	class CDoorTile *m_pDoor;
	struct SSwitchers
	{
		bool m_Status[16];
		int m_EndTick[16];
		int m_Type[16];
	};

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
		COLFLAG_NOLASER=8,
		COLFLAG_THROUGH=16
	};
	CCollision();
	void Dest();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsSolid(round(x), round(y)); }
	bool CheckPoint(vec2 p) { return CheckPoint(p.x, p.y); }
	void SetCollisionAt(float x, float y, int Flag);
	void SetDTile(float x, float y, bool State);
	void SetDCollisionAt(float x, float y, int Type, int Flags, int Number);
	int GetDTileIndex(int Index);
	int GetDTileFlags(int Index);
	int GetDTileNumber(int Index);
	int GetCollisionAt(float x, float y) { return GetTile(round(x), round(y)); }
	int GetFCollisionAt(float x, float y) { return GetFTile(round(x), round(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool AllowThrough);
	int IntersectNoLaser(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectNoLaserNW(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectAir(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *Bpounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);
	
	int GetIndex(int x, int y);
	int GetFIndex(int x, int y);
	
	int GetTile(int x, int y);
	int GetFTile(int x, int y);
	int Entity(int x, int y, int Layer);
	//DDRace
	int GetPureMapIndex(vec2 Pos);
	std::list<int> GetMapIndices(vec2 PrevPos, vec2 Pos, unsigned MaxIndices = 0);
	vec2 GetPos(int Index);
	int GetTileIndex(int Index);
	int GetFTileIndex(int Index);
	int GetTileFlags(int Index);
	int GetFTileFlags(int Index);
	int IsTeleport(int Index);
	int IsEvilTeleport(int Index);
	//int IsCheckpoint(int Index);
	int IsSpeedup(int Index);
	void GetSpeedup(int Index, vec2 *Dir, int *Force, int *MaxSpeed);
	int IsSwitch(int Index);
	int GetSwitchNumber(int Index);
	int GetSwitchDelay(int Index);
	
	int IsSolid(int x, int y);
	int IsThrough(int x, int y);
	int IsNoLaser(int x, int y);
	int IsFNoLaser(int x, int y);

	int IsCheckpoint(int Index);
	int IsFCheckpoint(int Index);
	
	int IsCp(int x, int y, int* Flags);

	vec2 CpSpeed(int index, int Flags = 0);
	
	class CTeleTile *TeleLayer() { return m_pTele; }
	//class CSpeedupTile *SpeedupLayer() { return m_pSpeedup; }
	//class CTile *FrontLayer() { m_pFront; }
	class CSwitchTile *SwitchLayer() { return m_pSwitch; }
	class CLayers *Layers() { return m_pLayers; }
	SSwitchers* m_pSwitchers;
	int m_NumSwitchers;
};

#endif
