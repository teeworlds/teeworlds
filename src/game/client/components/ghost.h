/* (c) Rajh, Redix and Sushi. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <base/tl/array.h>
#include <game/client/component.h>

class CGhost : public CComponent
{
public:
	struct CGhostHeader
	{
		unsigned char m_aMarker[8];
		unsigned char m_Version;
		char m_aOwner[MAX_NAME_LENGTH];
		char m_aMap[64];
		unsigned char m_aCrc[4];
		int m_NumShots;
		float m_Time;
	};
	
private:
	struct CGhostCharacter
	{
		int m_X;
		int m_Y;
		int m_VelX;
		int m_VelY;
		int m_Angle;
		int m_Direction;
		int m_Weapon;
		int m_HookState;
		int m_HookX;
		int m_HookY;
		int m_AttackTick;
	};
	
	struct CGhostItem
	{
		int m_ID;
		CNetObj_ClientInfo m_Info;
		array<CGhostCharacter> m_Path;
		
		bool operator==(const CGhostItem &Other) { return m_ID == Other.m_ID; }
	};
	
	array<CGhostItem> m_lGhosts;
	CGhostItem m_CurGhost;

	int m_StartRenderTick;
	int m_CurPos;
	bool m_Recording;
	bool m_Rendering;
	int m_RaceState;
	float m_BestTime;
	bool m_NewRecord;

	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	void AddInfos(CGhostCharacter Player);
	
	CGhostCharacter GetGhostCharacter(CNetObj_Character Char);
	
	void StartRecord();
	void StopRecord();
	void StartRender();
	void StopRender();
	void RenderGhost(CGhostCharacter Player, CGhostCharacter Prev, CNetObj_ClientInfo Info);
	void RenderGhostHook(CGhostCharacter Player, CGhostCharacter Prev);
	
	bool GetHeader(IOHANDLE *pFile, CGhostHeader *pHeader);
	
	void Save();

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

public:
	CGhost();

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();
	
	void Load(const char* pFilename, int ID);
	void Unload(int ID);
	
	bool GetInfo(const char* pFilename, CGhostHeader *pHeader);
};

#endif
