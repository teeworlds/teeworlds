#include <engine/e_client_interface.h>
#include <game/client/component.hpp>
#include <game/client/lineinput.hpp>

class CONSOLE : public COMPONENT
{
	class INSTANCE
	{
	public:
		char history_data[65536];
		struct RINGBUFFER *history;
		char *history_entry;
		
		char backlog_data[65536];
		struct RINGBUFFER *backlog;

		LINEINPUT input;
		
		int type;
		int completion_enumeration_count;
		
	public:
		char completion_buffer[128];
		int completion_chosen;
		int completion_flagmask;
		
		COMMAND *command;

		INSTANCE(int t);

		void execute_line(const char *line);
		
		void on_input(INPUT_EVENT e);
		void print_line(const char *line);
		
		const char *get_string() const { return input.get_string(); }

		static void possible_commands_complete_callback(const char *str, void *user);
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

	static void possible_commands_render_callback(const char *str, void *user);
	static void client_console_print_callback(const char *str, void *user_data);
	static void con_toggle_local_console(void *result, void *user_data);
	static void con_toggle_remote_console(void *result, void *user_data);
	
public:
	CONSOLE();

	void print_line(int type, const char *line);

	virtual void on_console_init();
	virtual void on_reset();
	virtual void on_render();
	virtual void on_message(int msgtype, void *rawmsg);
	virtual bool on_input(INPUT_EVENT e);
};
