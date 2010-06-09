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
	void RenderGameTimer();
	void RenderSuddenDeath();
	void RenderScoreHud();
	void RenderWarmupTimer();
	void RenderSpeedmeter();
	void RenderTime();
	void RenderRecord();

	void MapscreenToGroup(float CenterX, float CenterY, struct CMapItemGroup *PGroup);
public:
	CHud();
	
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
