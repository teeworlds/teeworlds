/* (c) Redix and Sushi */

#ifndef GAME_CLIENT_COMPONENTS_RACE_DEMO_H
#define GAME_CLIENT_COMPONENTS_RACE_DEMO_H

#include <game/client/component.h>

class CRaceDemo : public CComponent
{
	int m_RaceState;
	int m_RecordStopTick;
	int m_PrepareStartTick;
	int m_Time;
	
public:
	
	enum
	{
		RACE_NONE = 0,
		RACE_IDLE,
		RACE_PREPARE,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	CRaceDemo();
	
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);

	void StopRecord(int Time = 0);
	bool CheckDemo(int Time) const;
};
#endif
