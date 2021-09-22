/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/console.h>
#include "editor.h"


CLayerGame::CLayerGame(int w, int h)
: CLayerTiles(w, h)
{
	str_copy(m_aName, "Game", sizeof(m_aName));
	m_Game = 1;
}

CLayerGame::~CLayerGame()
{
}

int CLayerGame::RenderProperties(CUIRect *pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}

CLayerWater::CLayerWater(int w, int h)
	: CLayerTiles(w, h)
{
	str_copy(m_aName, "Water", sizeof(m_aName));
	m_Water = 1; //Set Defaults
	m_Color.r = 0;
	m_Color.g = 162;
	m_Color.b = 255;
	m_Color.a = 64;
}

CLayerWater::~CLayerWater()
{
}

int CLayerWater::RenderProperties(CUIRect* pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}
