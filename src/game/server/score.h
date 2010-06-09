#ifndef GAME_SERVER_INTERFACE_SCORE_H
#define GAME_SERVER_INTERFACE_SCORE_H

#include "entities/character.h"
#include "gamecontext.h"

#define NUM_TELEPORT 25

class CPlayerData
{
public:
	CPlayerData()
	{
		Reset();
	}
	
	void Reset()
	{
		m_BestTime = 0;
		m_CurrentTime = 0;
		for(int i = 0; i < NUM_TELEPORT; i++)
			m_aBestCpTime[i] = 0;
	}
	
	void Set(float Time, float CpTime[NUM_TELEPORT])
	{
		m_BestTime = Time;
		for(int i = 0; i < NUM_TELEPORT; i++)
			m_aBestCpTime[i] = CpTime[i];
	}
	
	float m_BestTime;
	float m_CurrentTime;
	float m_aBestCpTime[NUM_TELEPORT];
};

class IScore
{
	CPlayerData m_aPlayerData[MAX_CLIENTS];
	
public:
	virtual ~IScore() {}
	
	CPlayerData *PlayerData(int ID) { return &m_aPlayerData[ID]; }
	
	virtual void LoadScore(int ClientID) = 0;
	virtual void SaveScore(int ClientID, float Time, CCharacter *pChar) = 0;
	
	virtual void ShowTop5(int ClientID, int Debut=1) = 0;
	virtual void ShowRank(int ClientID, const char* pName, bool Search=false) = 0;
};

#endif
