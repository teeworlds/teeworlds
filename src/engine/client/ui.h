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

#ifdef __cplusplus
}
#endif

#endif
