// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include "dm.h"


CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "DM";
}

void CGameControllerDM::Tick()
{
	DoPlayerScoreWincheck();
	IGameController::Tick();
}
