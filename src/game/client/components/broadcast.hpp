#include <game/client/component.hpp>

class BROADCAST : public COMPONENT
{
public:
	// broadcasts
	char broadcast_text[1024];
	int64 broadcast_time;
	
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
};

