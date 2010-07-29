/* copyright (c) 2007 rajh, race mod stuff */
#include <engine/server.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "race.h"

CGameControllerRace::CGameControllerRace(class CGameContext *pGameServer)
: IGameController(pGameServer), m_Score(pGameServer)
{

	m_pGameType = "DDRace";
}


void CGameControllerRace::Tick()
{
	IGameController::Tick();
}
