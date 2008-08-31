#include <base/vmath.hpp>
#include <game/client/component.hpp>

class CONTROLS : public COMPONENT
{	
public:
	vec2 mouse_pos;
	vec2 target_pos;

	NETOBJ_PLAYER_INPUT input_data;
	int input_direction_left;
	int input_direction_right;

	CONTROLS();
	virtual void on_message(int msg, void *rawmsg);
	virtual bool on_mousemove(float x, float y);
	virtual void on_init();
	
	int snapinput(int *data);
};
