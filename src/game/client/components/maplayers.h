/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#define GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#include <base/tl/array.h>
#include <game/client/component.h>

class CMapLayers : public CComponent
{
	CLayers *m_pMenuLayers;
	IEngineMap *m_pMenuMap;

	int m_Type;
	int m_CurrentLocalTick;
	int m_LastLocalTick;
	float m_OnlineStartTime;
	bool m_EnvelopeUpdate;

	array<CEnvPoint> m_lEnvPoints;
	array<CEnvPoint> m_lEnvPointsMenu;

	static void EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser);

	void LoadEnvPoints(const CLayers *pLayers, array<CEnvPoint>& lEnvPoints);
	void LoadBackgroundMap();

public:
	enum
	{
		TYPE_BACKGROUND=0,
		TYPE_FOREGROUND,
	};

	CMapLayers(int Type);
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnInit();
	virtual void OnRender();
	virtual void OnMapLoad();

	void EnvelopeUpdate();

	static void ConchainBackgroundMap(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	virtual void OnConsoleInit();

	void BackgroundMapUpdate();

	bool MenuMapLoaded() { return m_pMenuMap ? m_pMenuMap->IsLoaded() : false; }
};

#endif
