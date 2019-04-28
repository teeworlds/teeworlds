/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BIOLOGIST_LASER_H
#define GAME_SERVER_ENTITIES_BIOLOGIST_LASER_H

#include <game/server/entity.h>

class CBiologistLaser : public CEntity
{
public:
	CBiologistLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, int Owner, int Dmg);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	
protected:
	void HitCharacter(vec2 From, vec2 To);
	void DoBounce();

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
	int m_Dmg;
};

#endif
