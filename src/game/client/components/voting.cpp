#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include "voting.hpp"

void VOTING::con_callvote(void *result, void *user_data)
{
}

void VOTING::con_vote(void *result, void *user_data)
{
}
	
VOTING::VOTING()
{
	on_reset();
}

void VOTING::on_reset()
{
	closetime = 0;
	description[0] = 0;
	command[0] = 0;
	yes = no = pass = total = 0;
	voted = 0;
}

void VOTING::on_console_init()
{
	MACRO_REGISTER_COMMAND("callvote", "r", con_callvote, this);
	MACRO_REGISTER_COMMAND("vote", "r", con_vote, this);
}

void VOTING::on_message(int msgtype, void *rawmsg)
{
	if(msgtype == NETMSGTYPE_SV_VOTE_SET)
	{
		NETMSG_SV_VOTE_SET *msg = (NETMSG_SV_VOTE_SET *)rawmsg;
		if(msg->timeout)
		{
			on_reset();
			str_copy(description, msg->description, sizeof(description));
			str_copy(command, msg->command, sizeof(description));
			closetime = time_get() + time_freq() * msg->timeout;
		}
		else
			on_reset();
	}
	else if(msgtype == NETMSGTYPE_SV_VOTE_STATUS)
	{
		NETMSG_SV_VOTE_STATUS *msg = (NETMSG_SV_VOTE_STATUS *)rawmsg;
		yes = msg->yes;
		no = msg->no;
		pass = msg->pass;
		total = msg->total;
	}	
}

void VOTING::on_render()
{
}

