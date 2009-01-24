#include <base/system.h>
#include <string.h>
#include <engine/e_client_interface.h>

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

static FONT_SET *default_font_set = 0;
void gfx_text_set_default_font(void *font)
{
	default_font_set = (FONT_SET *)font;
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

void gfx_text_ex(TEXT_CURSOR *cursor, const char *text, int length)
{
	FONT_SET *font_set = cursor->font_set;
	float screen_x0, screen_y0, screen_x1, screen_y1;
	float fake_to_screen_x, fake_to_screen_y;
	int actual_x, actual_y;

	FONT *font;
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

	if(!font_set)
		font_set = default_font_set;

	font = font_set_pick(font_set, actual_size);

	if (length < 0)
		length = strlen(text);
		
	end = text + length;

	/* if we don't want to render, we can just skip the first outline pass */
	i = 1;
	if(cursor->flags&TEXTFLAG_RENDER)
		i = 0;

	for(;i < 2; i++)
	{
		const unsigned char *current = (unsigned char *)text;
		int to_render = length;
		draw_x = cursor_x;
		draw_y = cursor_y;

		if(cursor->flags&TEXTFLAG_RENDER)
		{
			if (i == 0)
				gfx_texture_set(font->outline_texture);
			else
				gfx_texture_set(font->text_texture);

			gfx_quads_begin();
			if (i == 0)
				gfx_setcolor(0.0f, 0.0f, 0.0f, 0.3f*text_a);
			else
				gfx_setcolor(text_r, text_g, text_b, text_a);
		}

		while(to_render > 0)
		{
			int new_line = 0;
			int this_batch = to_render;
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
				
				this_batch = wlen;
			}
			
			if((const char *)current+this_batch > end)
				this_batch = (const char *)end-(const char *)current;
			
			to_render -= this_batch;

			while(this_batch-- > 0)
			{
				float tex_x0, tex_y0, tex_x1, tex_y1;
				float width, height;
				float x_offset, y_offset, x_advance;
				float advance;

				if(*current == '\n')
				{
					draw_x = cursor->start_x;
					draw_y += size;
					draw_x = (int)(draw_x * fake_to_screen_x) / fake_to_screen_x; /* realign */
					draw_y = (int)(draw_y * fake_to_screen_y) / fake_to_screen_y;
					current++;
					continue;
				}

				font_character_info(font, *current, &tex_x0, &tex_y0, &tex_x1, &tex_y1, &width, &height, &x_offset, &y_offset, &x_advance);

				if(cursor->flags&TEXTFLAG_RENDER)
				{
					gfx_quads_setsubset(tex_x0, tex_y0, tex_x1, tex_y1);
					gfx_quads_drawTL(draw_x+x_offset*size, draw_y+y_offset*size, width*size, height*size);
				}

				advance = x_advance + font_kerning(font, *current, *(current+1));
								
				if(cursor->flags&TEXTFLAG_STOP_AT_END && draw_x+advance*size-cursor->start_x > cursor->line_width)
				{
					/* we hit the end of the line, no more to render or count */
					to_render = 0;
					break;
				}

				draw_x += advance*size;
				cursor->charcount++;
				current++;
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
