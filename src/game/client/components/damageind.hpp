#include <base/vmath.hpp>
#include <game/client/component.hpp>

class DAMAGEIND : public COMPONENT
{
	int64 lastupdate;
	struct ITEM
	{
		vec2 pos;
		vec2 dir;
		float life;
		float startangle;
	};

	enum
	{
		MAX_ITEMS=64,
	};

	ITEM items[MAX_ITEMS];
	int num_items;

	ITEM *create_i();
	void destroy_i(ITEM *i);

public:	
	DAMAGEIND();

	void create(vec2 pos, vec2 dir);
	virtual void on_render();
};
