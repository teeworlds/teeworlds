/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SCOREBOARD_H
#define GAME_CLIENT_COMPONENTS_SCOREBOARD_H
#include <game/client/component.h>

class CScoreboard : public CComponent
{
	void RenderGoals(float x, float y, float w);
	float RenderSpectators(float x, float y, float w);
	float RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, int Align);
	void RenderRecordingNotification(float x);

	static void ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData);

	const char *GetClanName(int Team);

	bool m_Active;
	bool m_Activate;
	int m_PlayerLines;
 	class CUIRect m_TotalRect;

public:
	CScoreboard();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnRelease();
	
 	bool Active();
	void ResetPlayerStats(int ClientID);
 	class CUIRect GetScoreboardRect();
};

#endif
