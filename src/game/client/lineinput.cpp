#include <engine/e_client_interface.h>
#include <string.h> // strlen
#include "lineinput.hpp"

LINEINPUT::LINEINPUT()
{
	clear();
}

void LINEINPUT::clear()
{
	mem_zero(str, sizeof(str));
	len = 0;
	cursor_pos = 0;
}

void LINEINPUT::set(const char *string)
{
	str_copy(str, string, sizeof(str));
	len = strlen(str);
	cursor_pos = len;
}

void LINEINPUT::manipulate(INPUT_EVENT e, char *str, int str_max_size, int *str_len_ptr, int *cursor_pos_ptr)
{
	int cursor_pos = *cursor_pos_ptr;
	int len = *str_len_ptr;
	
	if(cursor_pos > len)
		cursor_pos = len;
	
	int code = e.unicode;
	int k = e.key;
	
	// 127 is produced on Mac OS X and corresponds to the delete key
	if (!(code >= 0 && code < 32) && code != 127)
	{
		char tmp[8];
		int charsize = str_utf8_encode(tmp, code);
		
		if (len < str_max_size - charsize && cursor_pos < str_max_size - charsize)
		{
			memmove(str + cursor_pos + charsize, str + cursor_pos, len - cursor_pos + charsize);
			for(int i = 0; i < charsize; i++)
				str[cursor_pos+i] = tmp[i];
			cursor_pos += charsize;
			len += charsize;
		}
	}
	
	if(e.flags&INPFLAG_PRESS)
	{
		if (k == KEY_BACKSPACE && cursor_pos > 0)
		{
			int new_cursor_pos = str_utf8_rewind(str, cursor_pos);
			int charsize = cursor_pos-new_cursor_pos;
			memmove(str+new_cursor_pos, str+cursor_pos, len - charsize + 1); // +1 == null term
			cursor_pos = new_cursor_pos;
			len -= charsize;
		}
		else if (k == KEY_DELETE && cursor_pos < len)
		{
			int p = str_utf8_forward(str, cursor_pos);
			int charsize = p-cursor_pos;
			memmove(str + cursor_pos, str + cursor_pos + charsize, len - cursor_pos - charsize + 1); // +1 == null term
			len -= charsize;
		}
		else if (k == KEY_LEFT && cursor_pos > 0)
			cursor_pos = str_utf8_rewind(str, cursor_pos);
		else if (k == KEY_RIGHT && cursor_pos < len)
			cursor_pos = str_utf8_forward(str, cursor_pos);
		else if (k == KEY_HOME)
			cursor_pos = 0;
		else if (k == KEY_END)
			cursor_pos = len;
	}
	
	*cursor_pos_ptr = cursor_pos;
	*str_len_ptr = len;
}

void LINEINPUT::process_input(INPUT_EVENT e)
{
	manipulate(e, str, sizeof(str), &len, &cursor_pos);
}
