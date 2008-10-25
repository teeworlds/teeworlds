
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
	
	float dx = game.players[snapping_client]->view_pos.x-check_pos.x;
	float dy = game.players[snapping_client]->view_pos.y-check_pos.y;
	
	if(fabs(dx) > 1000.0f || fabs(dy) > 800.0f)
		return 1;
	
	if(distance(game.players[snapping_client]->view_pos, check_pos) > 1100.0f)
		return 1;
	return 0;
}
