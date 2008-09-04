#include <game/client/component.hpp>

class SCOREBOARD : public COMPONENT
{
	void render_goals(float x, float y, float w);
	void render_spectators(float x, float y, float w);
	void render_scoreboard(float x, float y, float w, int team, const char *title);

	static void con_key_scoreboard(void *result, void *user_data);
	
	bool active;
	
public:
	SCOREBOARD();
	virtual void on_reset();
	virtual void on_console_init();
	virtual void on_render();
};

