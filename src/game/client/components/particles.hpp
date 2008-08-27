#include <base/vmath.hpp>
#include <game/client/component.hpp>

// particles
struct PARTICLE
{
	void set_default()
	{
		vel = vec2(0,0);
		life_span = 0;
		start_size = 32;
		end_size = 32;
		rot = 0;
		rotspeed = 0;
		gravity = 0;
		friction = 0;
		flow_affected = 1.0f;
		color = vec4(1,1,1,1);
	}
	
	vec2 pos;
	vec2 vel;

	int spr;

	float flow_affected;

	float life_span;
	
	float start_size;
	float end_size;

	float rot;
	float rotspeed;

	float gravity;
	float friction;

	vec4 color;
	
	// set by the particle system
	float life;
	int prev_part;
	int next_part;
};

class PARTICLES : public COMPONENT
{
	friend class GAMECLIENT;
public:
	enum
	{
		GROUP_PROJECTILE_TRAIL=0,
		GROUP_EXPLOSIONS,
		GROUP_GENERAL,
		NUM_GROUPS
	};

	PARTICLES();
	
	void add(int group, PARTICLE *part);
	
	virtual void on_reset();
	virtual void on_render();

private:
	
	enum
	{
		MAX_PARTICLES=1024*8,
	};

	PARTICLE particles[MAX_PARTICLES];
	int first_free;
	int first_part[NUM_GROUPS];
	
	void render_group(int group);
	void update(float time_passed);

	template<int TGROUP>
	class RENDER_GROUP : public COMPONENT
	{
	public:
		PARTICLES *parts;
		virtual void on_render() { parts->render_group(TGROUP); }
	};
	
	RENDER_GROUP<GROUP_PROJECTILE_TRAIL> render_trail;
	RENDER_GROUP<GROUP_EXPLOSIONS> render_explosions;
	RENDER_GROUP<GROUP_GENERAL> render_general;
};
