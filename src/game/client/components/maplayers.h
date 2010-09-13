// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#define GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#include <game/client/component.h>

class CMapLayers : public CComponent
{	
	CLayers *m_pLayers;	// todo refactor: maybe remove it and access it through client*
	int m_Type;

	void MapScreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup);
	static void EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser);
public:
	enum
	{
		TYPE_BACKGROUND=0,
		TYPE_FOREGROUND,
	};

	CMapLayers(int Type);
	virtual void OnInit();
	virtual void OnRender();
};

#endif
