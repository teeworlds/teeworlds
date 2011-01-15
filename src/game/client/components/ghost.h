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
		float m_Time;
	};
	
private:
	struct CGhostItem
	{
		int m_ID;
		CNetObj_ClientInfo m_Info;
		array<CNetObj_Character> m_Path;
		
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
	
	void AddInfos(CNetObj_Character Player);
	
	void StartRecord();
	void StopRecord();
	void StartRender();
	void StopRender();
	void RenderGhost(CNetObj_Character Player, CNetObj_Character Prev, CNetObj_ClientInfo Info);
	void RenderGhostHook(CNetObj_Character Player, CNetObj_Character Prev);
	
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
