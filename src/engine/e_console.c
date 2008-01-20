#include "e_console.h"
#include "e_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum 
{ 
    STATE_START, 
    STATE_INT, 
    STATE_FLOAT, 
    STATE_POT_FLOAT, 
    STATE_STRING, 
    STATE_QUOTED, 
    STATE_ESCAPE 
}; 

static const char *store_string(struct lexer_result *res, const char *str, int len)
{
	const char *ptr = res->next_string;
	int escaped = 0;

	while (len)
	{
		if (!escaped && *str == '\\')
		{
			escaped = 1;
		}
		else
		{
			escaped = 0;

			*res->next_string++ = *str;
		}

		str++;
		len--;
	}

	*res->next_string++ = 0;

	/*
	memcpy(res->next_string, str, len);
	res->next_string[len] = 0;
	res->next_string += len+1;
	*/

	return ptr;
}

static void save_token(struct lexer_result *res, int *index, const char **start, const char *end, int *state, int type) 
{ 
    /* printf("Saving token with length %d\n", end - *start); */
    struct token *tok = &res->tokens[*index]; 
	tok->stored_string = store_string(res, *start, end - *start);
    tok->type = type; 
    ++res->num_tokens; 
 
    *start = end + 1; 
    *state = STATE_START; 
    ++*index; 
} 
 
static int digit(char c) 
{ 
    return '0' <= c && c <= '9'; 
} 
 
static int lex(const char *line, struct lexer_result *res) 
{ 
    int state = STATE_START, i = 0; 
	int length_left = CONSOLE_MAX_STR_LENGTH;
    const char *start, *c; 
    res->num_tokens = 0; 

	memset(res, 0, sizeof(*res));
	res->next_string = res->string_storage;

    for (c = start = line; *c != '\0' && res->num_tokens < MAX_TOKENS && length_left; ++c, --length_left) 
    { 
        /* printf("State: %d.. c: %c\n", state, *c); */
        switch (state) 
        { 
        case STATE_START: 
            if (*c == ' ') 
                start = c + 1; 
            else if (digit(*c)) 
                state = STATE_INT; 
            else if (*c == '-') 
                state = STATE_INT; 
            else if (*c == '.') 
                state = STATE_POT_FLOAT; 
            else if (*c == '"')
				state = STATE_QUOTED;
			else
                state = STATE_STRING; 
            break; 
 
        case STATE_INT: 
            if (digit(*c)) 
                ; 
            else if (*c == '.') 
                state = STATE_FLOAT; 
            else if (*c == ' ') 
                save_token(res, &i, &start, c, &state, TOKEN_INT); 
            else 
                state = STATE_STRING; 
            break; 
 
        case STATE_FLOAT: 
            if (digit(*c)) 
                ; 
            else if (*c == ' ') 
                save_token(res, &i, &start, c, &state, TOKEN_FLOAT); 
            else 
                state = STATE_STRING; 
            break; 
 
        case STATE_POT_FLOAT: 
            if (digit(*c)) 
                state = STATE_FLOAT; 
            else if (*c == ' ') 
                save_token(res, &i, &start, c, &state, TOKEN_STRING); 
            else 
                state = STATE_STRING; 
            break; 
 
        case STATE_STRING: 
            if (*c == ' ') 
                save_token(res, &i, &start, c, &state, TOKEN_STRING); 
            break; 
 
        case STATE_QUOTED: 
            if (*c == '"') 
			{
				++start;
                save_token(res, &i, &start, c, &state, TOKEN_STRING); 
			}
            else if (*c == '\\') 
                state = STATE_ESCAPE; 
            break; 
 
        case STATE_ESCAPE: 
            if (*c != ' ') 
                state = STATE_QUOTED; 
            break; 
        } 
    } 
 
    switch (state) 
    { 
    case STATE_INT: 
        save_token(res, &i, &start, c, &state, TOKEN_INT); 
        break; 
    case STATE_FLOAT: 
        save_token(res, &i, &start, c, &state, TOKEN_FLOAT); 
        break; 
    case STATE_STRING: 
    case STATE_QUOTED: 
    case STATE_POT_FLOAT: 
        save_token(res, &i, &start, c, &state, TOKEN_STRING); 
        break; 
    case STATE_ESCAPE: 
        puts("LOL MALFORMED"); 
        break; 
    default: 
        break; 
    } 

	return 0;
}

int extract_result_string(struct lexer_result *result, int index, const char **str)
{
	if (index < 0 || index >= result->num_tokens)
		return -1;
	else
	{
		struct token *t = &result->tokens[index];

		*str = t->stored_string;

		return 0;
	}
}

int extract_result_int(struct lexer_result *result, int index, int *i)
{
	if (index < 0 || index >= result->num_tokens)
		return -1;
	else
	{
		struct token *t = &result->tokens[index];
		const char *str;

		if (t->type != TOKEN_INT)
			return -2;

		extract_result_string(result, index, &str);

		*i = atoi(str);

		return 0;
	}
}

int extract_result_float(struct lexer_result *result, int index, float *f)
{
	if (index < 0 || index >= result->num_tokens)
		return -1;
	else
	{
		struct token *t = &result->tokens[index];
		const char *str;

		if (t->type != TOKEN_INT && t->type != TOKEN_FLOAT)
			return -2;

		extract_result_string(result, index, &str);

		*f = atof(str);

		return 0;
	}
}

static COMMAND *first_command = 0x0;

COMMAND *console_find_command(const char *name)
{
	COMMAND *cmd;
	for (cmd = first_command; cmd; cmd = cmd->next)
		if (strcmp(cmd->name, name) == 0)
			return cmd;

	return 0x0;
}

void console_register(COMMAND *cmd)
{
	cmd->next = first_command;
	first_command = cmd;
}


static int console_validate(COMMAND *command, struct lexer_result *result)
{
	const char *c = command->params;
	int i = 1;

	const char *dummy_s;
	int dummy_i;
	float dummy_f;

	while (*c && *c != '?')
	{
		switch (*c)
		{
		case 's':
			if (extract_result_string(result, i, &dummy_s))
				return -1;
			break;
		case 'i':
			if (extract_result_int(result, i, &dummy_i))
				return -1;
			break;
		case 'f':
			if (extract_result_float(result, i, &dummy_f))
				return -1;
			break;
		default:
			/* unknown char, so just continue... */
			c++;
			continue;
		}

		i++;
		c++;
	}
	
	return 0;
}

static void (*print_callback)(const char *) = 0x0;

void console_register_print_callback(void (*callback)(const char *))
{
	print_callback = callback;
}

void console_print(const char *str)
{
	if (print_callback)
		print_callback(str);
}

void console_execute(const char *str)
{
	struct lexer_result result;
	int error;

	if ((error = lex(str, &result)))
		printf("ERROR: %d\n", error);
	else if (result.num_tokens > 0)
	{
		const char *name;
		COMMAND *command;
		extract_result_string(&result, 0, &name);

		command = console_find_command(name);

		if (command)
		{
			if (console_validate(command, &result))
			{
				char buf[256];
				sprintf(buf, "Invalid arguments... Usage: %s %s", command->name, command->params);
				console_print(buf);
			}
			else
				command->callback(&result, command->user_data);
		}
		else
		{
			char buf[256];
			sprintf(buf, "No such command: %s.", name);
			console_print(buf);
		}
	}
}

static void echo_command(struct lexer_result *result, void *user_data)
{
	const char *str;
	extract_result_string(result, 1, &str);

	console_print(str);
}


struct int_variable_data
{
	config_int_getter getter;
	config_int_setter setter;
};

struct str_variable_data
{
	config_str_getter getter;
	config_str_setter setter;
};

static void int_variable_command(struct lexer_result *result, void *user_data)
{
	struct int_variable_data *data = (struct int_variable_data *)user_data;
	int new_val;

	if (extract_result_int(result, 1, &new_val))
	{
		char buf[256];
		sprintf(buf, "Value: %d", data->getter(&config));
		console_print(buf);
	}
	else
	{
		data->setter(&config, new_val);
	}
}

static void str_variable_command(struct lexer_result *result, void *user_data)
{
	struct str_variable_data *data = (struct str_variable_data *)user_data;
	const char *new_val;

	if (extract_result_string(result, 1, &new_val))
	{
		char buf[256];
		sprintf(buf, "Value: %s", data->getter(&config));
		console_print(buf);
	}
	else
	{
		data->setter(&config, new_val);
	}
}

void console_init()
{
	MACRO_REGISTER_COMMAND("echo", "s", echo_command, 0x0);

    #define MACRO_CONFIG_INT(name,def,min,max) { static struct int_variable_data data = { &config_get_ ## name, &config_set_ ## name }; MACRO_REGISTER_COMMAND(#name, "?i", int_variable_command, &data) }
    #define MACRO_CONFIG_STR(name,len,def) { static struct str_variable_data data = { &config_get_ ## name, &config_set_ ## name }; MACRO_REGISTER_COMMAND(#name, "?s", str_variable_command, &data) }

    #include "e_config_variables.h" 

    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
}
