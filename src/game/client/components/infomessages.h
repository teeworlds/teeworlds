/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_INFOMESSAGES_H
#define GAME_CLIENT_COMPONENTS_INFOMESSAGES_H
#include <game/client/component.h>

class CInfoMessages : public CComponent
{
	// info messages
	struct CInfoMsg
	{
		int m_Type;
		int m_Tick;
	
		// victim / finishing player
		int m_Player1ID;
		char m_aPlayer1Name[MAX_NAME_LENGTH];
		CTeeRenderInfo m_Player1RenderInfo;

		// killer
		int m_Player2ID;
		char m_aPlayer2Name[MAX_NAME_LENGTH];
		CTeeRenderInfo m_Player2RenderInfo;

		// kill msg
		int m_Weapon;
		int m_ModeSpecial; // for CTF, if the guy is carrying a flag for example
		int m_FlagCarrierBlue;

		// finish msg
		int m_Time;
		int m_Diff;
		int m_RecordPersonal;
		int m_RecordServer;
	};

	enum
	{
		MAX_INFOMSGS = 5,

		INFOMSG_KILL = 0,
		INFOMSG_FINISH
	};

	CInfoMsg m_aInfoMsgs[MAX_INFOMSGS];
	int m_InfoMsgCurrent;

	void AddInfoMsg(int Type, CInfoMsg NewMsg);

	void RenderKillMsg(const CInfoMsg *pInfoMsg, float x, float y) const;
	void RenderFinishMsg(const CInfoMsg *pInfoMsg, float x, float y) const;

public:
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
