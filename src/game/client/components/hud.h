/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <game/client/component.h>

class CHud : public CComponent
{	
	float m_Width;
	float m_AverageFPS;
	
	// coop stuff
	char m_aName[MAX_NAME_LENGTH];
	int m_Cid;
	int m_Level;
	int m_Weapon;
	int m_Str;
	int m_Sta;
	int m_Dex;
	int m_Int;
	
	int m_LastAnswer;

	// spree stuff
	int m_SpreeInitTick;
	int m_SpreeNum;
	int m_SpreeId;
	int m_EndedBy;
	
	static void ConCoop(IConsole::IResult *pResult, void *pUserData);

	// Race
	float m_CheckpointDiff;
	int m_RaceTime;
	int m_LastReceivedTimeTick;
	int m_CheckpointTick;
	int m_RaceTick;
	float m_FinishTime;
	float m_Record;
	float m_LocalRecord;

	void RenderCursor();
	
	void RenderFps();
	void RenderConnectionWarning();
	void RenderTeambalanceWarning();
	void RenderVoting();
	void RenderHealthAndAmmo();
	void RenderLvlxHealthAndAmmo();
	void RenderGameTimer();
	void RenderSuddenDeath();
	void RenderScoreHud();
	void RenderWarmupTimer();

	void RenderTime();
	void RenderRecord();
	void RenderSpeedmeter();
	void RenderSpectate();
	void RenderCoop();
	void RenderExpBar();
	void RenderInfo();
	void RenderSpree();

	void MapscreenToGroup(float CenterX, float CenterY, struct CMapItemGroup *PGroup);
public:
	CHud();
	
	bool m_GotRequest;
	
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
