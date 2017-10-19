/* (c) Redix and Sushi */

#ifndef GAME_CLIENT_COMPONENTS_RACE_DEMO_H
#define GAME_CLIENT_COMPONENTS_RACE_DEMO_H

#include <game/client/component.h>

class CRaceDemo : public CComponent
{
	enum
	{
		RACE_NONE = 0,
		RACE_IDLE,
		RACE_PREPARE,
		RACE_STARTED,
		RACE_FINISHED,
	};

	static const char *ms_pRaceDemoDir;

	char m_aTmpFilename[128];

	int m_RaceState;
	int m_RaceStartTick;
	int m_RecordStopTick;
	int m_Time;

	static int RaceDemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);

	void GetPath(char *pBuf, int Size, int Time = -1) const;

	void StopRecord(int Time = -1);
	bool CheckDemo(int Time) const;

public:
	CRaceDemo();

	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);

	void OnNewSnapshot();
};
#endif
