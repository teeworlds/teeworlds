#ifndef GAME_CLIENT_COMPONENTS_RACE_DEMO_H
#define GAME_CLIENT_COMPONENTS_RACE_DEMO_H

#include <game/client/gameclient.h>

#include <game/client/component.h>

class CRaceDemo : public CComponent
{
	int m_RaceState;
	int m_RecordStopTime;
	int m_DemoStartTick;
	float m_Time;
	const char *m_pMap;
	
	bool m_Active;
	
public:
	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	CRaceDemo();
	
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	
	float GetFinishTime() { return m_Time; }
	int GetRaceState() { return m_RaceState; }
	void CheckDemo();
	void SaveDemo(const char* pDemo);
};
#endif
