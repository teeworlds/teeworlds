/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <game/client/component.h>
#include <base/tl/array.h>

class CHud : public CComponent
{
	float m_Width, m_Height;
	float m_AverageFPS;
	int64 m_WarmupHideTick;

	// broadcast
	typedef unsigned char u8;
	struct CBcColor
	{
		u8 m_R,m_G,m_B;
		int m_CharPos;
	};

	enum {
		MAX_BROADCAST_COLORS = 128,
		MAX_BROADCAST_MSG_LENGTH = 127
	};

	CBcColor m_aBroadcastColorList[MAX_BROADCAST_COLORS];
	char m_aBroadcastMsg[MAX_BROADCAST_MSG_LENGTH+1];
	int m_aBroadcastMsgLen;
	int m_BroadcastColorCount;
	float m_BroadcastReceivedTime;

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
	void RenderWarmupTimer();
	void RenderBroadcast();
public:
	CHud();

	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnRender();
};

#endif
