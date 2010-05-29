#ifndef GAME_CLIENT_COMPONENTS_KILLMESSAGES_H
#define GAME_CLIENT_COMPONENTS_KILLMESSAGES_H
#include <game/client/component.h>

class CKillMessages : public CComponent
{
public:
	// kill messages
	struct CKillMsg
	{
		int m_Weapon;
		int m_Victim;
		int m_Killer;
		int m_ModeSpecial; // for CTF, if the guy is carrying a flag for example
		int m_Tick;
	};
	
	enum
	{
		MAX_KILLMSGS = 5,
	};

	CKillMsg m_aKillmsgs[MAX_KILLMSGS];
	int m_KillmsgCurrent;

	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
