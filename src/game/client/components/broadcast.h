/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BROADCAST_H
#define GAME_CLIENT_COMPONENTS_BROADCAST_H
#include <game/client/component.h>

class CBroadcast : public CComponent
{
	// client broadcast
	CTextCursor m_ClientBroadcastCursor;
	float m_BroadcastTime;

	void RenderClientBroadcast();

	// server broadcast
	struct CBroadcastSegment
	{
		bool m_IsHighContrast;
		int m_GlyphPos;
	};

	enum {
		MAX_BROADCAST_MSG_SIZE = 256,
		MAX_BROADCAST_LINES = 3,
	};

	CBroadcastSegment m_aServerBroadcastSegments[MAX_BROADCAST_MSG_SIZE];
	int m_NumSegments;
	CTextCursor m_ServerBroadcastCursor;
	float m_ServerBroadcastReceivedTime;
	bool m_MuteServerBroadcast;

	void OnBroadcastMessage(const CNetMsg_Sv_Broadcast *pMsg);
	void RenderServerBroadcast();

public:
	CBroadcast();

	void DoClientBroadcast(const char *pText);

	void ToggleMuteServerBroadcast() { m_MuteServerBroadcast = !m_MuteServerBroadcast; }
	bool IsMuteServerBroadcast() const { return m_MuteServerBroadcast; }

	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnRender();
};

#endif
