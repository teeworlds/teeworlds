#ifndef _CONSOLE_H
#define _CONSOLE_H

#ifdef __cplusplus
extern "C"{
#endif

#define CONSOLE_MAX_STR_LENGTH 255
/* the maximum number of tokens occurs in a string of length CONSOLE_MAX_STR_LENGTH with tokens size 1 separated by single spaces */
#define MAX_TOKENS (CONSOLE_MAX_STR_LENGTH+1)/2
 
enum 
{ 
    TOKEN_INT, 
    TOKEN_FLOAT, 
    TOKEN_STRING 
}; 
 
struct token 
{ 
    int type; 
	const char *stored_string;
}; 
 
struct lexer_result 
{ 
	char string_storage[CONSOLE_MAX_STR_LENGTH+1];
	char *next_string;

    struct token tokens[MAX_TOKENS]; 
    unsigned int num_tokens; 
}; 

int lex(const char *line, struct lexer_result *result);

int extract_result_string(struct lexer_result *result, int index, const char **str);
int extract_result_int(struct lexer_result *result, int index, int *i);
int extract_result_float(struct lexer_result *result, int index, float *f);

typedef void (*console_callback)(struct lexer_result *result, void *user_data);

typedef struct COMMAND
{
	const char *name;
	const char *params;
	console_callback callback;
	void *user_data;
	struct COMMAND *next;
	
} COMMAND;

void console_init();
void console_register(COMMAND *cmd);
void console_execute(const char *str);
void console_print(const char *str);
void console_register_print_callback(void (*callback)(const char *));

#define MACRO_REGISTER_COMMAND(name, params, func, ptr) { static COMMAND cmd = { name, params, func, ptr, 0x0 }; console_register(&cmd); }

#ifdef __cplusplus
}
#endif

#endif
