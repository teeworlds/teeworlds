/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_MSG_H
#define ENGINE_IF_MSG_H

/*
	Section: Messaging
*/

void msg_pack_start_system(int msg, int flags);

/*
	Function: msg_pack_start
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_start(int msg, int flags);

/*
	Function: msg_pack_int
		TODO	
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_int(int i);

/*
	Function: msg_pack_string
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_string(const char *p, int limit);

/*
	Function: msg_pack_raw
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_raw(const void *data, int size);

/*
	Function: msg_pack_end
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_end();

typedef struct
{
	int msg;
	int flags;
	const unsigned char *data;
	int size;
} MSG_INFO;

const MSG_INFO *msg_get_info();

/* message unpacking */
int msg_unpack_start(const void *data, int data_size, int *is_system);

/*
	Function: msg_unpack_int
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int msg_unpack_int();

/*
	Function: msg_unpack_string
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *msg_unpack_string();

/*
	Function: msg_unpack_raw
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const unsigned char *msg_unpack_raw(int size);

/*
	Function: msg_unpack_error
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int msg_unpack_error();


#endif
