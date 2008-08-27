#include <game/client/component.hpp>

class KILLMESSAGES : public COMPONENT
{
public:
	// kill messages
	struct KILLMSG
	{
		int weapon;
		int victim;
		int killer;
		int mode_special; // for CTF, if the guy is carrying a flag for example
		int tick;
	};

	static const int killmsg_max = 5;
	KILLMSG killmsgs[killmsg_max];
	int killmsg_current;

	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
};

