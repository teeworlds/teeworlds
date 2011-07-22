/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>

typedef struct
{
	unsigned char r, g, b, a;
} CPixel;

static CPixel Sample(int x, int y, int w, int h, CPixel *pData, int Pitch, float u, float v)
{
	x = x + (int)(w*u);
	y = y + (int)(h*v);
	return pData[y*Pitch+x];
}

static void TilesetBorderfix(int w, int h, CPixel *pSrc, CPixel *pDest)
{
	int TileW = w/16;
	int TileH = h/16;

	mem_zero(pDest, sizeof(CPixel)*w*h);

	for(int ty = 0; ty < 16; ty++)
	{
		for(int tx = 0; tx < 16; tx++)
		{
			for(int y = 0; y < TileH-2; y++)
			{
				for(int x = 0; x < TileW-2; x++)
				{
					float u = 0.5f/TileW + x/(float)(TileW-2);
					float v = 0.5f/TileH + y/(float)(TileH-2);
					int k = (ty*TileH+1+y)*w + tx*TileW+x+1;
					pDest[k] = Sample(tx*TileW, ty*TileH, TileW, TileH, pSrc, w, u, v);

					if(x == 0) pDest[k-1] = pDest[k];
					if(x == TileW-2-1) pDest[k+1] = pDest[k];
					if(y == 0) pDest[k-w] = pDest[k];
					if(y == TileH-2-1) pDest[k+w] = pDest[k];

					if(x == 0 && y == 0) pDest[k-w-1] = pDest[k];
					if(x == TileW-2-1 && y == 0) pDest[k-w+1] = pDest[k];
					if(x == 0 && y == TileH-2-1) pDest[k+w-1] = pDest[k];
					if(x == TileW-2-1 && y == TileH-2-1) pDest[k+w+1] = pDest[k];
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
		dbg_msg("tileset_borderfix", "%s: not an RGBA image", pFileName);
		return 1;
	}

	int w = Png.width;
	int h = Png.height;

	pBuffer[0] = (CPixel*)mem_alloc(w*h*sizeof(CPixel), 1);
	pBuffer[1] = (CPixel*)mem_alloc(w*h*sizeof(CPixel), 1);
	png_get_data(&Png, (unsigned char *)pBuffer[0]);
	png_close_file(&Png);

	TilesetBorderfix(w, h, pBuffer[0], pBuffer[1]);

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
