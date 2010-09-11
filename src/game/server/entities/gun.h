/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef CGun_TYPE
#define CGun_TYPE

#include <game/server/entity.h>
#include <game/gamecore.h>

class CCharacter;

class CGun : public CEntity
{
	int m_EvalTick;

	vec2 m_Core;
	 int m_Freeze;
	 bool m_Explosive;

	void Fire();
	int m_Delay;
	
public:
	CGun(CGameWorld *pGameWorld, vec2 Pos, int Freeze, bool Explosive);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};


#endif
