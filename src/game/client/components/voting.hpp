#include <game/client/component.hpp>
#include <game/client/ui.hpp>

class VOTING : public COMPONENT
{
	/*
	void render_goals(float x, float y, float w);
	void render_spectators(float x, float y, float w);
	void render_scoreboard(float x, float y, float w, int team, const char *title);

	static void con_key_scoreboard(void *result, void *user_data);
	
	bool active;
	*/

	static void con_callvote(void *result, void *user_data);
	static void con_vote(void *result, void *user_data);
	
	int64 closetime;
	char description[512];
	char command[512];
	int voted;
	
	void callvote(const char *type, const char *value);
	
public:
	VOTING();
	virtual void on_reset();
	virtual void on_console_init();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual void on_render();
	
	void render_bars(RECT bars, bool text);
	
	void callvote_kick(int client_id);
	void callvote_map(const char *map);
	
	void vote(int v); // -1 = no, 1 = yes
	
	int seconds_left() { return (closetime - time_get())/time_freq(); }
	bool is_voting() { return closetime != 0; }
	int taken_choice() const { return voted; }
	const char *vote_description() const { return description; }
	const char *vote_command() const { return command; }
	
	int yes, no, pass, total;
};

