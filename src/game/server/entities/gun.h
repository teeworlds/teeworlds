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
	 bool m_Freeze;
	 bool m_Explosive;

	void Fire();
	int m_Delay;
	
public:
	CGun(CGameWorld *pGameWorld, vec2 Pos, bool Freeze, bool Explosive, int Layer = 0, int Number = 0);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};


#endif
