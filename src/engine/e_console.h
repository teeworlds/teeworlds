#ifndef _CONSOLE_H
#define _CONSOLE_H

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*CONSOLE_CALLBACK)(void *result, void *user_data);

typedef struct COMMAND_t
{
	const char *name;
	const char *params;
	CONSOLE_CALLBACK callback;
	void *user_data;
	struct COMMAND_t *next;
} COMMAND;

void console_init();
void console_register(COMMAND *cmd);
void console_execute_line(const char *str);
void console_execute_line_stroked(int stroke, const char *str);
void console_execute_file(const char *filename);
void console_print(const char *str);
void console_register_print_callback(void (*callback)(const char *, void *user_data), void *user_data);

/*int console_result_string(void *result, int index, const char **str);
int console_result_int(void *result, int index, int *i);
int console_result_float(void *result, int index, float *f);*/

const char *console_arg_string(void *result, int index);
int console_arg_int(void *result, int index);
float console_arg_float(void *result, int index);
int console_arg_num(void *result);

#define MACRO_REGISTER_COMMAND(name, params, func, ptr) { static COMMAND cmd = { name, params, func, ptr, 0x0 }; console_register(&cmd); }

#ifdef __cplusplus
}
#endif

#endif
