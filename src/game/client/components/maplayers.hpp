#include <game/client/component.hpp>

class MAPLAYERS : public COMPONENT
{	
	int type;

	void mapscreen_to_group(float center_x, float center_y, MAPITEM_GROUP *group);
	static void envelope_eval(float time_offset, int env, float *channels, void *user);
public:
	enum
	{
		TYPE_BACKGROUND=0,
		TYPE_FOREGROUND,
	};

	MAPLAYERS(int type);
	virtual void on_render();
};

