#include <base/vmath.hpp>
#include <game/client/component.hpp>

class EMOTICON : public COMPONENT
{
	void draw_circle(float x, float y, float r, int segments);
	
	bool was_active;
	bool active;
	
	vec2 selector_mouse;
	int selected_emote;

	static void con_key_emoticon(void *result, void *user_data);
	
public:
	EMOTICON();
	
	virtual void on_reset();
	virtual void on_init();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_mousemove(float x, float y);

	void emote(int emoticon);
};

