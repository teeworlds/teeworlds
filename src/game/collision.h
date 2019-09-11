/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <engine/shared/protocol.h>
#include <game/mapitems.h>

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	bool IsTile(int x, int y, int Id=COLFLAG_SOLID) const;
	int GetTile(int x, int y) const;

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y, int Id=TILE_SOLID) const { return IsTile(round_to_int(x), round_to_int(y), Id); }
	bool CheckPoint(vec2 Pos, int Id=TILE_SOLID) const { return CheckPoint(Pos.x, Pos.y, Id); }
	int GetCollisionAt(float x, float y) const { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() const { return m_Width; };
	int GetHeight() const { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath=0) const;
	bool TestBox(vec2 Pos, vec2 Size, int Id=TILE_SOLID) const;

	// DDRace

private:
	class CTeleTile *m_pTele;
	class CSpeedupTile *m_pSpeedup;
	class CTile *m_pFront;
	class CSwitchTile *m_pSwitch;
	class CTuneTile *m_pTune;
	class CDoorTile *m_pDoor;
	struct SSwitchers
	{
		bool m_Status[MAX_CLIENTS];
		bool m_Initial;
		int m_EndTick[MAX_CLIENTS];
		int m_Type[MAX_CLIENTS];
	};

public:
	SSwitchers *m_pSwitchers;
};

#endif
