/* copyright (c) 2007 rajh and gregwar. Score stuff */

#ifndef DDRACE_H
#define DDRACE_H
#include <game/server/gamecontroller.h>
#include <game/server/score.h>

class CGameControllerDDRace : public IGameController
{
public:
	
	CGameControllerDDRace(class CGameContext *pGameServer);
	~CGameControllerDDRace();
	
	vec2 *m_pTeleporter;
	
	void InitTeleporter();
	CScore m_Score;

	virtual void Tick();
};
#endif