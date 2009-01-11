#include <game/client/component.hpp>

class SOUNDS : public COMPONENT
{
public:
	// sound channels
	enum
	{
		CHN_GUI=0,
		CHN_MUSIC,
		CHN_WORLD,
		CHN_GLOBAL,
	};

	virtual void on_init();
	virtual void on_render();
	
	void play(int chn, int setid, float vol, vec2 pos);
	void play_and_record(int chn, int setid, float vol, vec2 pos);
};


