/*Based on rajh's, Redix's & Sushi Tee's, DDRace mod stuff and tweaked byt btd and GreYFoX@GTi with STL to fit our DDRace needs*/
#include <engine/server.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "DDRace.h"

CGameControllerDDRace::CGameControllerDDRace(class CGameContext *pGameServer) : IGameController(pGameServer), m_Teams(pGameServer), m_SDoors(50)
{
	m_pGameType = "DDRace";

	InitTeleporter();
	InitSwitcher();
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

void CGameControllerDDRace::InitSwitcher()
{
	if(!GameServer()->Collision()->SwitchLayer()) return;
	CMapItemLayerTilemap *pTileMap = GameServer()->Layers()->GameLayer();
	for(int y = 0; y < pTileMap->m_Height; y++) // Count Doors and push them to m_SDoors
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			if(GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Type == (ENTITY_DOOR + ENTITY_OFFSET))
			{
				int sides[8][2];
				sides[0][0]=GameServer()->Collision()->SwitchLayer()[(x)+pTileMap->m_Width*(y+1)].m_Type - ENTITY_OFFSET;
				sides[1][0]=GameServer()->Collision()->SwitchLayer()[(x+1)+pTileMap->m_Width*(y+1)].m_Type - ENTITY_OFFSET;
				sides[2][0]=GameServer()->Collision()->SwitchLayer()[(x+1)+pTileMap->m_Width*(y)].m_Type - ENTITY_OFFSET;
				sides[3][0]=GameServer()->Collision()->SwitchLayer()[(x+1)+pTileMap->m_Width*(y-1)].m_Type - ENTITY_OFFSET;
				sides[4][0]=GameServer()->Collision()->SwitchLayer()[(x)+pTileMap->m_Width*(y-1)].m_Type - ENTITY_OFFSET;
				sides[5][0]=GameServer()->Collision()->SwitchLayer()[(x-1)+pTileMap->m_Width*(y-1)].m_Type - ENTITY_OFFSET;
				sides[6][0]=GameServer()->Collision()->SwitchLayer()[(x-1)+pTileMap->m_Width*(y)].m_Type - ENTITY_OFFSET;
				sides[7][0]=GameServer()->Collision()->SwitchLayer()[(x-1)+pTileMap->m_Width*(y+1)].m_Type - ENTITY_OFFSET;
				sides[0][1]=GameServer()->Collision()->SwitchLayer()[(x)+pTileMap->m_Width*(y+1)].m_Number;
				sides[1][1]=GameServer()->Collision()->SwitchLayer()[(x+1)+pTileMap->m_Width*(y+1)].m_Number;
				sides[2][1]=GameServer()->Collision()->SwitchLayer()[(x+1)+pTileMap->m_Width*(y)].m_Number;
				sides[3][1]=GameServer()->Collision()->SwitchLayer()[(x+1)+pTileMap->m_Width*(y-1)].m_Number;
				sides[4][1]=GameServer()->Collision()->SwitchLayer()[(x)+pTileMap->m_Width*(y-1)].m_Number;
				sides[5][1]=GameServer()->Collision()->SwitchLayer()[(x-1)+pTileMap->m_Width*(y-1)].m_Number;
				sides[6][1]=GameServer()->Collision()->SwitchLayer()[(x-1)+pTileMap->m_Width*(y)].m_Number;
				sides[7][1]=GameServer()->Collision()->SwitchLayer()[(x-1)+pTileMap->m_Width*(y+1)].m_Number;
				for(int i = 0; i < 8; i++) {
					if ((sides[i][0] >= ENTITY_LASER_SHORT && sides[i][0] <= ENTITY_LASER_LONG)
						&& GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Number == sides[i][1]) {
							vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
							SDoors tmp;
							tmp.m_Pos = Pos;
							tmp.m_Address = new CDoor(
								&GameServer()->m_World,
								Pos,
								pi/4*i,
								(32*3 + 32*(sides[i][0] - ENTITY_LASER_SHORT)*3),
								false);
							tmp.m_Number = GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Number;
							m_SDoors.push_back(tmp);
					}
				}
			}
		}
	}
	for(int y = 0; y < pTileMap->m_Height; y++)//Create Switch for each Door in m_SDoors
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			if(GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Type - ENTITY_OFFSET == ENTITY_TRIGGER) {
				for(int i = 0; i < m_SDoors.size(); ++i) {
					if(m_SDoors[i].m_Number == GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Number) {
						new CTrigger(
							&GameServer()->m_World,
							vec2(x*32.0f+16.0f, y*32.0f+16.0f),
							m_SDoors[i].m_Address);
					}
				}
			}
		}
	}
}
