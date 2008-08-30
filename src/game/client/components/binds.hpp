#include <game/client/component.hpp>

class BINDS : public COMPONENT
{
	char keybindings[KEY_LAST][128];

	int get_key_id(const char *key_name);

	static void con_bind(void *result, void *user_data);
	static void con_unbind(void *result, void *user_data);
	static void con_unbindall(void *result, void *user_data);
	static void con_dump_binds(void *result, void *user_data);
	
public:
	BINDS();
	
	class BINDS_SPECIAL : public COMPONENT
	{
	public:
		BINDS *binds;
		virtual bool on_input(INPUT_EVENT e);
	};
	
	BINDS_SPECIAL special_binds;
	
	void bind(int keyid, const char *str);
	void set_defaults();
	void unbindall();
	const char *get(int keyid);
	
	virtual void on_init();
	virtual bool on_input(INPUT_EVENT e);
};
