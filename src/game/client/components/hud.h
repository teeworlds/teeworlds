#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <game/client/component.h>

class CHud : public CComponent
{	
	float m_Width;
	float m_AverageFPS;
	
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

	void MapscreenToGroup(float CenterX, float CenterY, struct CMapItemGroup *PGroup);
public:
	CHud();
	
	virtual void OnReset();
	virtual void OnRender();
};

#endif
