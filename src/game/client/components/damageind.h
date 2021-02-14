/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_DAMAGEIND_H
#define GAME_CLIENT_COMPONENTS_DAMAGEIND_H
#include <base/vmath.h>
#include <game/client/component.h>

class CDamageInd : public CComponent
{
	struct CItem
	{
		vec2 m_Pos;
		vec2 m_Dir;
		float m_LifeTime;
		float m_StartAngle;
		int m_ClientID;
	};

	enum
	{
		MAX_ITEMS=64,
	};

	CItem m_aItems[MAX_ITEMS];
	int m_NumItems;

	CItem *CreateItem();
	void DestroyItem(CItem *pItem);

public:
	CDamageInd();

	void Create(vec2 Pos, vec2 Dir, int ClientID);

	virtual void OnRender();
	virtual void OnReset();
};
#endif
