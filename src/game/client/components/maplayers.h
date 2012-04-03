/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#define GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#include <game/client/component.h>

class CMapLayers : public CComponent
{
	CLayers *m_pLayers;	// todo refactor: maybe remove it and access it through client*
	int m_Type;
	int m_CurrentLocalTick;
	int m_LastLocalTick;
	bool m_EnvelopeUpdate;


	class CGroupLayer
	{
		bool m_Render;
	public:
		CGroupLayer() : m_Render(true) {}
		void SwitchRender() { m_Render ^= 1; }
		bool Active() { return m_Render; }
	};

	array<CGroupLayer> m_lGroups;
	array<CGroupLayer> m_lLayers;

	void MapScreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup);
	static void EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser);
public:
	enum
	{
		TYPE_BACKGROUND=0,
		TYPE_FOREGROUND,

		TYPE_GROUP=0,
		TYPE_LAYER,
	};

	CMapLayers(int Type);
	virtual void OnInit();
	virtual void OnMapLoad();
	virtual void OnRender();

	void SwitchRender(int Type, int Index);
	bool Active(int Type, int Index);

	void EnvelopeUpdate();
};

#endif
