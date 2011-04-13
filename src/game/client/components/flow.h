/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_FLOW_H
#define GAME_CLIENT_COMPONENTS_FLOW_H
#include <base/vmath.h>
#include <game/client/component.h>

class CFlow : public CComponent
{
	struct CCell
	{
		vec2 m_Vel;
	};

	CCell *m_pCells;
	int m_Height;
	int m_Width;
	int m_Spacing;

	void DbgRender();
	void Init();
public:
	CFlow();

	vec2 Get(vec2 Pos);
	void Add(vec2 Pos, vec2 Vel, float Size);
	void Update();
};

#endif
