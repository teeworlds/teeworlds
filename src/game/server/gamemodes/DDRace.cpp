/*Based on rajh's, Race mod stuff */
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
}

CGameControllerDDRace::~CGameControllerDDRace()
{
	//delete[] m_pTele1D;
	//TODO make normal cleanup !!!!!!!!
	//delete[] m_pNumTele;
	//m_pTele2D = 0x0;
}

void CGameControllerDDRace::Tick()
{
	IGameController::Tick();
}

void CGameControllerDDRace::InitTeleporter()
{
	m_ArraySize = 0;
	m_TotalTele = 0;
	if(GameServer()->Collision()->Layers()->TeleLayer())
	{
		for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
		{
			// get the array size
			if(GameServer()->Collision()->TeleLayer()[i].m_Number > m_ArraySize)
				m_ArraySize = GameServer()->Collision()->TeleLayer()[i].m_Number;
		}
	} else {
		return;
	}
	
	if(!m_ArraySize)
	{
		m_pNumTele = 0x0;
		m_pTele1D = 0x0;
		m_pTele2D = 0x0;
		return;
	}
	int *Count;
	m_pNumTele = new int[m_ArraySize];
	Count = new int[m_ArraySize];
	mem_zero(m_pNumTele, m_ArraySize*sizeof(int));
	mem_zero(Count, m_ArraySize*sizeof(int));
	/* No need in it u use mem_zero
	for (int i = 0; i < m_ArraySize; ++i)
		Count[i] = m_pNumTele[i] = 0;
	*/
	
	int Width = GameServer()->Collision()->Layers()->TeleLayer()->m_Width;
	int Height = GameServer()->Collision()->Layers()->TeleLayer()->m_Height;
	// Count
	for(int i = 0; i < Width*Height; i++)
	{
		if(GameServer()->Collision()->TeleLayer()[i].m_Number > 0 && GameServer()->Collision()->TeleLayer()[i].m_Type == TILE_TELEOUT)
		{
			m_pNumTele[GameServer()->Collision()->TeleLayer()[i].m_Number-1]++;
			Count[GameServer()->Collision()->TeleLayer()[i].m_Number-1]++;
			m_TotalTele++;
		}
	}
	//m_pTele1D = new vec2[m_TotalTele];
	//mem_zero(m_pTele1D, m_TotalTele*sizeof(vec2));
	
	//Please rewrite this place in next lines you broke all initialization
	m_pTele2D = new vec2 * [m_ArraySize];
	for (int i = 0; i < m_ArraySize; ++i)
	{
		m_pTele2D[i] = new vec2[m_pNumTele[i]];
		mem_zero(m_pTele2D[i], m_pNumTele[i]*sizeof(vec2));
	}
	
	for(int i = 0; i < Width*Height; i++)
	{
		if(GameServer()->Collision()->TeleLayer()[i].m_Number > 0 
			&& GameServer()->Collision()->TeleLayer()[i].m_Type == TILE_TELEOUT)
		{
			m_pTele2D[GameServer()->Collision()->TeleLayer()[i].m_Number-1][--Count[GameServer()->Collision()->TeleLayer()[i].m_Number-1]] 
				= vec2(i % GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16, 
						i/GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16);
		}
	}
}

void CGameControllerDDRace::InitSwitcher()
{
	m_Size = 0;
	CMapItemLayerTilemap *pTileMap = GameServer()->Layers()->GameLayer();
	if (GameServer()->m_pSwitch)
	{
		for(int y = 0; y < pTileMap->m_Height; y++)
		{
			for(int x = 0; x < pTileMap->m_Width; x++)
			{
				if(GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Type 
					== (ENTITY_DOOR + ENTITY_OFFSET)) 
				{
					int sides[8][2];
					sides[0][0]=GameServer()->m_pSwitch[(x)+pTileMap->m_Width*(y+1)].m_Type - ENTITY_OFFSET;
					sides[1][0]=GameServer()->m_pSwitch[(x+1)+pTileMap->m_Width*(y+1)].m_Type - ENTITY_OFFSET;
					sides[2][0]=GameServer()->m_pSwitch[(x+1)+pTileMap->m_Width*(y)].m_Type - ENTITY_OFFSET;
					sides[3][0]=GameServer()->m_pSwitch[(x+1)+pTileMap->m_Width*(y-1)].m_Type - ENTITY_OFFSET;
					sides[4][0]=GameServer()->m_pSwitch[(x)+pTileMap->m_Width*(y-1)].m_Type - ENTITY_OFFSET;
					sides[5][0]=GameServer()->m_pSwitch[(x-1)+pTileMap->m_Width*(y-1)].m_Type - ENTITY_OFFSET;
					sides[6][0]=GameServer()->m_pSwitch[(x-1)+pTileMap->m_Width*(y)].m_Type - ENTITY_OFFSET;
					sides[7][0]=GameServer()->m_pSwitch[(x-1)+pTileMap->m_Width*(y+1)].m_Type - ENTITY_OFFSET;
					sides[0][1]=GameServer()->m_pSwitch[(x)+pTileMap->m_Width*(y+1)].m_Number;
					sides[1][1]=GameServer()->m_pSwitch[(x+1)+pTileMap->m_Width*(y+1)].m_Number;
					sides[2][1]=GameServer()->m_pSwitch[(x+1)+pTileMap->m_Width*(y)].m_Number;
					sides[3][1]=GameServer()->m_pSwitch[(x+1)+pTileMap->m_Width*(y-1)].m_Number;
					sides[4][1]=GameServer()->m_pSwitch[(x)+pTileMap->m_Width*(y-1)].m_Number;
					sides[5][1]=GameServer()->m_pSwitch[(x-1)+pTileMap->m_Width*(y-1)].m_Number;
					sides[6][1]=GameServer()->m_pSwitch[(x-1)+pTileMap->m_Width*(y)].m_Number;
					sides[7][1]=GameServer()->m_pSwitch[(x-1)+pTileMap->m_Width*(y+1)].m_Number;
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
		for(int y = 0; y < pTileMap->m_Height; y++)
		{
			for(int x = 0; x < pTileMap->m_Width; x++)
			{
				if(GameServer()->m_pSwitch[y*pTileMap->m_Width+x].m_Type - ENTITY_OFFSET == ENTITY_TRIGGER) {
					for(int i = 0; i < m_SDoors.size(); ++i) {
						if(m_SDoors[i].m_Number == GameServer()->m_pSwitch[y*pTileMap->m_Width+x].m_Number) {
							new CTrigger(
								&GameServer()->m_World,
								vec2(x*32.0f+16.0f, y*32.0f+16.0f), 
								m_SDoors[i].m_Address);
							break;
						}
					}
				}
			}
		}
	}
}
