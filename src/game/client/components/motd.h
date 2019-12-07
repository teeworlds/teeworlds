/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MOTD_H
#define GAME_CLIENT_COMPONENTS_MOTD_H
#include <game/client/component.h>

class CMotd : public CComponent
{
	// motd
	int64 m_ServerMotdTime;
	char m_aServerMotd[1024];
public:
	void Clear();
	bool IsActive();
	const char *GetMotd() const { return m_aServerMotd; }

	virtual void OnRender();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);
};

#endif
