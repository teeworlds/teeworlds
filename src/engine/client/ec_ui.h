/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

int ui_update(float mx, float my, float mwx, float mwy, int buttons);

float ui_mouse_x();
float ui_mouse_y();
float ui_mouse_world_x();
float ui_mouse_world_y();
int ui_mouse_button(int index);

void ui_set_hot_item(const void *id);
void ui_set_active_item(const void *id);
void ui_clear_last_active_item();
const void *ui_hot_item();
const void *ui_active_item();
const void *ui_last_active_item();

int ui_mouse_inside(float x, float y, float w, float h);

typedef void (*draw_button_callback)(const void *id, const char *text, int checked, float x, float y, float w, float h, void *extra);

void ui_do_image(int texture, float x, float y, float w, float h);
void ui_do_label(float x, float y, const char *text, float size);
int ui_do_button(const void *id, const char *text, int checked, float x, float y, float w, float h, draw_button_callback draw_func, void *extra);

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
    int texture;
    CHARACTER characters[256];
    float kerning[256*256];
} FONT;

int font_load(FONT *font, const char *filename);
int font_save(FONT *font, const char *filename);
float font_string_width(FONT *font, const char *string, float size);
void font_character_info(FONT *font, unsigned char c, float *tex_x0, float *tex_y0, float *tex_x1, float *tex_y1, float *width, float *height, float *x_offset, float *y_offset, float *x_advance);
void font_render(FONT *font, const char *string, float x, float y, float size);

#ifdef __cplusplus
}
#endif

#endif
