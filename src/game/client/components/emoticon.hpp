#include <base/vmath.hpp>
#include <game/client/component.hpp>

class EMOTICON : public COMPONENT
{
	void draw_circle(float x, float y, float r, int segments);
	
	vec2 selector_mouse;
	int selector_active;
	int selected_emote;
	
public:
	EMOTICON();
	
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);

	void emote(int emoticon);
};

