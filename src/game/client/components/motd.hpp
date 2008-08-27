#include <game/client/component.hpp>

class MOTD : public COMPONENT
{
public:
	// motd
	int64 server_motd_time;
	char server_motd[900]; // FUGLY
	
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);
};

