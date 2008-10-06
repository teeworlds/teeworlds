
#include <engine/e_server_interface.h>
#include "entity.hpp"
#include "gamecontext.hpp"

//////////////////////////////////////////////////
// Entity
//////////////////////////////////////////////////
ENTITY::ENTITY(int objtype)
{
	this->objtype = objtype;
	pos = vec2(0,0);
	proximity_radius = 0;

	marked_for_destroy = false;	
	id = snap_new_id();

	next_entity = 0;
	prev_entity = 0;
	prev_type_entity = 0;
	next_type_entity = 0;
}

ENTITY::~ENTITY()
{
	game.world.remove_entity(this);
	snap_free_id(id);
}

int ENTITY::networkclipped(int snapping_client)
{
	return networkclipped(snapping_client, pos);
}

int ENTITY::networkclipped(int snapping_client, vec2 check_pos)
{
	if(snapping_client == -1)
		return 0;
	if(distance(game.players[snapping_client]->view_pos, check_pos) > 1000.0f)
		return 1;
	return 0;
}
