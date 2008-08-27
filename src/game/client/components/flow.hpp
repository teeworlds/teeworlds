#include <base/vmath.hpp>
#include <game/client/component.hpp>

class FLOW : public COMPONENT
{
	struct CELL
	{
		vec2 vel;
	};

	CELL *cells;
	int height;
	int width;
	int spacing;
	
	void dbg_render();
	void init();
public:
	FLOW();
	
	vec2 get(vec2 pos);
	void add(vec2 pos, vec2 vel, float size);
	void update();
};

