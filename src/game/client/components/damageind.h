/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_DAMAGEIND_H
#define GAME_CLIENT_COMPONENTS_DAMAGEIND_H
#include <base/vmath.h>
#include <game/client/component.h>

class CDamageInd : public CComponent
{
	int64 m_Lastupdate;
	struct CItem
	{
		vec2 m_Pos;
		vec2 m_Dir;
		float m_StartTime;
		float m_StartAngle;
	};

	enum
	{
		MAX_ITEMS=64,
	};

	CItem m_aItems[MAX_ITEMS];
	int m_NumItems;

	CItem *CreateI();
	void DestroyI(CItem *i);

public:
	CDamageInd();

	void Create(vec2 Pos, vec2 Dir);
	virtual void OnRender();
};
#endif
