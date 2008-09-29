#include <game/client/component.hpp>

class MAPLIST : public COMPONENT
{
	char buffer[2048];
	const char *maps[128];
	int num_maps;
public:
	MAPLIST();
	virtual void on_reset();
	virtual void on_message(int msgtype, void *rawmsg);
	
	int num() const { return num_maps; }
	const char *name(int index) const { return maps[index]; }
};
