#include <game/client/component.hpp>

class MOTD : public COMPONENT
{
	// motd
	int64 server_motd_time;
public:
	char server_motd[900];

	void clear();
	bool is_active();
	
	virtual void on_render();
	virtual void on_statechange(int new_state, int old_state);
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);
};

