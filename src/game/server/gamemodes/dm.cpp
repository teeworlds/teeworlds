/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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

#ifdef CONF_FAMILY_UNIX

IGameController *CreateGameController(CGameContext *pContext)
{
	return new CGameControllerDM(pContext);
}

void DestroyGameController(IGameController *pController)
{
	delete pController;
}

#endif /* CONF_FAMILY_UNIX */
