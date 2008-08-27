#include <game/client/component.hpp>

class ITEMS : public COMPONENT
{	
	void render_projectile(const NETOBJ_PROJECTILE *current, int itemid);
	void render_pickup(const NETOBJ_PICKUP *prev, const NETOBJ_PICKUP *current);
	void render_flag(const NETOBJ_FLAG *prev, const NETOBJ_FLAG *current);
	void render_laser(const struct NETOBJ_LASER *current);
	
public:
	virtual void on_render();
};

