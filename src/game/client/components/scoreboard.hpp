#include <game/client/component.hpp>

class SCOREBOARD : public COMPONENT
{
	void render_goals(float x, float y, float w);
	void render_spectators(float x, float y, float w);
	void render_scoreboard(float x, float y, float w, int team, const char *title);
public:
	virtual void on_render();
};

