#include <game/client/component.hpp>
#include <game/client/lineinput.hpp>

class CHAT : public COMPONENT
{
public:
	LINEINPUT input;
	
	enum 
	{
		MAX_LINES = 10,
	};

	struct LINE
	{
		int tick;
		int client_id;
		int team;
		int name_color;
		char name[64];
		char text[512];
	};

	LINE lines[MAX_LINES];
	int current_line;

	// chat
	enum
	{
		MODE_NONE=0,
		MODE_ALL,
		MODE_TEAM,
	};

	int mode;
	
	void add_line(int client_id, int team, const char *line);
	//void chat_reset();
	//bool chat_input_handle(INPUT_EVENT e, void *user_data);
	
	void enable_mode(int team);
	
	void say(int team, const char *line);
	
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);
};
