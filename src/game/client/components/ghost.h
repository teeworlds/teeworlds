/* (c) Rajh. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <game/client/component.h>

class CGhost : public CComponent
{
private:
	struct CGhostHeader
	{
		unsigned char m_aMarker[8];
		unsigned char m_Version;
	};
	
	array<CNetObj_Character> m_CurPath;
	array<CNetObj_Character> m_BestPath;
	
	CNetObj_ClientInfo m_CurInfo;
	CNetObj_ClientInfo m_GhostInfo;

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
	
	void AddInfos(CNetObj_Character Player);
	
	void StartRecord();
	void StopRecord();
	void StartRender();
	void StopRender();
	void RenderGhost();
	void RenderGhostHook();
	
	void Save();
	void Load();

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

public:
	CGhost();

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();
};

#endif
