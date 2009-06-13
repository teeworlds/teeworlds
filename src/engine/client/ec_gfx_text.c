#include <base/system.h>
#include <string.h>
#include <engine/e_client_interface.h>


#ifdef CONF_PLATFORM_MACOSX
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

static int word_length(const char *text)
{
	int s = 1;
	while(1)
	{
		if(*text == 0)
			return s-1;
		if(*text == '\n' || *text == '\t' || *text == ' ')
			return s;
		text++;
		s++;
	}
}

static float text_r=1;
static float text_g=1;
static float text_b=1;
static float text_a=1;

static struct FONT *default_font = 0;
void gfx_text_set_default_font(struct FONT *font)
{
	default_font = font;
}


void gfx_text_set_cursor(TEXT_CURSOR *cursor, float x, float y, float font_size, int flags)
{
	mem_zero(cursor, sizeof(*cursor));
	cursor->font_size = font_size;
	cursor->start_x = x;
	cursor->start_y = y;
	cursor->x = x;
	cursor->y = y;
	cursor->line_count = 1;
	cursor->line_width = -1;
	cursor->flags = flags;
	cursor->charcount = 0;
}


void gfx_text(void *font_set_v, float x, float y, float size, const char *text, int max_width)
{
	TEXT_CURSOR cursor;
	gfx_text_set_cursor(&cursor, x, y, size, TEXTFLAG_RENDER);
	cursor.line_width = max_width;
	gfx_text_ex(&cursor, text, -1);
}

float gfx_text_width(void *font_set_v, float size, const char *text, int length)
{
	TEXT_CURSOR cursor;
	gfx_text_set_cursor(&cursor, 0, 0, size, 0);
	gfx_text_ex(&cursor, text, length);
	return cursor.x;
}

void gfx_text_color(float r, float g, float b, float a)
{
	text_r = r;
	text_g = g;
	text_b = b;
	text_a = a;
}

/* ft2 texture */
#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ft_library;

#define MAX_CHARACTERS 64


/* GL_LUMINANCE can be good for debugging*/
static int font_texture_format = GL_ALPHA;


static int font_sizes[] = {8,9,10,11,12,13,14,15,16,17,18,19,20,36};
#define NUM_FONT_SIZES (sizeof(font_sizes)/sizeof(int))


typedef struct FONTCHAR
{
	int id;
	
	/* these values are scaled to the font size */
	/* width * font_size == real_size */
	float width;
	float height;
	float offset_x;
	float offset_y;
	float advance_x;
	
	float uvs[4];
	int64 touch_time;
} FONTCHAR;

typedef struct FONTSIZEDATA
{
	int font_size;
	FT_Face *face;

	unsigned textures[2];
	int texture_width;
	int texture_height;
	
	int num_x_chars;
	int num_y_chars;
	
	int char_max_width;
	int char_max_height;
	
	FONTCHAR characters[MAX_CHARACTERS*MAX_CHARACTERS];
	
	int current_character;	
} FONTSIZEDATA;

typedef struct FONT
{
	char filename[128];
	FT_Face ft_face;
	FONTSIZEDATA sizes[NUM_FONT_SIZES];
} FONT;

static int font_get_index(int pixelsize)
{
	int i;
	for(i = 0; i < NUM_FONT_SIZES; i++)
	{
		if(font_sizes[i] >= pixelsize)
			return i;
	}
	
	return NUM_FONT_SIZES-1;
}

FONT *gfx_font_load(const char *filename)
{
	int i;
	FONT *font = mem_alloc(sizeof(FONT), 1);
	
	mem_zero(font, sizeof(*font));
	str_copy(font->filename, filename, sizeof(font->filename));
	
	if(FT_New_Face(ft_library, font->filename, 0, &font->ft_face))
	{
		mem_free(font);
		return NULL;
	}

	for(i = 0; i < NUM_FONT_SIZES; i++)
		font->sizes[i].font_size = -1;
		
	return font;
};

void gfx_font_destroy(FONT *font)
{
	mem_free(font);
}

void gfx_font_init()
{
	FT_Init_FreeType(&ft_library);
}

static void grow(unsigned char *in, unsigned char *out, int w, int h)
{
	int y, x;
	for(y = 0; y < h; y++) 
		for(x = 0; x < w; x++) 
		{ 
			int c = in[y*w+x]; 
			int s_y, s_x;

			for(s_y = -1; s_y <= 1; s_y++)
				for(s_x = -1; s_x <= 1; s_x++)
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

static void font_init_texture(FONTSIZEDATA *sizedata, int charwidth, int charheight, int xchars, int ychars)
{
	static int font_memory_usage = 0;
	int i;
	int width = charwidth*xchars;
	int height = charheight*ychars;
	void *mem = mem_alloc(width*height, 1);
	mem_zero(mem, width*height);
	
	if(sizedata->textures[0] == 0)
		glGenTextures(2, sizedata->textures);
	else
		font_memory_usage -= sizedata->texture_width*sizedata->texture_height*2;
	
	sizedata->num_x_chars = xchars;
	sizedata->num_y_chars = ychars;
	sizedata->texture_width = width;
	sizedata->texture_height = height;
	sizedata->current_character = 0;
	
	for(i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, sizedata->textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, font_texture_format, width, height, 0, font_texture_format, GL_UNSIGNED_BYTE, mem);
		font_memory_usage += width*height;
	}
	
	dbg_msg("", "font memory usage: %d", font_memory_usage);
	
	mem_free(mem);
}

static void font_increase_texture_size(FONTSIZEDATA *sizedata)
{
	if(sizedata->texture_width < sizedata->texture_height)
		sizedata->num_x_chars <<= 1;
	else
		sizedata->num_y_chars <<= 1;
	font_init_texture(sizedata, sizedata->char_max_width, sizedata->char_max_height, sizedata->num_x_chars, sizedata->num_y_chars);		
}

static void font_init_index(FONT *font, int index)
{
	int outline_thickness = 1;
	FONTSIZEDATA *sizedata = &font->sizes[index];
	
	sizedata->font_size = font_sizes[index];
	FT_Set_Pixel_Sizes(font->ft_face, 0, sizedata->font_size);
	
	if(sizedata->font_size >= 18)
		outline_thickness = 2;
		
	{
		unsigned glyph_index;
		int charcode;
		int max_h = 0;
		int max_w = 0;
		
		charcode = FT_Get_First_Char(font->ft_face, &glyph_index);
		while(glyph_index != 0)
		{   
			/* do stuff */
			FT_Load_Glyph(font->ft_face, glyph_index, FT_LOAD_DEFAULT);
			
			if(font->ft_face->glyph->metrics.width > max_w) max_w = font->ft_face->glyph->metrics.width;
			if(font->ft_face->glyph->metrics.height > max_h) max_h = font->ft_face->glyph->metrics.height;
			charcode = FT_Get_Next_Char(font->ft_face, charcode, &glyph_index);
		}
		
		max_w = (max_w>>6)+2+outline_thickness*2;
		max_h = (max_h>>6)+2+outline_thickness*2;
		
		for(sizedata->char_max_width = 1; sizedata->char_max_width < max_w; sizedata->char_max_width <<= 1);
		for(sizedata->char_max_height = 1; sizedata->char_max_height < max_h; sizedata->char_max_height <<= 1);
	}
	
	//dbg_msg("font", "init size %d, texture size %d %d", font->sizes[index].font_size, w, h);
	//FT_New_Face(ft_library, "data/fonts/vera.ttf", 0, &font->ft_face);
	font_init_texture(sizedata, sizedata->char_max_width, sizedata->char_max_height, 8, 8);
}

static FONTSIZEDATA *font_get_size(FONT *font, int pixelsize)
{
	int index = font_get_index(pixelsize);
	if(font->sizes[index].font_size != font_sizes[index])
		font_init_index(font, index);
	return &font->sizes[index];
}


static void font_upload_glyph(FONTSIZEDATA *sizedata, int texnum, int slot_id, int chr, const void *data)
{
	int x = (slot_id%sizedata->num_x_chars) * (sizedata->texture_width/sizedata->num_x_chars);
	int y = (slot_id/sizedata->num_x_chars) * (sizedata->texture_height/sizedata->num_y_chars);
	
	glBindTexture(GL_TEXTURE_2D, sizedata->textures[texnum]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
		sizedata->texture_width/sizedata->num_x_chars,
		sizedata->texture_height/sizedata->num_y_chars,
		font_texture_format, GL_UNSIGNED_BYTE, data);
}

/* 8k of data used for rendering glyphs */
static unsigned char glyphdata[(4096/64) * (4096/64)];
static unsigned char glyphdata_outlined[(4096/64) * (4096/64)];

static int font_get_slot(FONTSIZEDATA *sizedata)
{
	int char_count = sizedata->num_x_chars*sizedata->num_y_chars;
	if(sizedata->current_character < char_count)
	{
		int i = sizedata->current_character;
		sizedata->current_character++;
		return i;
	}

	/* kick out the oldest */
	/* TODO: remove this linear search */
	{
		int oldest = 0;
		int i;
		for(i = 1; i < char_count; i++)
		{
			if(sizedata->characters[i].touch_time < sizedata->characters[oldest].touch_time)
				oldest = i;
		}
		
		if(time_get()-sizedata->characters[oldest].touch_time < time_freq())
		{
			font_increase_texture_size(sizedata);
			return font_get_slot(sizedata);
		}
		
		return oldest;
	}
}

static int font_render_glyph(FONT *font, FONTSIZEDATA *sizedata, int chr)
{
	FT_Bitmap *bitmap;
	int slot_id = 0;
	int slot_w = sizedata->texture_width / sizedata->num_x_chars;
	int slot_h = sizedata->texture_height / sizedata->num_y_chars;
	int slot_size = slot_w*slot_h;
	int outline_thickness = 1;
	int x = 1;
	int y = 1;
	int px, py;

	FT_Set_Pixel_Sizes(font->ft_face, 0, sizedata->font_size);

	if(FT_Load_Char(font->ft_face, chr, FT_LOAD_RENDER|FT_LOAD_NO_BITMAP))
	{
		dbg_msg("font", "error loading glyph %d", chr);
		return -1;
	}

	bitmap = &font->ft_face->glyph->bitmap; 
	
	/* fetch slot */
	slot_id = font_get_slot(sizedata);
	if(slot_id < 0)
		return -1;
	
	/* adjust spacing */
	if(sizedata->font_size >= 18)
		outline_thickness = 2;
	x += outline_thickness;
	y += outline_thickness;

	/* prepare glyph data */
	mem_zero(glyphdata, slot_size);

	if(bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		for(py = 0; py < bitmap->rows; py++) 
			for(px = 0; px < bitmap->width; px++) 
				glyphdata[(py+y)*slot_w+px+x] = bitmap->buffer[py*bitmap->pitch+px];
	}
	else if(bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
	{
		for(py = 0; py < bitmap->rows; py++) 
			for(px = 0; px < bitmap->width; px++)
			{
				if(bitmap->buffer[py*bitmap->pitch+px/8]&(1<<(7-(px%8))))
					glyphdata[(py+y)*slot_w+px+x] = 255;
			}
	}

	if(0) for(py = 0; py < slot_w; py++) 
		for(px = 0; px < slot_h; px++) 
			glyphdata[py*slot_w+px] = 255;
	
	/* upload the glyph */
	font_upload_glyph(sizedata, 0, slot_id, chr, glyphdata);
	
	if(outline_thickness == 1)
	{
		grow(glyphdata, glyphdata_outlined, slot_w, slot_h);
		font_upload_glyph(sizedata, 1, slot_id, chr, glyphdata_outlined);
	}
	else
	{
		grow(glyphdata, glyphdata_outlined, slot_w, slot_h);
		grow(glyphdata_outlined, glyphdata, slot_w, slot_h);
		font_upload_glyph(sizedata, 1, slot_id, chr, glyphdata);
	}
	
	/* set char info */
	{
		FONTCHAR *fontchr = &sizedata->characters[slot_id];
		float scale = 1.0f/sizedata->font_size;
		float uscale = 1.0f/sizedata->texture_width;
		float vscale = 1.0f/sizedata->texture_height;
		int height = bitmap->rows + outline_thickness*2 + 2;
		int width = bitmap->width + outline_thickness*2 + 2;
		
		fontchr->id = chr;
		fontchr->height = height * scale;
		fontchr->width = width * scale;
		fontchr->offset_x = (font->ft_face->glyph->bitmap_left-1) * scale;
		fontchr->offset_y = (sizedata->font_size - font->ft_face->glyph->bitmap_top) * scale;
		fontchr->advance_x = (font->ft_face->glyph->advance.x>>6) * scale;
		
		fontchr->uvs[0] = (slot_id%sizedata->num_x_chars) / (float)(sizedata->num_x_chars);
		fontchr->uvs[1] = (slot_id/sizedata->num_x_chars) / (float)(sizedata->num_y_chars);
		fontchr->uvs[2] = fontchr->uvs[0] + width*uscale;
		fontchr->uvs[3] = fontchr->uvs[1] + height*vscale;
	}
	
	return slot_id;
}

static FONTCHAR *font_get_char(FONT *font, FONTSIZEDATA *sizedata, int chr)
{
	FONTCHAR *fontchr = NULL;
	
	/* search for the character */
	/* TODO: remove this linear search */
	int i;
	for(i = 0; i < sizedata->current_character; i++)
	{
		if(sizedata->characters[i].id == chr)
		{
			fontchr = &sizedata->characters[i];
			break;
		}
	}
	
	/* check if we need to render the character */
	if(!fontchr)
	{
		int index = font_render_glyph(font, sizedata, chr);
		if(index >= 0)
			fontchr = &sizedata->characters[index];
	}
	
	/* touch the character */
	/* TODO: don't call time_get here */
	if(fontchr)
		fontchr->touch_time = time_get();
		
	return fontchr;
}

/* must only be called from the rendering function as the font must be set to the correct size */
static void font_render_setup(FONT *font, int size)
{
	FT_Set_Pixel_Sizes(font->ft_face, 0, size);
}

static float font_kerning(FONT *font, int left, int right)
{
	FT_Vector kerning = {0,0};
	FT_Get_Kerning(font->ft_face, left, right, FT_KERNING_DEFAULT, &kerning);
	return (kerning.x>>6);
}

void gfx_text_ex(TEXT_CURSOR *cursor, const char *text, int length)
{
	FONT *font = cursor->font;
	FONTSIZEDATA *sizedata = NULL;

	float screen_x0, screen_y0, screen_x1, screen_y1;
	float fake_to_screen_x, fake_to_screen_y;
	int actual_x, actual_y;

	int actual_size;
	int i;
	int got_new_line = 0;
	float draw_x, draw_y;
	float cursor_x, cursor_y;
	const char *end;

	float size = cursor->font_size;

	/* to correct coords, convert to screen coords, round, and convert back */
	gfx_getscreen(&screen_x0, &screen_y0, &screen_x1, &screen_y1);
	
	fake_to_screen_x = (gfx_screenwidth()/(screen_x1-screen_x0));
	fake_to_screen_y = (gfx_screenheight()/(screen_y1-screen_y0));
	actual_x = cursor->x * fake_to_screen_x;
	actual_y = cursor->y * fake_to_screen_y;

	cursor_x = actual_x / fake_to_screen_x;
	cursor_y = actual_y / fake_to_screen_y;

	/* same with size */
	actual_size = size * fake_to_screen_y;
	size = actual_size / fake_to_screen_y;

	/* fetch font data */
	if(!font)
		font = default_font;
	
	if(!font)
		return;

	sizedata = font_get_size(font, actual_size);
	font_render_setup(font, actual_size);
	
	/* set length */
	if(length < 0)
		length = strlen(text);
		
	end = text + length;

	/* if we don't want to render, we can just skip the first outline pass */
	i = 1;
	if(cursor->flags&TEXTFLAG_RENDER)
		i = 0;

	for(;i < 2; i++)
	{
		const char *current = (char *)text;
		const char *end = current+length;
		draw_x = cursor_x;
		draw_y = cursor_y;

		if(cursor->flags&TEXTFLAG_RENDER)
		{
			// TODO: Make this better
			glEnable(GL_TEXTURE_2D);
			if (i == 0)
				glBindTexture(GL_TEXTURE_2D, sizedata->textures[1]);
			else
				glBindTexture(GL_TEXTURE_2D, sizedata->textures[0]);

			gfx_quads_begin();
			if (i == 0)
				gfx_setcolor(0.0f, 0.0f, 0.0f, 0.3f*text_a);
			else
				gfx_setcolor(text_r, text_g, text_b, text_a);
		}

		while(current < end)
		{
			int new_line = 0;
			const char *batch_end = end;
			if(cursor->line_width > 0 && !(cursor->flags&TEXTFLAG_STOP_AT_END))
			{
				int wlen = word_length((char *)current);
				TEXT_CURSOR compare = *cursor;
				compare.x = draw_x;
				compare.y = draw_y;
				compare.flags &= ~TEXTFLAG_RENDER;
				compare.line_width = -1;
				gfx_text_ex(&compare, text, wlen);
				
				if(compare.x-draw_x > cursor->line_width)
				{
					/* word can't be fitted in one line, cut it */
					TEXT_CURSOR cutter = *cursor;
					cutter.charcount = 0;
					cutter.x = draw_x;
					cutter.y = draw_y;
					cutter.flags &= ~TEXTFLAG_RENDER;
					cutter.flags |= TEXTFLAG_STOP_AT_END;
					
					gfx_text_ex(&cutter, (const char *)current, wlen);
					wlen = cutter.charcount;
					new_line = 1;
					
					if(wlen <= 3) /* if we can't place 3 chars of the word on this line, take the next */
						wlen = 0;
				}
				else if(compare.x-cursor->start_x > cursor->line_width)
				{
					new_line = 1;
					wlen = 0;
				}
				
				batch_end = current + wlen;
			}
			
			while(current < batch_end)
			{
				const char *tmp;
				float advance = 0;
				int character = 0;
				int nextcharacter = 0;
				FONTCHAR *chr;

				// TODO: UTF-8 decode
				character = str_utf8_decode(&current);
				tmp = current;
				nextcharacter = str_utf8_decode(&tmp);
				
				if(character == '\n')
				{
					draw_x = cursor->start_x;
					draw_y += size;
					draw_x = (int)(draw_x * fake_to_screen_x) / fake_to_screen_x; /* realign */
					draw_y = (int)(draw_y * fake_to_screen_y) / fake_to_screen_y;
					continue;
				}

				chr = font_get_char(font, sizedata, character);

				if(chr)
				{
					if(cursor->flags&TEXTFLAG_RENDER)
					{
						gfx_quads_setsubset(chr->uvs[0], chr->uvs[1], chr->uvs[2], chr->uvs[3]);
						gfx_quads_drawTL(draw_x+chr->offset_x*size, draw_y+chr->offset_y*size, chr->width*size, chr->height*size);
					}

					advance = chr->advance_x + font_kerning(font, character, nextcharacter)/size;
				}
								
				if(cursor->flags&TEXTFLAG_STOP_AT_END && draw_x+advance*size-cursor->start_x > cursor->line_width)
				{
					/* we hit the end of the line, no more to render or count */
					current = end;
					break;
				}

				draw_x += advance*size;
				cursor->charcount++;
			}
			
			if(new_line)
			{
				draw_x = cursor->start_x;
				draw_y += size;
				got_new_line = 1;
				draw_x = (int)(draw_x * fake_to_screen_x) / fake_to_screen_x; /* realign */
				draw_y = (int)(draw_y * fake_to_screen_y) / fake_to_screen_y;				
			}
		}

		if(cursor->flags&TEXTFLAG_RENDER)
			gfx_quads_end();
	}

	cursor->x = draw_x;
	
	if(got_new_line)
		cursor->y = draw_y;
}
