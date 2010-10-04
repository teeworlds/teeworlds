/* copyright (c) 2007 rajh and gregwar. Score stuff */

#ifndef DDRACE_H
#define DDRACE_H
#include <game/server/gamecontroller.h>
#include <game/server/teams.h>
#include <game/server/entities/door.h>
#include <game/server/entities/trigger.h>

#include <vector>
#include <map>


class CGameControllerDDRace : public IGameController
{
public:
	
	CGameControllerDDRace(class CGameContext *pGameServer);
	~CGameControllerDDRace();
	
	CGameTeams m_Teams;

	std::map < int , std::vector < vec2 > > m_TeleOuts;
	
	void InitTeleporter();
	void InitSwitcher();
	//int m_Size;

	struct SDoors
	{
		int m_Number;
		vec2 m_Pos;
		CDoor * m_Address;
	};

	std::vector < SDoors > m_SDoors;

	virtual void Tick();
};
#endif
