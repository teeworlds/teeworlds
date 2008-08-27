
// line input helter
class LINEINPUT
{
	char str[256];
	unsigned len;
	unsigned cursor_pos;
public:
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
