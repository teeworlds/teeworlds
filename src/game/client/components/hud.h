/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <game/client/component.h>

class CHud : public CComponent
{	
	float m_Width;
	float m_AverageFPS;
	
	// Race
	float m_CheckpointDiff;
	int m_RaceTime;
	int m_LastReceivedTimeTick;
	int m_CheckpointTick;
	int m_RaceTick;
	float m_Record;
	float m_FinishTime;
	int m_RaceState;

	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	void RenderCursor();
	
	void RenderFps();
	void RenderConnectionWarning();
	void RenderTeambalanceWarning();
	void RenderVoting();
	void RenderHealthAndAmmo();
	void RenderGameTimer();
	void RenderSuddenDeath();
	void RenderScoreHud();
	void RenderWarmupTimer();

	void RenderTime();
	void RenderRecord();
	void RenderSpeedmeter();
	void RenderSpectate();

	void MapscreenToGroup(float CenterX, float CenterY, struct CMapItemGroup *PGroup);
public:
	CHud();
	
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
