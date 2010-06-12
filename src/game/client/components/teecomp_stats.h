#include <game/client/component.h>

class CTeecompStats: public CComponent
{
private:
	int m_Mode;
	int m_StatsCid;
	static void ConKeyStats(IConsole::IResult *pResult, void *pUserData);
	static void ConKeyNext(IConsole::IResult *pResult, void *pUserData);
	void RenderGlobalStats();
	void RenderIndividualStats();
	void CheckStatsCid();

public:
	CTeecompStats();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	bool IsActive();
};
