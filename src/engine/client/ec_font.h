/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef _FONT_H
#define _FONT_H

typedef struct
{
    float tex_x0;
    float tex_y0;
    float tex_x1;
    float tex_y1;
    float width;
    float height;
    float x_offset;
    float y_offset;
    float x_advance;
}
CHARACTER;

typedef struct
{
    int text_texture;
    int outline_texture;
    int size;
    CHARACTER characters[256];
    char kerning[256*256];
} FONT;

typedef struct
{
    int font_count;
    FONT fonts[14];
} FONT_SET;

int font_load(FONT *font, const char *filename);
int font_set_load(FONT_SET *font_set, const char *font_filename, const char *text_texture_filename, const char *outline_texture_filename, int fonts, ...);
float font_text_width(FONT *font, const char *text, float size, int width);
void font_character_info(FONT *font, unsigned char c, float *tex_x0, float *tex_y0, float *tex_x1, float *tex_y1, float *width, float *height, float *x_offset, float *y_offset, float *x_advance);
float font_kerning(FONT *font, unsigned char c1, unsigned char c2);
FONT *font_set_pick(FONT_SET *font_set, float size);

#endif
