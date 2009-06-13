#ifndef GAME_CLIENT_LINEINPUT_H
#define GAME_CLIENT_LINEINPUT_H

// line input helter
class LINEINPUT
{
	char str[256];
	int len;
	int cursor_pos;
public:
	static void manipulate(INPUT_EVENT e, char *str, int str_max_size, int *str_len, int *cursor_pos);

	class CALLBACK
	{
	public:
		virtual ~CALLBACK() {}
		virtual bool event(INPUT_EVENT e) = 0;
	};

	LINEINPUT();
	void clear();
	void process_input(INPUT_EVENT e);
	void set(const char *string);
	const char *get_string() const { return str; }
	int get_length() const { return len; }
	unsigned cursor_offset() const { return cursor_pos; }
};

#endif
