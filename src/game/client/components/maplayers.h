/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#define GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#include <game/client/component.h>

class CMapLayers : public CComponent
{
	CLayers *m_pMenuLayers;
	IEngineMap *m_pMenuMap;

	int m_Type;
	int m_CurrentLocalTick;
	int m_LastLocalTick;
	bool m_EnvelopeUpdate;

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

	void EnvelopeUpdate();

	static void ConchainBackgroundMap(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	virtual void OnConsoleInit();

	void BackgroundMapUpdate();

	bool MenuMapLoaded() { return m_pMenuMap ? m_pMenuMap->IsLoaded() : false; }
};

#endif
