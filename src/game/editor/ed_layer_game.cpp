// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#include "ed_editor.h"

#include <game/localization.h>


CLayerGame::CLayerGame(int w, int h)
: CLayerTiles(w, h)
{
	m_pTypeName = Localize("Game");
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
