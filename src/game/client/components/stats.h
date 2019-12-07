#ifndef GAME_CLIENT_COMPONENTS_STATS_H
#define GAME_CLIENT_COMPONENTS_STATS_H

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
// stats
	class CPlayerStats
	{
	public:
		CPlayerStats()
		{
			Reset();
		}

		int m_IngameTicks;
		int m_aFragsWith[NUM_WEAPONS];
		int m_aDeathsFrom[NUM_WEAPONS];
		int m_Frags;
		int m_Deaths;
		int m_Suicides;
		int m_BestSpree;
		int m_CurrentSpree;

		int m_FlagGrabs;
		int m_FlagCaptures;
		int m_CarriersKilled;
		int m_KillsCarrying;
		int m_DeathsCarrying;

		void Reset();
	};
	CPlayerStats m_aStats[MAX_CLIENTS];

	int m_Active;
	bool m_ScreenshotTaken;
	int64 m_ScreenshotTime;
	static void ConKeyStats(IConsole::IResult *pResult, void *pUserData);
	void AutoStatScreenshot();

public:
	CStats();
	bool IsActive();
	virtual void OnReset();
	void OnStartGame();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	bool IsActive() const { return m_Active; }

	void UpdatePlayTime(int Ticks);
	void OnMatchStart();
	void OnFlagGrab(int ClientID);
	void OnFlagCapture(int ClientID);
	void OnPlayerEnter(int ClientID, int Team);
	void OnPlayerLeave(int ClientID);

	const CPlayerStats *GetPlayerStats(int ClientID) const { return &m_aStats[ClientID]; }
};

#endif
