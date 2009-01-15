#include <stdio.h> 
#include <ft2build.h> 
#include FT_FREETYPE_H 
 
FT_Library library; 
FT_Face face; 
 
const int max_width = 512; 
const int max_height = max_width; 
 
const char chars[] = "abcdefghijklmnopqrstuv"; 
 
void save(const char *filename, unsigned char *data, int w, int h) 
{ 
        char header[18] = {0}; 
        header[2] = 2; // 2=rgb 3=grayscale 
        header[12] = w&255; // width 
        header[13] = w>>8; 
        header[14] = h&255; // height 
        header[15] = h>>8; 
        header[16] = 32; 
        header[17] = (1<<5) | 8; // org 
 
        char tganame[128]; 
        sprintf(tganame, "%s.tga", filename); 
 
        FILE *f = fopen(tganame, "wb"); 
        fwrite(header, 1, sizeof(header), f); 
 
        for(int y = 0; y < h; y++) 
                for(int x = 0; x < w; x++) 
                { 
                        fputc(0xff, f); 
                        fputc(0xff, f); 
                        fputc(0xff, f); 
                        fputc(data[y*w+x], f); 
                } 

        fclose(f); 
 
        char cmd[512]; 
        sprintf(cmd, "convert %s.tga %s.png; rm %s.tga",filename, filename, filename); 
        system(cmd); 
} 

void grow(unsigned char *in, unsigned char *out, int w, int h)
{
        for(int y = 0; y < h; y++) 
                for(int x = 0; x < w; x++) 
                { 
                        int c = in[y*w+x]; 

                        for (int s_y = -1; s_y <= 1; s_y++)
                            for (int s_x = -1; s_x <= 1; s_x++)
                            {
                                int get_x = x+s_x;
                                int get_y = y+s_y;
                                if (get_x >= 0 && get_y >= 0 && get_x < w && get_y < h)
                                {
                                    int index = get_y*w+get_x;
                                    if(in[index] > c)
                                        c = in[index]; 
                                }
                            }

                        out[y*w+x] = c;
                } 
}
 
unsigned char alpha_buf[max_width*(max_height*4)] = {0}; 
unsigned char alpha_buf2[max_width*(max_height*4)] = {0}; 
unsigned char *alpha = &alpha_buf[max_width*2]; 
unsigned char *alpha2 = &alpha_buf2[max_width*2]; 
 
int gen(const char *filename, const char *output, int size, int border) 
{ 
        char buf[256]; 

		int w = 128; 
		int h = 128; 
 
		while (1)
		{
			bool continue_now = false;
			memset(alpha, 0, w*h); 

			sprintf(buf, "trying to generate %dx%d", w, h);
			puts(buf);
	 
			sprintf(buf, "%s%d.fnt", output, size); 
			FILE *fnt = fopen(buf, "wb"); 
			sprintf(buf, "%s%d.tga", output, size); 
			fprintf(fnt, "info face=\"%s\" size=%d bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=4 padding=0,0,0,0 spacing=1,1\n", output, size); 
			fprintf(fnt, "common lineHeight=%d base=26 scaleW=%d scaleH=%d pages=1 packed=0\n", size, w, h); 
			fprintf(fnt, "page id=0 file=\"%s\"\n", buf); 
	 
			int error = FT_New_Face(library, filename, 0, &face); 

			int outline_thickness = size >= 18 ? 2 : 1;
	 
			int spacing = outline_thickness + 1; 
			error = FT_Set_Pixel_Sizes(face, 0, size); 
			int x = spacing; 
			int y = spacing; 
			for(int i = 32; i < 255; i++) 
			{ 
					if((i >= 8*16-1 && i < 10*16) || i == 11*16-1) // remove some strange characters 
							continue; 
	 
					error = FT_Load_Char(face, i, FT_LOAD_RENDER); 
					if(error) 
							continue; 
					FT_Bitmap *bitmap = &face->glyph->bitmap; 
	 
					if(x + bitmap->width + spacing*2 >= w) 
					{ 
							x = spacing; 
							y += size+spacing*2; 
					} 
	 
					x += spacing; 
	 
					fprintf(fnt, "char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d page=0 chnl=15\n", 
							i, x-outline_thickness, y-outline_thickness, bitmap->width+2*outline_thickness, bitmap->rows+2*outline_thickness, face->glyph->bitmap_left-1, size-face->glyph->bitmap_top, face->glyph->advance.x>>6); 

					if (x + bitmap->width - 1 >= w || y + bitmap->rows - 1 >= h)
					{
						if (w == h)
							w <<= 1;
						else
							h <<= 1;

						continue_now = true;

						break;
					}
	 
					for(int py = 0; py < bitmap->rows; py++) 
							for(int px = 0; px < bitmap->width; px++) 
									alpha[(py+y)*w+px+x] = bitmap->buffer[py*bitmap->pitch+px]; 
					x += spacing; 
					x += bitmap->width; 
			} 
	 
			fclose(fnt); 

			if (continue_now)
				continue;
	 
			sprintf(buf, "%s%d", output, size); 
			save(buf, alpha, w, h);

			// generate outline
			int passes = size >= 18 ? 2 : 1;
			for (int i = 0; i < passes; i++)
			{
				grow(alpha, alpha2, w, h);

				unsigned char *temp = alpha;
				alpha = alpha2;
				alpha2 = temp;
			}

			sprintf(buf, "%s%d_b", output, size); 
			save(buf, alpha, w, h);

			break;
		}
}

void gen_font(char *font_name, char *output, int size_count, int *sizes)
{
        for(int i = 0; i < size_count; i++) 
                gen(font_name, output, sizes[i], 0); 
}
 
int main() 
{ 
        int error = FT_Init_FreeType(&library); 

        int tahoma_sizes[] = { 8, 9, 10, 11, 12 }; 
        gen_font("tahoma.ttf", "default_font", sizeof(tahoma_sizes)/sizeof(*tahoma_sizes), tahoma_sizes);

        int alte_haas_sizes[] = { 13, 14, 15, 16, 17 };
        gen_font("alte_haas_grotesk.ttf", "default_font", sizeof(alte_haas_sizes)/sizeof(*alte_haas_sizes), alte_haas_sizes);

        int alte_haas_bold_sizes[] = { 18, 19, 20, 36 };
        gen_font("alte_haas_grotesk_bold.ttf", "default_font", sizeof(alte_haas_bold_sizes)/sizeof(*alte_haas_bold_sizes), alte_haas_bold_sizes);
 
        return 0; 
} 
