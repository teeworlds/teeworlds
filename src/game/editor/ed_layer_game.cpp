#include "ed_editor.h"


CLayerGame::CLayerGame(int w, int h)
: CLayerTiles(w, h)
{
	m_pTypeName = "Game";
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
