/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_TIMEMESSAGES_H
#define GAME_CLIENT_COMPONENTS_TIMEMESSAGES_H
#include <game/client/component.h>

class CTimeMessages : public CComponent
{
public:
	// time messages
	struct CTimeMsg
	{
		int m_Minutes;
		float m_Seconds;
		float m_LocalDiff;
		float m_ServerDiff;
		int m_PlayerID;
		char m_aPlayerName[MAX_NAME_LENGTH];
		CTeeRenderInfo m_PlayerRenderInfo;
		int64 m_Tick;
	};
	
	enum
	{
		MAX_TIMEMSGS = 7,
	};

	CTimeMsg m_aTimemsgs[MAX_TIMEMSGS];
	int m_TimemsgCurrent;

	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
