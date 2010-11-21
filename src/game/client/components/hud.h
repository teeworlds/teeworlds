/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <game/client/component.h>

class CHud : public CComponent
{	
	float m_Width;
	float m_AverageFPS;
	
	float m_CheckpointDiff;
	int m_DDRaceTime;
	int m_LastReceivedTimeTick;
	int m_CheckpointTick;
	int m_DDRaceTick;
	bool m_FinishTime;
	float m_ServerRecord;
	float m_PlayerRecord;
	bool m_DDRaceTimeReceived;
	
	
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

	void MapscreenToGroup(float CenterX, float CenterY, struct CMapItemGroup *PGroup);
public:
	CHud();
	
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
