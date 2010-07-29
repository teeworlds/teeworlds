/*Based on rajh's, Race mod stuff */
#include <engine/server.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "DDRace.h"

CGameControllerDDRace::CGameControllerDDRace(class CGameContext *pGameServer)
: IGameController(pGameServer), m_Score(pGameServer)
{

	m_pGameType = "DDRace";
}


void CGameControllerDDRace::Tick()
{
	IGameController::Tick();
}
