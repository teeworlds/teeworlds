#ifndef GAME_CLIENT_COMPONENTS_STATBOARD_H
#define GAME_CLIENT_COMPONENTS_STATBOARD_H
#include <game/client/component.h>

class CStatboard : public CComponent
{
	void RenderStatboard();

	static void ConKeyStatboard(IConsole::IResult *pResult, void *pUserData);
	
	bool m_StatJustActivated;
	
	int m_LastRequest;
	
	int m_CurrentLine;
	int m_CurrentRow;
	int m_StartX;
	int m_StartY;
	
	void SendStatMsg(int Num);
	
public:
	CStatboard();
	
	bool m_Active;
	int m_Marked;
	
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
};

#endif
