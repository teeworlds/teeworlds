/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

// dependencies: image

//
int tilemap_init();

// renders the tilemaps
void tilemap_render(float scale, int fg);

struct mapres_tilemap
{
	int image;
	int width;
	int height;
	int x, y;
	int scale;
	int data;
	int main;
};
