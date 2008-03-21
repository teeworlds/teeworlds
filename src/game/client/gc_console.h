#ifndef _GC_CONSOLE_H
#define _GC_CONSOLE_H

#include <engine/e_client_interface.h>

bool console_input_cli(INPUT_EVENT e, void *user_data);
bool console_input_special_binds(INPUT_EVENT e, void *user_data);
bool console_input_normal_binds(INPUT_EVENT e, void *user_data);

//void console_handle_input();

void console_toggle(int tpye);
void console_render();
int console_active();
void client_console_init();
void console_rcon_print(const char *line);

#endif
