#ifndef __MENU_H
#define __MENU_H

extern int cursor_texture;

void draw_image_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra);
void draw_single_part_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra);
void draw_menu_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra);
void draw_teewars_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra);
int ui_do_key_reader(void *id, float x, float y, float w, float h, int key);
int ui_do_combo_box(void *id, float x, float y, float w, char *lines, int line_count, int selected_index);
int ui_do_edit_box(void *id, float x, float y, float w, float h, char *str, int str_size);
int ui_do_check_box(void *id, float x, float y, float w, float h, int value);
int do_scroll_bar_horiz(void *id, float x, float y, float width, int steps, int last_index);
int do_scroll_bar_vert(void *id, float x, float y, float height, int steps, int last_index);

#endif
