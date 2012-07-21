/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>


enum
{
	OP_ADD=0,
	OP_FIX,
	OP_REM,
	OP_SET
};

typedef struct
{
	unsigned char r, g, b, a;
} CPixel;


void TilesetBorderAdd(int w, int h, CPixel *pSrc, CPixel *pDest)
{
	int TileW = w/16;
	int TileH = h/16;

	for(int tx = 0; tx < 16; tx++)
	{
		for(int ty = 0; ty < 16; ty++)
		{
			for(int x = 0; x < TileW + 4; x++)
			{
				for(int y = 0; y < TileH + 4; y++)
				{
					int SourceX = tx * TileW + clamp(x - 2, 0, TileW - 1);
					int SourceY = ty * TileH + clamp(y - 2, 0, TileH - 1);
					int DestX = tx * (TileW + 4) + x;
					int DestY = ty * (TileH + 4) + y;

					int SourceI = SourceY * w + SourceX;
					int DestI = DestY * (w + 16 * 4) + DestX;

					pDest[DestI] = pSrc[SourceI];

				}
			}
		}
	}
}


CPixel Sample(int x, int y, int w, int h, CPixel *pData, int Pitch, float u, float v)
{
	x = x + (int)(w*u);
	y = y + (int)(h*v);
	return pData[y*Pitch+x];
}

void TilesetBorderFix(int w, int h, CPixel *pSrc, CPixel *pDest)
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


void TilesetBorderRem(int w, int h, CPixel *pSrc, CPixel *pDest)
{
	int TileW = w/16;
	int TileH = h/16;

	for(int tx = 0; tx < 16; tx++)
	{
		for(int ty = 0; ty < 16; ty++)
		{
			for(int x = 0; x < TileW - 4; x++)
			{
				for(int y = 0; y < TileH - 4; y++)
				{
					int SourceX = tx * TileW + x + 2;
					int SourceY = ty * TileH + y + 2;
					int DestX = tx * (TileW - 4) + x;
					int DestY = ty * (TileH - 4) + y;

					int SourceI = SourceY * w + SourceX;
					int DestI = DestY * (w - 16 * 4) + DestX;

					pDest[DestI] = pSrc[SourceI];

				}
			}
		}
	}
}


void TilesetBorderSet(int w, int h, CPixel *pSrc, CPixel *pDest)
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


void ProcessFile(const char *pFileName, int WidthMod, int HeightMod, int Operator)
{
	png_t Png;
	CPixel *pBuffer[2] = {0,0};

	png_init(0, 0);
	png_open_file(&Png, pFileName);

	if(Png.color_type != PNG_TRUECOLOR_ALPHA)
	{
		dbg_msg("tileset_borderop", "%s: not an RGBA image", pFileName);
		return;
	}

	int w = Png.width;
	int h = Png.height;

	pBuffer[0] = (CPixel*)mem_alloc(w*h*sizeof(CPixel), 1);
	pBuffer[1] = (CPixel*)mem_alloc((w+WidthMod)*(h+HeightMod)*sizeof(CPixel), 1);
	png_get_data(&Png, (unsigned char *)pBuffer[0]);
	png_close_file(&Png);

	switch(Operator)
	{
	case OP_ADD: TilesetBorderAdd(w, h, pBuffer[0], pBuffer[1]); break;
	case OP_FIX: TilesetBorderFix(w, h, pBuffer[0], pBuffer[1]); break;
	case OP_REM: TilesetBorderRem(w, h, pBuffer[0], pBuffer[1]); break;
	case OP_SET: TilesetBorderSet(w, h, pBuffer[0], pBuffer[1]); break;
	}

	// save here
	png_open_file_write(&Png, pFileName);
	png_set_data(&Png, w + WidthMod, h + HeightMod, 8, PNG_TRUECOLOR_ALPHA, (unsigned char *)pBuffer[1]);
	png_close_file(&Png);
}

int main(int argc, const char **argv)
{
	dbg_logger_stdout();
	if(argc < 3)
	{
		dbg_msg("Usage", "%s OPERATION FILE1 [ FILE2... ]", argv[0]);
		dbg_msg("Usage", "OPERATION - ADD, FIX, REM, SET");
		return -1;
	}

	int WidthMod, HeightMod, Operator;

	if(str_comp_nocase(argv[1], "ADD") == 0)
	{
		WidthMod = 16*4;
		HeightMod = 16*4;
		Operator = OP_ADD;
	}
	else if(str_comp_nocase(argv[1], "FIX") == 0)
	{
		WidthMod = 0;
		HeightMod = 0;
		Operator = OP_FIX;
	}
	else if(str_comp_nocase(argv[1], "REM") == 0)
	{
		WidthMod = -16*4;
		HeightMod = -16*4;
		Operator = OP_REM;
	}
	else if(str_comp_nocase(argv[1], "SET") == 0)
	{
		WidthMod = 0;
		HeightMod = 0;
		Operator = OP_SET;
	}
	else
	{
		dbg_msg("Usage", "invalid OPERATION (ADD, FIX, REM, SET)");
		return -1;
	}

	for(int i = 2; i < argc; i++)
		ProcessFile(argv[i], WidthMod, HeightMod, Operator);
	
	return 0;
}
