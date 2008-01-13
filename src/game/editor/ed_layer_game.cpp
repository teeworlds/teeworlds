#include "ed_editor.hpp"


LAYER_GAME::LAYER_GAME(int w, int h)
: LAYER_TILES(w, h)
{
	type_name = "Game";
	game = 1;
}

LAYER_GAME::~LAYER_GAME()
{
}

int LAYER_GAME::render_properties(RECT *toolbox)
{
	int r = LAYER_TILES::render_properties(toolbox);
	image = -1;
	return r;
}
