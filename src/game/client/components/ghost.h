/* (c) Rajh, Redix and Sushi. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <base/tl/array.h>

#include <game/client/component.h>

#include <game/ghost.h>

class CGhost : public CComponent
{

private:
	class CGhostItem
	{
	public:
		int m_ID;
		CTeeRenderInfo m_RenderInfo;
		array<CGhostCharacter> m_Path;
		char m_aOwner[MAX_NAME_LENGTH];

		bool operator==(const CGhostItem &Other) { return m_ID == Other.m_ID; }

		CGhostItem() : m_ID(-1) { }
		CGhostItem(int ID) : m_ID(ID) { }
	};

	array<CGhostItem> m_lGhosts;
	CGhostItem m_CurGhost;

	int m_StartRenderTick;
	int m_LastRecordTick;
	CGhostCharacter m_LastRecordChar;
	int m_CurPos;
	bool m_Rendering;
	bool m_Recording;
	int m_BestTime;

	void AddInfos(CGhostCharacter Player);

	bool IsStart(vec2 PrevPos, vec2 Pos);

	void StartRecord();
	void StopRecord(int Time=0);
	void StartRender();
	void StopRender();
	void RenderGhost(CGhostCharacter Player, CGhostCharacter Prev, CTeeRenderInfo *pRenderInfo);
	void RenderGhostHook(CGhostCharacter Player, CGhostCharacter Prev);
	void RenderGhostNamePlate(CGhostCharacter Player, CGhostCharacter Prev, const char *pName);
	
	void InitRenderInfos(CTeeRenderInfo *pRenderInfo, const char *pSkinName, int UseCustomColor, int ColorBody, int ColorFeet);

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

public:
	CGhost();

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnShutdown();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();

	bool Load(const char* pFilename, int ID);
	void Unload(int ID);
};

#endif
