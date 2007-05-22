#ifndef _UI_H
#define _UI_H
/*
extern void *hot_item;
extern void *active_item;
extern void *becomming_hot_item;
extern float mouse_x, mouse_y; // in gui space
extern float mouse_wx, mouse_wy; // in world space
extern unsigned mouse_buttons;*/

int ui_update(float mx, float my, float mwx, float mwy, int buttons);

float ui_mouse_x();
float ui_mouse_y();
float ui_mouse_world_x();
float ui_mouse_world_y();
int ui_mouse_button(int index);

void ui_set_hot_item(void *id);
void ui_set_active_item(void *id);
void *ui_hot_item();
void *ui_active_item();

int ui_mouse_inside(float x, float y, float w, float h);

typedef void (*draw_button_callback)(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra);

void ui_do_image(int texture, float x, float y, float w, float h);
void ui_do_label(float x, float y, char *text);
int ui_do_button(void *id, const char *text, int checked, float x, float y, float w, float h, draw_button_callback draw_func, void *extra);
int ui_do_button(void *id, const char *text, int checked, float x, float y, float w, float h, draw_button_callback draw_func);

#endif
