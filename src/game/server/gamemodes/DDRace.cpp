/*Based on rajh's, Race mod stuff */
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
	delete[] m_pNumTele;
	delete[] m_pTeleOuts;
}

void CGameControllerDDRace::Tick()
{
	IGameController::Tick();
}

void CGameControllerDDRace::InitTeleporter()
{
	int ArraySize = 0;
	m_TotalTele = 0;
	if(GameServer()->Collision()->Layers()->TeleLayer())
	{
		for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
		{
			// get the array size
			if(GameServer()->Collision()->TeleLayer()[i].m_Number > ArraySize)
				ArraySize = GameServer()->Collision()->TeleLayer()[i].m_Number;
		}
	}
	
	if(!ArraySize)
	{
		m_pNumTele = 0x0;
		return;
	}
	int *Count;
	m_pNumTele = new int[ArraySize];
	Count = new int[ArraySize];
	m_pTeleOuts = new vec2*[ArraySize];
	mem_zero(m_pTeleOuts, ArraySize*sizeof(vec2));
	mem_zero(m_pNumTele, ArraySize*sizeof(int));
	mem_zero(Count, ArraySize*sizeof(int));
	for (int i = 0; i < ArraySize; ++i)
		Count[i] = m_pNumTele[i] = 0;
	
	// Count
	for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
	{
		if(GameServer()->Collision()->TeleLayer()[i].m_Number > 0 && GameServer()->Collision()->TeleLayer()[i].m_Type == TILE_TELEOUT)
		{
			m_pNumTele[GameServer()->Collision()->TeleLayer()[i].m_Number-1]++;
			Count[GameServer()->Collision()->TeleLayer()[i].m_Number-1]++;
			m_TotalTele++;
		}
	}
	for (int i = 0; i < ArraySize; ++i)
	{
		m_pTeleOuts[i] = new vec2[m_pNumTele[i]];
		mem_zero(m_pTeleOuts[i], m_pNumTele[i]*sizeof(vec2));
	}
	for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
	{
		if(GameServer()->Collision()->TeleLayer()[i].m_Number > 0 && GameServer()->Collision()->TeleLayer()[i].m_Type == TILE_TELEOUT)
		{
			m_pTeleOuts[GameServer()->Collision()->TeleLayer()[i].m_Number-1][--Count[GameServer()->Collision()->TeleLayer()[i].m_Number-1]] = vec2(i%GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16, i/GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16);
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
			for(int x = 0; x < pTileMap->m_Width; x++)
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
				for(int i=0; i<8;i++)
					if ((sides[i][0] >= ENTITY_LASER_SHORT && sides[i][0] <= ENTITY_LASER_LONG) && GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Number == sides[i][1])
						m_Size++;
			}
		if(m_Size)
		{
			m_SDoors = new SDoors[m_Size];
			mem_zero(m_SDoors, m_Size*sizeof(SDoors));
			int num=0;
			for(int y = 0; y < pTileMap->m_Height; y++)
				for(int x = 0; x < pTileMap->m_Width; x++)
					if(GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Type == (ENTITY_DOOR + ENTITY_OFFSET))
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
						for(int i=0; i<8;i++)
							if ((sides[i][0] >= ENTITY_LASER_SHORT && sides[i][0] <= ENTITY_LASER_LONG) && GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Number == sides[i][1])
							{
								vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
								m_SDoors[num].m_Address = new CDoor(&GameServer()->m_World, Pos, pi/4*i, (32*3 + 32*(sides[i][0] - ENTITY_LASER_SHORT)*3), false);
								m_SDoors[num].m_Pos = Pos;
								m_SDoors[num++].m_Number = GameServer()->Collision()->SwitchLayer()[y*pTileMap->m_Width+x].m_Number;
							}
					}
		}
	}

}
