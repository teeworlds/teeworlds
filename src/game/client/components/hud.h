/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <game/client/component.h>

class CHud : public CComponent
{
	float m_Width, m_Height;
	float m_AverageFPS;


	int64 m_WarmupHideTick;
	bool IsLargeWarmupTimerShown();

	int m_CheckpointDiff;
	int64 m_CheckpointTime;

	void RenderCursor();

	void RenderFps();
	void RenderConnectionWarning();
	void RenderTeambalanceWarning();
	void RenderVoting();
	void RenderNinjaBar(float x, float y, float Progress);
	void RenderHealthAndAmmo(const CNetObj_Character *pCharacter);
	void RenderGameTimer();
	void RenderPauseTimer();
	void RenderStartCountdown();
	void RenderDeadNotification();
	void RenderSuddenDeath();
	void RenderScoreHud();
	void RenderSpectatorHud();
	void RenderSpectatorNotification();
	void RenderReadyUpNotification();
	void RenderWarmupTimer();
	void RenderRaceTime(const CNetObj_PlayerInfoRace *pRaceInfo);
	void RenderCheckpoint();
public:
	CHud();

	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnRender();
};

#endif
