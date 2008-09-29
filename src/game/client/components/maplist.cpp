#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>

#include "maplist.hpp"

MAPLIST::MAPLIST()
{
	on_reset();
}

void MAPLIST::on_reset()
{
	buffer[0] = 0;
	num_maps = 0;
}

static bool is_separator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void MAPLIST::on_message(int msgtype, void *rawmsg)
{
	if(msgtype == NETMSGTYPE_SV_MAPLIST)
	{
		NETMSG_SV_MAPLIST *msg = (NETMSG_SV_MAPLIST*)rawmsg;
		str_copy(buffer, msg->names, sizeof(buffer));
		
		// parse list
		num_maps = 0;
		char *ptr = buffer;
		while(*ptr)
		{
			while(*ptr && is_separator(*ptr))
			{
				*ptr = 0;
				ptr++;
			}

			if(*ptr)
			{
				maps[num_maps++] = ptr;
				while(*ptr && !is_separator(*ptr))
					ptr++;
			}
		}
	}
}
