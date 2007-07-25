
#include "../engine/client/pnglite/pnglite.c"

typedef struct pixel_t
{
	unsigned char r, g, b, a;
} pixel;

static int clamp(int v, int min, int max)
{
	if(v > max)
		return max;
	if(v < min)
		return min;
	return v;
}

static void dilate(int w, int h, pixel *src, pixel *dst)
{
	int x, y, k, m, c;
	int ix, iy;
	const int xo[] = {0, -1, 1, 0};
	const int yo[] = {-1, 0, 0, 1};

	m = 0;
	for(y = 0; y < h; y++)
	{
		for(x = 0; x < w; x++, m++)
		{
			dst[m] = src[m];
			if(src[m].a)
				continue;
			
			for(c = 0; c < 4; c++)
			{
				ix = clamp(x + xo[c], 0, w-1);
				iy = clamp(y + yo[c], 0, h-1);
				k = iy*w+ix;
				if(src[k].a)
				{
					dst[m] = src[k];
					dst[m].a = 255;
					break;
				}
			}
		}
	}
}

static void copy_alpha(int w, int h, pixel *src, pixel *dst)
{
	int x, y, m;

	m = 0;
	for(y = 0; y < h; y++)
		for(x = 0; x < w; x++, m++)
			dst[m].a = src[m].a;
}

int main(int argc, char **argv)
{
	png_t png;
	pixel *buffer[3] = {0,0,0};
	int i, w, h;
	
	png_init(0,0);
	png_open_file(&png, argv[1]);
	
	if(png.color_type != PNG_TRUECOLOR_ALPHA)
	{
		printf("not an RGBA image\n");
		return -1;
	}
	
	buffer[0] = (pixel*)malloc(png.width*png.height*sizeof(pixel));
	buffer[1] = (pixel*)malloc(png.width*png.height*sizeof(pixel));
	buffer[2] = (pixel*)malloc(png.width*png.height*sizeof(pixel));
	png_get_data(&png, (unsigned char *)buffer[0]);
	png_close_file(&png);
	
	w = png.width;
	h = png.height;
	
	dilate(w, h, buffer[0], buffer[1]);
	for(i = 0; i < 5; i++)
	{
		dilate(w, h, buffer[1], buffer[2]);
		dilate(w, h, buffer[2], buffer[1]);
	}
	
	copy_alpha(w, h, buffer[0], buffer[1]);
	
	// save here
	png_open_file_write(&png, argv[1]);
	png_set_data(&png, w, h, 8, PNG_TRUECOLOR_ALPHA, (unsigned char *)buffer[1]);
	png_close_file(&png);
	
	return 0;
}
