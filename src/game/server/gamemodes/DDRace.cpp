/*Based on rajh's, Redix's & Sushi Tee's, DDRace mod stuff and tweaked byt btd and GreYFoX@GTi with STL to fit our DDRace needs*/
#include <engine/server.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "DDRace.h"

CGameControllerDDRace::CGameControllerDDRace(class CGameContext *pGameServer) : IGameController(pGameServer), m_Teams(pGameServer)
{
	m_pGameType = "DDRace";

	InitTeleporter();
}

CGameControllerDDRace::~CGameControllerDDRace()
{
	//Nothing to clean
}

void CGameControllerDDRace::Tick()
{
	IGameController::Tick();
}

void CGameControllerDDRace::InitTeleporter()
{
	if(!GameServer()->Collision()->Layers()->TeleLayer()) return;
	int Width = GameServer()->Collision()->Layers()->TeleLayer()->m_Width;
	int Height = GameServer()->Collision()->Layers()->TeleLayer()->m_Height;

	for(int i = 0; i < Width*Height; i++)
	{
		if(GameServer()->Collision()->TeleLayer()[i].m_Number > 0 
			&& GameServer()->Collision()->TeleLayer()[i].m_Type == TILE_TELEOUT)
		{
			m_TeleOuts[GameServer()->Collision()->TeleLayer()[i].m_Number-1].push_back(vec2(i % GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16, 
						i/GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16));
		}
	}
}
