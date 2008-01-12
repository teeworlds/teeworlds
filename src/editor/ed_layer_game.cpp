#include <game/client/gc_mapres_tilemap.h>
#include "editor.hpp"


LAYER_GAME::LAYER_GAME(int w, int h)
: LAYER_TILES(w, h)
{
	type_name = "Game";
	game = 1;
}

LAYER_GAME::~LAYER_GAME()
{
}

void LAYER_GAME::render_properties(RECT *toolbox)
{
	LAYER_TILES::render_properties(toolbox);
	image = -1;
}
