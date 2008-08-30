#include <game/client/component.hpp>

class MOTD : public COMPONENT
{
	// motd
	int64 server_motd_time;
public:
	char server_motd[900];

	void clear();
	bool is_active();
	
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);
};

