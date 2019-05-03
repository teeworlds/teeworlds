#include <game/client/component.h>

enum {
	TC_STATS_FRAGS=1,
	TC_STATS_DEATHS=2,
	TC_STATS_SUICIDES=4,
	TC_STATS_RATIO=8,
	TC_STATS_NET=16,
	TC_STATS_FPM=32,
	TC_STATS_SPREE=64,
	TC_STATS_BESTSPREE=128,
	TC_STATS_FLAGGRABS=256,
	TC_STATS_WEAPS=512,
	TC_STATS_FLAGCAPTURES=1024,
};

class CStats: public CComponent
{
private:
	int m_Active;
	static void ConKeyStats(IConsole::IResult *pResult, void *pUserData);

public:
	CStats();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};