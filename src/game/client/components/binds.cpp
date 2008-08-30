#include <stdlib.h> // atoi
#include <string.h> // strcmp
#include <engine/e_client_interface.h>
#include "binds.hpp"

bool BINDS::BINDS_SPECIAL::on_input(INPUT_EVENT e)
{
	// don't handle invalid events and keys that arn't set to anything
	if(e.key >= KEY_F1 && e.key <= KEY_F25 && binds->keybindings[e.key][0] != 0)
	{
		int stroke = 0;
		if(e.flags&INPFLAG_PRESS)
			stroke = 1;
		console_execute_line_stroked(stroke, binds->keybindings[e.key]);
		return true;
	}
	
	return false;
}

BINDS::BINDS()
{
	mem_zero(keybindings, sizeof(keybindings));
	special_binds.binds = this;
}

void BINDS::bind(int keyid, const char *str)
{
	if(keyid < 0 && keyid >= KEY_LAST)
		return;
		
	str_copy(keybindings[keyid], str, sizeof(keybindings[keyid]));
	if(!keybindings[keyid][0])
		dbg_msg("binds", "unbound %s (%d)", inp_key_name(keyid), keyid);
	else
		dbg_msg("binds", "bound %s (%d) = %s", inp_key_name(keyid), keyid, keybindings[keyid]);
}


bool BINDS::on_input(INPUT_EVENT e)
{
	// don't handle invalid events and keys that arn't set to anything
	if(e.key <= 0 || e.key >= KEY_LAST || keybindings[e.key][0] == 0)
		return false;

	int stroke = 0;
	if(e.flags&INPFLAG_PRESS)
		stroke = 1;
	console_execute_line_stroked(stroke, keybindings[e.key]);
	return true;
}

void BINDS::unbindall()
{
	for(int i = 0; i < KEY_LAST; i++)
		keybindings[i][0] = 0;
}

const char *BINDS::get(int keyid)
{
	if(keyid > 0 && keyid < KEY_LAST)
		return keybindings[keyid];
	return "";
}

void BINDS::set_defaults()
{
	// set default key bindings
	unbindall();
	bind(KEY_F1, "toggle_local_console");
	bind(KEY_F2, "toggle_remote_console");
	bind(KEY_TAB, "+scoreboard");
	bind(KEY_F10, "screenshot");
	
	bind('A', "+left");
	bind('D', "+right");
	bind(KEY_SPACE, "+jump");
	bind(KEY_MOUSE_1, "+fire");
	bind(KEY_MOUSE_2, "+hook");
	bind(KEY_LSHIFT, "+emote");

	bind('1', "+weapon1");
	bind('2', "+weapon2");
	bind('3', "+weapon3");
	bind('4', "+weapon4");
	bind('5', "+weapon5");
	
	bind(KEY_MOUSE_WHEEL_UP, "+prevweapon");
	bind(KEY_MOUSE_WHEEL_DOWN, "+nextweapon");
	
	bind('T', "chat all");
	bind('Y', "chat team");	
}

void BINDS::on_init()
{
	// bindings
	MACRO_REGISTER_COMMAND("bind", "sr", con_bind, this);
	MACRO_REGISTER_COMMAND("unbind", "s", con_unbind, this);
	MACRO_REGISTER_COMMAND("unbindall", "", con_unbindall, this);
	MACRO_REGISTER_COMMAND("dump_binds", "", con_dump_binds, this);
	
	// default bindings
	set_defaults();
}



void BINDS::con_bind(void *result, void *user_data)
{
	BINDS *binds = (BINDS *)user_data;
	const char *key_name = console_arg_string(result, 0);
	int id = binds->get_key_id(key_name);
	
	if(!id)
	{
		dbg_msg("binds", "key %s not found", key_name);
		return;
	}
	
	binds->bind(id, console_arg_string(result, 1));
}


void BINDS::con_unbind(void *result, void *user_data)
{
	BINDS *binds = (BINDS *)user_data;
	const char *key_name = console_arg_string(result, 0);
	int id = binds->get_key_id(key_name);
	
	if(!id)
	{
		dbg_msg("binds", "key %s not found", key_name);
		return;
	}
	
	binds->bind(id, "");
}


void BINDS::con_unbindall(void *result, void *user_data)
{
	BINDS *binds = (BINDS *)user_data;
	binds->unbindall();
}


void BINDS::con_dump_binds(void *result, void *user_data)
{
	BINDS *binds = (BINDS *)user_data;
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(binds->keybindings[i][0] == 0)
			continue;
		dbg_msg("binds", "%s (%d) = %s", inp_key_name(i), i, binds->keybindings[i]);
	}
}

int BINDS::get_key_id(const char *key_name)
{
	// check for numeric
	if(key_name[0] == '#')
	{
		int i = atoi(key_name+1);
		if(i > 0 && i < KEY_LAST)
			return i; // numeric
	}
		
	// search for key
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(strcmp(key_name, inp_key_name(i)) == 0)
			return i;
	}
	
	return 0;
}

/*
void binds_save()
{
	char buffer[256];
	char *end = buffer+sizeof(buffer)-8;
	client_save_line("unbindall");
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(keybindings[i][0] == 0)
			continue;
		str_format(buffer, sizeof(buffer), "bind %s ", inp_key_name(i));
		
		// process the string. we need to escape some characters
		const char *src = keybindings[i];
		char *dst = buffer + strlen(buffer);
		*dst++ = '"';
		while(*src && dst < end)
		{
			if(*src == '"' || *src == '\\') // escape \ and "
				*dst++ = '\\';
			*dst++ = *src++;
		}
		*dst++ = '"';
		*dst++ = 0;
		
		client_save_line(buffer);
	}
}

*/
