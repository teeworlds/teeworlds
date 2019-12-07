/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_NOTIFICATIONS_H
#define GAME_CLIENT_COMPONENTS_NOTIFICATIONS_H
#include <game/client/component.h>

class CNotifications : public CComponent
{
	float m_SoundToggleTime;

	void OnConsoleInit();
	void RenderSoundNotification();

	static void Con_SndToggle(IConsole::IResult *pResult, void *pUserData);
public:
	CNotifications();
	virtual void OnRender();
};

#endif
