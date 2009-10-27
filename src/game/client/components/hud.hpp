#include <game/client/component.hpp>

class HUD : public COMPONENT
{	
	float width;
	
	void render_cursor();
	
	void render_fps();
	void render_connectionwarning();
	void render_teambalancewarning();
	void render_voting();
	void render_healthandammo();
	void render_goals();
	
	
	void mapscreen_to_group(float center_x, float center_y, struct MAPITEM_GROUP *group);
public:
	HUD();
	
	virtual void on_reset();
	virtual void on_render();
};

