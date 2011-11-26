/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>

typedef struct
{
	unsigned char r, g, b, a;
} CPixel;

static void TilesetBorderset(int w, int h, CPixel *pSrc, CPixel *pDest)
{
	int TileW = w/16;
	int TileH = h/16;

	for(int tx = 0; tx < 16; tx++)
	{
		for(int ty = 0; ty < 16; ty++)
		{
			for(int x = 0; x < TileW; x++)
			{
				for(int y = 0; y < TileH; y++)
				{
					#define TILE_INDEX(tx_, ty_, x_, y_) (((ty_) * TileH + (y_)) * w + (tx_) * TileW + (x_))
					pDest[TILE_INDEX(tx, ty, x, y)] = pSrc[TILE_INDEX(tx, ty, clamp(x, 2, TileW - 3), clamp(y, 2, TileH - 3))];
				}
			}
		}
	}
}


int FixFile(const char *pFileName)
{
	png_t Png;
	CPixel *pBuffer[2] = {0,0};

	png_init(0, 0);
	png_open_file(&Png, pFileName);

	if(Png.color_type != PNG_TRUECOLOR_ALPHA)
	{
		dbg_msg("tileset_borderset", "%s: not an RGBA image", pFileName);
		return 1;
	}

	int w = Png.width;
	int h = Png.height;

	pBuffer[0] = (CPixel*)mem_alloc(w*h*sizeof(CPixel), 1);
	pBuffer[1] = (CPixel*)mem_alloc(w*h*sizeof(CPixel), 1);
	png_get_data(&Png, (unsigned char *)pBuffer[0]);
	png_close_file(&Png);

	TilesetBorderset(w, h, pBuffer[0], pBuffer[1]);

	// save here
	png_open_file_write(&Png, pFileName);
	png_set_data(&Png, w, h, 8, PNG_TRUECOLOR_ALPHA, (unsigned char *)pBuffer[1]);
	png_close_file(&Png);

	return 0;
}

int main(int argc, const char **argv)
{
	dbg_logger_stdout();
	if(argc == 1)
	{
		dbg_msg("Usage", "%s FILE1 [ FILE2... ]", argv[0]);
		return -1;
	}

	for(int i = 1; i < argc; i++)
		FixFile(argv[i]);
	return 0;
}
