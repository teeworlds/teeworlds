/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <engine/external/pnglite/pnglite.h>

typedef struct
{
	unsigned char r, g, b, a;
} CPixel;

static void Dilate(int w, int h, CPixel *pSrc, CPixel *pDest)
{
	int ix, iy;
	const int xo[] = {0, -1, 1, 0};
	const int yo[] = {-1, 0, 0, 1};

	int m = 0;
	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++, m++)
		{
			pDest[m] = pSrc[m];
			if(pSrc[m].a)
				continue;

			for(int c = 0; c < 4; c++)
			{
				ix = clamp(x + xo[c], 0, w-1);
				iy = clamp(y + yo[c], 0, h-1);
				int k = iy*w+ix;
				if(pSrc[k].a)
				{
					pDest[m] = pSrc[k];
					pDest[m].a = 255;
					break;
				}
			}
		}
	}
}

static void CopyAlpha(int w, int h, CPixel *pSrc, CPixel *pDest)
{
	int m = 0;
	for(int y = 0; y < h; y++)
		for(int x = 0; x < w; x++, m++)
			pDest[m].a = pSrc[m].a;
}

int main(int argc, char **argv)
{
	png_t Png;
	CPixel *pBuffer[3] = {0,0,0};

	png_init(0, 0);
	png_open_file(&Png, argv[1]);

	if(Png.color_type != PNG_TRUECOLOR_ALPHA)
	{
		dbg_msg("dilate", "not an RGBA image");
		return -1;
	}

	pBuffer[0] = (CPixel*)mem_alloc(Png.width*Png.height*sizeof(CPixel), 1);
	pBuffer[1] = (CPixel*)mem_alloc(Png.width*Png.height*sizeof(CPixel), 1);
	pBuffer[2] = (CPixel*)mem_alloc(Png.width*Png.height*sizeof(CPixel), 1);
	png_get_data(&Png, (unsigned char *)pBuffer[0]);
	png_close_file(&Png);

	int w = Png.width;
	int h = Png.height;

	Dilate(w, h, pBuffer[0], pBuffer[1]);
	for(int i = 0; i < 5; i++)
	{
		Dilate(w, h, pBuffer[1], pBuffer[2]);
		Dilate(w, h, pBuffer[2], pBuffer[1]);
	}

	CopyAlpha(w, h, pBuffer[0], pBuffer[1]);

	// save here
	png_open_file_write(&Png, argv[1]);
	png_set_data(&Png, w, h, 8, PNG_TRUECOLOR_ALPHA, (unsigned char *)pBuffer[1]);
	png_close_file(&Png);

	return 0;
}
