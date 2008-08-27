#include <game/client/component.hpp>

class DEBUGHUD : public COMPONENT
{	
	void render_netcorrections();
	void render_tuning();
public:
	virtual void on_render();
};

