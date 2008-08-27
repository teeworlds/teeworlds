#include <game/client/component.hpp>

class EFFECTS : public COMPONENT
{	
	bool add_50hz;
	bool add_100hz;
public:
	EFFECTS();

	virtual void on_render();

	void bullettrail(vec2 pos);
	void smoketrail(vec2 pos, vec2 vel);
	void skidtrail(vec2 pos, vec2 vel);
	void explosion(vec2 pos);
	void air_jump(vec2 pos);
	void damage_indicator(vec2 pos, vec2 dir);
	void playerspawn(vec2 pos);
	void playerdeath(vec2 pos, int cid);
	void powerupshine(vec2 pos, vec2 size);

	void update();
};
