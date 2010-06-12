#ifndef GAME_CLIENT_COMPONENTS_COOPBOARD_H
#define GAME_CLIENT_COMPONENTS_COOPBOARD_H
#include <game/client/component.h>

class CCoopboard : public CComponent
{
	void RenderCoopboard();

	static void ConKeyCoopboard(IConsole::IResult *pResult, void *pUserData);
	
	bool m_CoopJustActivated;
	
	int m_Marked;	
	int m_StartY;
	
	int m_LastRequest;
	
	bool m_HasPartner;
	int m_Partner;
	
	void RequestPartner(int NumPlayers, const class CNetObj_PlayerInfo *pPlayer);
	
public:
	CCoopboard();
	
	bool m_Active;
	
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
