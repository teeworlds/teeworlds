#include <game/client/component.hpp>

class PLAYERS : public COMPONENT
{	
	void render_hand(class TEE_RENDER_INFO *info, vec2 center_pos, vec2 dir, float angle_offset, vec2 post_rot_offset);
	void render_player(
		const class NETOBJ_CHARACTER *prev_char,
		const class NETOBJ_CHARACTER *player_char,
		const class NETOBJ_PLAYER_INFO *prev_info,
		const class NETOBJ_PLAYER_INFO *player_info
	);	
	
public:
	virtual void on_render();
};

