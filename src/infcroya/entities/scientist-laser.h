/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_SCILASER_H
#define GAME_SERVER_ENTITIES_SCILASER_H

#include <game/server/entity.h>

class CScientistLaser : public CEntity
{
public:
	CScientistLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Dmg);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	
protected:
	bool HitCharacter(vec2 From, vec2 To);
	void DoBounce();
	void CreateWhiteHole(vec2 CenterPos);

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
	int m_Dmg;
	CCharacter *m_OwnerChar;
};

#endif
