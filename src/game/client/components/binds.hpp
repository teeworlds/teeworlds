#include <game/client/component.hpp>

class BINDS : public COMPONENT
{
	char keybindings[KEY_LAST][128];
public:
	BINDS();
	
	void bind(int keyid, const char *str);
	void set_defaults();
	void unbindall();
	const char *get(int keyid);
	
	/*virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);*/
	virtual void on_init();
	virtual bool on_input(INPUT_EVENT e);
};
