
#include "../engine/external/pnglite/pnglite.c"

typedef struct pixel_t
{
	unsigned char r, g, b, a;
} pixel;

static pixel sample(int x, int y, int w, int h, pixel *data, int pitch, float u, float v)
{
	x = x + (int)(w*u);
	y = y + (int)(h*v);
	return data[y*pitch+x];
}

static void tilemap_borderfix(int w, int h, pixel *src, pixel *dst)
{
	int tilew = w/16;
	int tileh = h/16;
	int tx, ty;
	int x, y;
	int k;
	float u, v;
	
	memset(dst, 0, sizeof(pixel)*w*h);
				
	for(ty = 0; ty < 16; ty++)
		for(tx = 0; tx < 16; tx++)
		{
			for(y = 0; y < tileh-2; y++)
				for(x = 0; x < tilew-2; x++)
				{
					u = 0.5f/tilew + x/(float)(tilew-2);
					v = 0.5f/tileh + y/(float)(tileh-2);
					k = (ty*tileh+1+y)*w + tx*tilew+x+1;
					dst[k] = sample(tx*tilew, ty*tileh, tilew, tileh, src, w, u, v);
					
					if(x == 0) dst[k-1] = dst[k];
					if(x == tilew-2-1) dst[k+1] = dst[k];
					if(y == 0) dst[k-w] = dst[k];
					if(y == tileh-2-1) dst[k+w] = dst[k];
					
					if(x == 0 && y == 0) dst[k-w-1] = dst[k];
					if(x == tilew-2-1 && y == 0) dst[k-w+1] = dst[k];
					if(x == 0 && y == tileh-2-1) dst[k+w-1] = dst[k];
					if(x == tilew-2-1 && y == tileh-2-1) dst[k+w+1] = dst[k];
				}
		}
}


int main(int argc, char **argv)
{
	png_t png;
	pixel *buffer[2] = {0,0};
	int w, h;
	
	png_init(0,0);
	png_open_file(&png, argv[1]);
	
	if(png.color_type != PNG_TRUECOLOR_ALPHA)
	{
		printf("not an RGBA image\n");
		return -1;
	}
	
	w = png.width;
	h = png.height;
	
	buffer[0] = (pixel*)malloc(w*h*sizeof(pixel));
	buffer[1] = (pixel*)malloc(w*h*sizeof(pixel));
	png_get_data(&png, (unsigned char *)buffer[0]);
	png_close_file(&png);
	
	tilemap_borderfix(w, h, buffer[0], buffer[1]);
	
	// save here
	png_open_file_write(&png, argv[1]);
	png_set_data(&png, w, h, 8, PNG_TRUECOLOR_ALPHA, (unsigned char *)buffer[1]);
	png_close_file(&png);
		
	return 0;
}
