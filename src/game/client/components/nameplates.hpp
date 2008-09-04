#include <game/client/component.hpp>

class NAMEPLATES : public COMPONENT
{
	void render_nameplate(
		const class NETOBJ_CHARACTER *prev_char,
		const class NETOBJ_CHARACTER *player_char,
		const class NETOBJ_PLAYER_INFO *player_info
	);

public:
	virtual void on_render();
};

