/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BROADCAST_H
#define GAME_CLIENT_COMPONENTS_BROADCAST_H
#include <game/client/component.h>

class CBroadcast : public CComponent
{
	// client broadcast
	char m_aBroadcastText[128];
	float m_BroadcastTime;
	float m_BroadcastRenderOffset;

	void RenderClientBroadcast();

	// server broadcast
	typedef unsigned char u8;
	struct CBcColor
	{
		u8 m_R, m_G, m_B;
		int m_CharPos;
	};

	struct CBcLineInfo
	{
		const char* m_pStrStart;
		int m_StrLen;
		float m_Width;
	};

	enum {
		MAX_BROADCAST_COLORS = 128,
		MAX_BROADCAST_MSG_LENGTH = 127,
		MAX_BROADCAST_LINES = 3,
	};

	CBcColor m_aSrvBroadcastColorList[MAX_BROADCAST_COLORS];
	CBcLineInfo m_aSrvBroadcastLines[MAX_BROADCAST_LINES];
	char m_aSrvBroadcastMsg[MAX_BROADCAST_MSG_LENGTH+1];
	int m_aSrvBroadcastMsgLen;
	int m_SrvBroadcastColorCount;
	int m_SrvBroadcastLineCount;
	float m_SrvBroadcastReceivedTime;
	float m_SrvBroadcastFontSize;
	bool m_MuteServerBroadcast;

	void OnBroadcastMessage(const CNetMsg_Sv_Broadcast *pMsg);
	void RenderServerBroadcast();

public:
	CBroadcast();

	void DoClientBroadcast(const char *pText);

	void ToggleMuteServerBroadcast() { m_MuteServerBroadcast = !m_MuteServerBroadcast; };
	bool IsMuteServerBroadcast() const { return m_MuteServerBroadcast; };

	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnRender();
};

#endif
