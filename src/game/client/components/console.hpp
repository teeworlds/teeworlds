extern "C" {
	#include <engine/e_client_interface.h>
	#include <engine/e_config.h>
	#include <engine/e_console.h>
	#include <engine/e_ringbuffer.h>
	#include <engine/client/ec_font.h>
}

#include <game/client/component.hpp>

class CONSOLE : public COMPONENT
{
	class INSTANCE
	{
	public:
		char history_data[65536];
		RINGBUFFER *history;
		char *history_entry;
		
		char backlog_data[65536];
		RINGBUFFER *backlog;

		LINEINPUT input;
		
		int type;
		
	public:
		INSTANCE(int t);

		void execute_line(const char *line);
		
		void on_input(INPUT_EVENT e);
		void print_line(const char *line);
	};
	
	INSTANCE local_console;
	INSTANCE remote_console;
	
	INSTANCE *current_console();
	float time_now();
	
	int console_type;
	int console_state;
	float state_change_end;
	float state_change_duration;


	void toggle(int type);

	static void client_console_print_callback(const char *str, void *user_data);
	static void con_toggle_local_console(void *result, void *user_data);
	static void con_toggle_remote_console(void *result, void *user_data);
	
public:
	CONSOLE();

	virtual void on_init();
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);
};
