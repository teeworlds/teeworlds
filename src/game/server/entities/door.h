#ifndef GAME_SERVER_ENTITY_DOOR_H
#define GAME_SERVER_ENTITY_DOOR_H

#include <game/server/entity.h>

class CTrigger;

class CDoor : public CEntity
{
	vec2 m_To;
	int m_EvalTick[MAX_CLIENTS];
	bool m_Opened[MAX_CLIENTS];
	bool HitCharacter(int Team);
	enum
	{
		DOORED_R = 1,
		DOORED_L = 2,
		DOORED_T = 4,
		DOORED_B = 8
	};
	enum
	{
		DOOR_VER = 1,/*LIKE '|'  */
		DOOR_HOR = 2,/*LIKE '--'  */
		DOOR_DIAGBACK = 4,/*LIKE '\'  */
		DOOR_DIAGFORW = 8 /*LIKE '/'  *///xD
	};
	bool DoDoored(CCharacter* pChar);
	int m_Angle;
public:

	void Open(int Tick, bool ActivatedTeam[]);
	void Close(int Team);
	CDoor(CGameWorld *pGameWorld, vec2 Pos, float Rotation, int Length, bool Opened);
	
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
