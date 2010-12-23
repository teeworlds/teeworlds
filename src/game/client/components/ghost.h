/* (c) Rajh. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <base/tl/array.h>
#include <game/client/component.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

class CGhost : public CComponent
{
private:
	//All infos needed
	struct GhostInfo
	{
		CNetObj_Character Player;
		CTeeRenderInfo RenderInfo;
		CAnimState AnimState;
	};
	
	GhostInfo m_CurrentInfos;

	array<GhostInfo> m_CurPath;
	array<GhostInfo> m_BestPath;

	int m_StartRenderTick;
	int m_StartRecordTick;
	int m_CurPos;
	bool m_Recording;
	bool m_Rendering;
	int m_RaceState;
	float m_PrevTime;
	bool m_NewRecord;
	int m_RecordTick;

	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};

public:
	CGhost();

	void SetGeneralInfos(CNetObj_Character Player, CTeeRenderInfo RenderInfo, CAnimState AnimState);
	void AddInfos();
	void StartRecord();
	void StopRecord();
	void StartRender();
	void StopRender();
	void RenderGhost();
	void RenderGhostWeapon();
	void RenderGhostHook();

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();
};

#endif
