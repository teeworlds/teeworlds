/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef FILE_GAME_CLIENT_UI_H
#define FILE_GAME_CLIENT_UI_H

typedef struct 
{
    float x, y, w, h;
} RECT;

enum
{
	CORNER_TL=1,
	CORNER_TR=2,
	CORNER_BL=4,
	CORNER_BR=8,
	
	CORNER_T=CORNER_TL|CORNER_TR,
	CORNER_B=CORNER_BL|CORNER_BR,
	CORNER_R=CORNER_TR|CORNER_BR,
	CORNER_L=CORNER_TL|CORNER_BL,
	
	CORNER_ALL=CORNER_T|CORNER_B
};

int ui_update(float mx, float my, float mwx, float mwy, int buttons);

float ui_mouse_x();
float ui_mouse_y();
float ui_mouse_world_x();
float ui_mouse_world_y();
int ui_mouse_button(int index);
int ui_mouse_button_clicked(int index);

void ui_set_hot_item(const void *id);
void ui_set_active_item(const void *id);
void ui_clear_last_active_item();
const void *ui_hot_item();
const void *ui_next_hot_item();
const void *ui_active_item();
const void *ui_last_active_item();

int ui_mouse_inside(const RECT *r);

RECT *ui_screen();
void ui_set_scale(float s);
float ui_scale();
void ui_clip_enable(const RECT *r);
void ui_clip_disable();

void ui_hsplit_t(const RECT *original, float cut, RECT *top, RECT *bottom);
void ui_hsplit_b(const RECT *original, float cut, RECT *top, RECT *bottom);
void ui_vsplit_mid(const RECT *original, RECT *left, RECT *right);
void ui_vsplit_l(const RECT *original, float cut, RECT *left, RECT *right);
void ui_vsplit_r(const RECT *original, float cut, RECT *left, RECT *right);

void ui_margin(const RECT *original, float cut, RECT *other_rect);
void ui_vmargin(const RECT *original, float cut, RECT *other_rect);
void ui_hmargin(const RECT *original, float cut, RECT *other_rect);

typedef void (*ui_draw_button_func)(const void *id, const char *text, int checked, const RECT *r, const void *extra);
int ui_do_button(const void *id, const char *text, int checked, const RECT *r, ui_draw_button_func draw_func, const void *extra);
void ui_do_label(const RECT *r, const char *text, float size, int align, int max_width = -1);


#endif
