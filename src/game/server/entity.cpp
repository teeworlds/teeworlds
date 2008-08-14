
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
