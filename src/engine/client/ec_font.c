/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdarg.h>
#include <stdio.h>
#include <engine/e_system.h>
#include "ec_font.h"

typedef struct
{
    short x, y;
    short width, height;
    short x_offset, y_offset;
    short x_advance;
} FONT_CHARACTER;

typedef struct
{
    short size;
    short width, height;

    short line_height;
    short base;

    FONT_CHARACTER characters[256];

    short kerning[256*256];
} FONT_DATA;

int font_load(FONT *font, const char *filename)
{
    FONT_DATA font_data;
	IOHANDLE file;

	dbg_msg("font/load", "loading %s", filename);
	file = io_open(filename, IOFLAG_READ);
	
	if(file)
	{
        int i;

        io_read(file, &font_data, sizeof(FONT_DATA));
        io_close(file);

#if defined(CONF_ARCH_ENDIAN_BIG)
        swap_endian(&font_data, 2, sizeof(FONT_DATA)/2);
#endif

        dbg_msg("font/load", "width: %d, height: %d, sizeof(FONT_DATA): %d", font_data.width, font_data.height, sizeof(FONT_DATA));

        {
            float scale_factor_x = 1.0f/font_data.size;
            float scale_factor_y = 1.0f/font_data.size;
            float scale_factor_tex_x = 1.0f/font_data.width;
            float scale_factor_tex_y = 1.0f/font_data.height;

            for (i = 0; i < 256; i++)
            {
                float tex_x0 = font_data.characters[i].x*scale_factor_tex_x;
                float tex_y0 = font_data.characters[i].y*scale_factor_tex_y;
                float tex_x1 = (font_data.characters[i].x+font_data.characters[i].width)*scale_factor_tex_x;
                float tex_y1 = (font_data.characters[i].y+font_data.characters[i].height)*scale_factor_tex_y;

                float width = font_data.characters[i].width*scale_factor_x;
                float height = font_data.characters[i].height*scale_factor_y;
                float x_offset = font_data.characters[i].x_offset*scale_factor_x;
                float y_offset = font_data.characters[i].y_offset*scale_factor_y;
                float x_advance = (font_data.characters[i].x_advance>>6)*scale_factor_x;

                font->characters[i].tex_x0 = tex_x0;
                font->characters[i].tex_y0 = tex_y0;
                font->characters[i].tex_x1 = tex_x1;
                font->characters[i].tex_y1 = tex_y1;
                font->characters[i].width = width;
                font->characters[i].height = height;
                font->characters[i].x_offset = x_offset;
                font->characters[i].y_offset = y_offset;
                font->characters[i].x_advance = x_advance;

            }

            for (i = 0; i < 256*256; i++)
            {
                font->kerning[i] = font_data.kerning[i]*scale_factor_x;
            }
        }

        return 0;
    }
    else
        return -1;
}

int gfx_load_texture(const char *filename);

int font_set_load(FONT_SET *font_set, const char *font_filename, const char *texture_filename, int fonts, ...)
{
    int i;
    va_list va; 

    font_set->font_count = fonts;

    va_start(va, fonts); 
    for (i = 0; i < fonts; i++)
    {
        int size;
        char composed_font_filename[256];
        char composed_texture_filename[256];
        FONT *font = &font_set->fonts[i];

        size = va_arg(va, int);
        sprintf(composed_font_filename, font_filename, size);
        sprintf(composed_texture_filename, texture_filename, size);

        if (font_load(font, composed_font_filename))
        {
            va_end(va);
            return -1;
        }

        font->size = size;
        font->texture = gfx_load_texture(composed_texture_filename);

        dbg_msg("font_set/loading", "filename: %s", composed_font_filename);
    }

    va_end(va);
    return 0;
}

float font_set_string_width(FONT_SET *font_set, const char *string, float size)
{
    float width = 0.0f;

    const unsigned char *c = (unsigned char *)string;

    FONT *font = &font_set->fonts[0];

    while (*c)
    {
        float tex_x0, tex_y0, tex_x1, tex_y1;
        float char_width, char_height;
        float x_offset, y_offset, x_advance;

        font_character_info(font, *c, &tex_x0, &tex_y0, &tex_x1, &tex_y1, &char_width, &char_height, &x_offset, &y_offset, &x_advance);

        width += x_advance;

        c++;
    }

    return width*size;
}

void font_character_info(FONT *font, unsigned char c, float *tex_x0, float *tex_y0, float *tex_x1, float *tex_y1, float *width, float *height, float *x_offset, float *y_offset, float *x_advance)
{
    CHARACTER *character = &font->characters[c];

    *tex_x0 = character->tex_x0;
    *tex_y0 = character->tex_y0;
    *tex_x1 = character->tex_x1;
    *tex_y1 = character->tex_y1;
    *width = character->width;
    *height = character->height;
    *x_offset = character->x_offset;
    *y_offset = character->y_offset;
    *x_advance = character->x_advance;
}

float font_kerning(FONT *font, unsigned char c1, unsigned char c2)
{
    return font->kerning[c1 + c2*256];
}

FONT *font_set_pick(FONT_SET *font_set, float size)
{
    int i;
    FONT *picked_font = 0x0;

    for (i = font_set->font_count-1; i >= 0; i--)
    {
        FONT *font = &font_set->fonts[i];

        if (font->size >= size)
            picked_font = font;
    }

    if (!picked_font)
        picked_font = &font_set->fonts[font_set->font_count-1];

    return picked_font;
}
