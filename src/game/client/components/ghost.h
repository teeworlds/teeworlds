/* (c) Rajh, Redix and Sushi. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <engine/shared/ghost.h>
#include <game/client/component.h>

class CGhost : public CComponent
{

private:

	struct CGhostItem
	{
		int m_ID;
		CNetObj_ClientInfo m_Info;
		array<IGhostRecorder::CGhostCharacter> m_Path;
		
		bool operator==(const CGhostItem &Other) { return m_ID == Other.m_ID; }
	};
	
	array<CGhostItem> m_lGhosts;
	CGhostItem m_CurGhost;

	int m_StartRenderTick;
	int m_CurPos;
	bool m_Rendering;
	bool m_Recording;
	int m_RaceState;
	float m_BestTime;
	bool m_NewRecord;

	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	void AddInfos(IGhostRecorder::CGhostCharacter Player);
	
	IGhostRecorder::CGhostCharacter GetGhostCharacter(CNetObj_Character Char);
	
	void StartRecord();
	void StopRecord(float Time=0.0f);
	void StartRender();
	void StopRender();
	void RenderGhost(IGhostRecorder::CGhostCharacter Player, IGhostRecorder::CGhostCharacter Prev, CNetObj_ClientInfo Info);
	void RenderGhostHook(IGhostRecorder::CGhostCharacter Player, IGhostRecorder::CGhostCharacter Prev);
	void RenderGhostNamePlate(IGhostRecorder::CGhostCharacter Player, IGhostRecorder::CGhostCharacter Prev, CNetObj_ClientInfo Info);
	
	bool GetHeader(IOHANDLE *pFile, IGhostRecorder::CGhostHeader *pHeader);
	
	void Save(bool WasRecording);

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

public:
	CGhost();

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnShutdown();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();
	
	void Load(const char* pFilename, int ID);
	void Unload(int ID);
	
	bool GetInfo(const char* pFilename, IGhostRecorder::CGhostHeader *pHeader);
	
	static void ClearFilename(char *pFilename, int Size);
};

#endif
