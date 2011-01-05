/* (c) Rajh. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <base/tl/array.h>
#include <game/client/component.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include "players.h"

class CGhost : public CComponent
{
private:
	struct CGhostHeader
	{
		unsigned char m_aMarker[8];
		unsigned char m_Version;
	};

	// All infos needed
	struct CGhostInfo
	{
		CNetObj_Character m_Player;
		CNetObj_ClientInfo m_Info;
	};
	
	array<CGhostInfo> m_CurPath;
	array<CGhostInfo> m_BestPath;

	int m_StartRenderTick;
	int m_StartRecordTick;
	int m_CurPos;
	bool m_Recording;
	bool m_Rendering;
	int m_RaceState;
	float m_PrevTime;
	bool m_NewRecord;

	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	void AddInfos(CNetObj_Character Player, CNetObj_ClientInfo Info);

public:
	CGhost();

	void StartRecord();
	void StopRecord();
	void StartRender();
	void StopRender();
	void RenderGhost();
	void RenderGhostHook();
	
	void Save();
	void Load();

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();
};

#endif
