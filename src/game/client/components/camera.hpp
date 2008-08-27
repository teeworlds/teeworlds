#include <base/vmath.hpp>
#include <game/client/component.hpp>

class CAMERA : public COMPONENT
{	
public:
	vec2 center;
	float zoom;

	CAMERA();
	virtual void on_render();
};

