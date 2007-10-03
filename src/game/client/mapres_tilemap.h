
// dependencies: image

//
int tilemap_init();

// renders the tilemaps
void tilemap_render(float scale, int fg);

enum
{
	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
};

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
