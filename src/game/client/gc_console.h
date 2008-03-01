#ifndef _GC_CONSOLE_H
#define _GC_CONSOLE_H

void console_handle_input();
void console_toggle(int tpye);
void console_render();
int console_active();
void client_console_init();
void console_rcon_print(const char *line);

#endif
