#include <base/system.h>
#include "e_if_other.h"
#include "e_console.h"
#include "e_config.h"
#include "e_engine.h"
#include "e_linereader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define CONSOLE_MAX_STR_LENGTH 1024
/* the maximum number of tokens occurs in a string of length CONSOLE_MAX_STR_LENGTH with tokens size 1 separated by single spaces */
#define MAX_PARTS (CONSOLE_MAX_STR_LENGTH+1)/2

typedef struct
{ 
	char string_storage[CONSOLE_MAX_STR_LENGTH+1];
	char *args_start;
	
	const char *command;
	const char *args[MAX_PARTS]; 
	unsigned int num_args; 
} PARSE_RESULT;

static char *str_skipblanks(char *str)
{
	while(*str && (*str == ' ' || *str == '\t' || *str == '\n'))
		str++;
	return str;
}

static char *str_skiptoblank(char *str)
{
	while(*str && (*str != ' ' && *str != '\t' && *str != '\n'))
		str++;
	return str;
}

/* static int digit(char c) { return '0' <= c && c <= '9'; } */

static int console_parse_start(PARSE_RESULT *result, const char *string, int length)
{
	char *str;
	int len = sizeof(result->string_storage);
	if(length < len)
		len = length;
		
	str_copy(result->string_storage, string, length);
	str = result->string_storage;
	
	/* get command */
	str = str_skipblanks(str);
	result->command = str;
	str = str_skiptoblank(str);
	
	if(*str)
	{
		str[0] = 0;
		str++;
	}
	
	result->args_start = str;
	result->num_args = 0;
	return 0;
}

static int console_parse_args(PARSE_RESULT *result, const char *format)
{
	char command;
	char *str;
	int optional = 0;
	int error = 0;
	
	str = result->args_start;

	while(1)	
	{
		/* fetch command */
		command = *format;
		format++;
		
		if(!command)
			break;
		
		if(command == '?')
			optional = 1;
		else
		{
			str = str_skipblanks(str);
		
			if(!(*str)) /* error, non optional command needs value */
			{
				if(!optional)
					error = 1;
				break;
			}
			
			/* add token */
			if(*str == '"')
			{
				char *dst;
				str++;
				result->args[result->num_args++] = str;
				
				dst = str; /* we might have to process escape data */
				while(1)
				{
					if(str[0] == '"')
						break;
					else if(str[0] == '\\')
					{
						if(str[1] == '\\')
							str++; /* skip due to escape */
						else if(str[1] == '"')
							str++; /* skip due to escape */
					}
					else if(str[0] == 0)
						return 1; /* return error */
						
					*dst = *str;
					dst++;
					str++;
				}
				
				/* write null termination */
				*dst = 0;
			}
			else
			{
				result->args[result->num_args++] = str;
				
				if(command == 'r') /* rest of the string */
					break;
				else if(command == 'i') /* validate int */
					str = str_skiptoblank(str);
				else if(command == 'f') /* validate float */
					str = str_skiptoblank(str);
				else if(command == 's') /* validate string */
					str = str_skiptoblank(str);

				if(str[0] != 0) /* check for end of string */
				{
					str[0] = 0;
					str++;
				}
			}
		}
	}

	return error;
}

const char *console_arg_string(void *res, int index)
{
	PARSE_RESULT *result = (PARSE_RESULT *)res;
	if (index < 0 || index >= result->num_args)
		return "";
	return result->args[index];
}

int console_arg_int(void *res, int index)
{
	PARSE_RESULT *result = (PARSE_RESULT *)res;
	if (index < 0 || index >= result->num_args)
		return 0;
	return atoi(result->args[index]);
}

float console_arg_float(void *res, int index)
{
	PARSE_RESULT *result = (PARSE_RESULT *)res;
	if (index < 0 || index >= result->num_args)
		return 0.0f;
	return atof(result->args[index]);
}

int console_arg_num(void *result)
{
	return ((PARSE_RESULT *)result)->num_args;
}

static COMMAND *first_command = 0x0;

COMMAND *console_find_command(const char *name)
{
	COMMAND *cmd;
	for (cmd = first_command; cmd; cmd = cmd->next)
	{
		if (strcmp(cmd->name, name) == 0)
			return cmd;
	}

	return 0x0;
}

void console_register(COMMAND *cmd)
{
	cmd->next = first_command;
	first_command = cmd;
}

static void (*print_callback)(const char *, void *) = 0x0;
static void *print_callback_userdata;

void console_register_print_callback(void (*callback)(const char *, void *), void *user_data)
{
	print_callback = callback;
	print_callback_userdata = user_data;
}

void console_print(const char *str)
{
	if (print_callback)
		print_callback(str, print_callback_userdata);
}

void console_execute_line_stroked(int stroke, const char *str)
{
	PARSE_RESULT result;
	COMMAND *command;
	
	char strokestr[2] = {'0', 0};
	if(stroke)
		strokestr[0] = '1';

	while(str)
	{
		const char *end = str;
		const char *next_part = 0;
		int in_string = 0;
		
		while(*end)
		{
			if(*end == '"')
				in_string ^= 1;
			else if(*end == '\\') /* escape sequences */
			{
				if(end[1] == '"')
					end++;
			}
			else if(!in_string)
			{
				if(*end == ';')  /* command separator */
				{
					next_part = end+1;
					break;
				}
				else if(*end == '#')  /* comment, no need to do anything more */
					break;
			}
			
			end++;
		}
		
		if(console_parse_start(&result, str, (end-str) + 1) != 0)
			return;

		command = console_find_command(result.command);

		if(command)
		{
			int is_stroke_command = 0;
			if(result.command[0] == '+')
			{
				/* insert the stroke direction token */
				result.args[result.num_args] = strokestr;
				result.num_args++;
				is_stroke_command = 1;
			}
			
			if(stroke || is_stroke_command)
			{
				if(console_parse_args(&result, command->params))
				{
					char buf[256];
					str_format(buf, sizeof(buf), "Invalid arguments... Usage: %s %s", command->name, command->params);
					console_print(buf);
				}
				else
					command->callback(&result, command->user_data);
			}
		}
		else
		{
			char buf[256];
			str_format(buf, sizeof(buf), "No such command: %s.", result.command);
			console_print(buf);
		}
		
		str = next_part;
	}
}

void console_execute_line(const char *str)
{
	console_execute_line_stroked(1, str);
}

static void console_execute_file_real(const char *filename)
{
	IOHANDLE file;
	file = engine_openfile(filename, IOFLAG_READ);
	
	if(file)
	{
		char *line;
		LINEREADER lr;
		
		dbg_msg("console", "executing '%s'", filename);
		linereader_init(&lr, file);

		while((line = linereader_get(&lr)))
			console_execute_line(line);

		io_close(file);
	}
	else
		dbg_msg("console", "failed to open '%s'", filename);
}

struct exec_file
{
	const char *filename;
	struct exec_file *next;
};

void console_execute_file(const char *filename)
{
	static struct exec_file *first = 0;
	struct exec_file this;
	struct exec_file *cur;
	struct exec_file *prev;

	/* make sure that this isn't being executed already */	
	for(cur = first; cur; cur = cur->next)
		if(strcmp(filename, cur->filename) == 0)
			return;
	
	/* push this one to the stack */
	prev = first;
	this.filename = filename;
	this.next = first;
	first = &this;
	
	/* execute file */
	console_execute_file_real(filename);
	
	/* pop this one from the stack */
	first = prev;
}

static void con_echo(void *result, void *user_data)
{
	console_print(console_arg_string(result, 0));
}

static void con_exec(void *result, void *user_data)
{
	console_execute_file(console_arg_string(result, 0));

}


typedef struct 
{
	CONFIG_INT_GETTER getter;
	CONFIG_INT_SETTER setter;
} INT_VARIABLE_DATA;

typedef struct
{
	CONFIG_STR_GETTER getter;
	CONFIG_STR_SETTER setter;
} STR_VARIABLE_DATA;

static void int_variable_command(void *result, void *user_data)
{
	INT_VARIABLE_DATA *data = (INT_VARIABLE_DATA *)user_data;

	if(console_arg_num(result))
		data->setter(&config, console_arg_int(result, 0));
	else
	{
		char buf[1024];
		str_format(buf, sizeof(buf), "Value: %d", data->getter(&config));
		console_print(buf);
	}
}

static void str_variable_command(void *result, void *user_data)
{
	STR_VARIABLE_DATA *data = (STR_VARIABLE_DATA *)user_data;

	if(console_arg_num(result))
		data->setter(&config, console_arg_string(result, 0));
	else
	{
		char buf[1024];
		str_format(buf, sizeof(buf), "Value: %s", data->getter(&config));
		console_print(buf);
	}
}

void console_init()
{
	MACRO_REGISTER_COMMAND("echo", "r", con_echo, 0x0);
	MACRO_REGISTER_COMMAND("exec", "r", con_exec, 0x0);

	#define MACRO_CONFIG_INT(name,def,min,max,flags,desc) { static INT_VARIABLE_DATA data = { &config_get_ ## name, &config_set_ ## name }; MACRO_REGISTER_COMMAND(#name, "?i", int_variable_command, &data) }
	#define MACRO_CONFIG_STR(name,len,def,flags,desc) { static STR_VARIABLE_DATA data = { &config_get_ ## name, &config_set_ ## name }; MACRO_REGISTER_COMMAND(#name, "?r", str_variable_command, &data) }

	#include "e_config_variables.h" 

	#undef MACRO_CONFIG_INT 
	#undef MACRO_CONFIG_STR 
}
