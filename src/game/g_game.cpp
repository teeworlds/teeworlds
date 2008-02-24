/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include "g_game.h"

const char *tuning_params::names[] =
{
	#define MACRO_TUNING_PARAM(name,value) #name,
	#include "g_tuning.h"
	#undef MACRO_TUNING_PARAM
};


bool tuning_params::set(int index, float value)
{
	if(index < 0 || index >= num())
		return false;
	((tune_param *)this)[index] = value;
	return true;
}

bool tuning_params::get(int index, float *value)
{
	if(index < 0 || index >= num())
		return false;
	*value = (float)((tune_param *)this)[index];
	return true;
}

bool tuning_params::set(const char *name, float value)
{
	for(int i = 0; i < num(); i++)
		if(strcmp(name, names[i]) == 0)
			return set(i, value);
	return false;
}

bool tuning_params::get(const char *name, float *value)
{
	for(int i = 0; i < num(); i++)
		if(strcmp(name, names[i]) == 0)
			return get(i, value);
	
	return false;
}

// TODO: OPT: rewrite this smarter!
void move_point(vec2 *inout_pos, vec2 *inout_vel, float elasticity, int *bounces)
{
	if(bounces)
		*bounces = 0;
	
	vec2 pos = *inout_pos;
	vec2 vel = *inout_vel;
	if(col_check_point(pos + vel))
	{
		int affected = 0;
		if(col_check_point(pos.x + vel.x, pos.y))
		{
			inout_vel->x *= -elasticity;
			if(bounces)
				(*bounces)++;			
			affected++;
		}

		if(col_check_point(pos.x, pos.y + vel.y))
		{
			inout_vel->y *= -elasticity;
			if(bounces)
				(*bounces)++;			
			affected++;
		}
		
		if(affected == 0)
		{
			inout_vel->x *= -elasticity;
			inout_vel->y *= -elasticity;
		}
	}
	else
	{
		*inout_pos = pos + vel;
	}
}

bool test_box(vec2 pos, vec2 size)
{
	size *= 0.5f;
	if(col_check_point(pos.x-size.x, pos.y-size.y))
		return true;
	if(col_check_point(pos.x+size.x, pos.y-size.y))
		return true;
	if(col_check_point(pos.x-size.x, pos.y+size.y))
		return true;
	if(col_check_point(pos.x+size.x, pos.y+size.y))
		return true;
	return false;
}

void move_box(vec2 *inout_pos, vec2 *inout_vel, vec2 size, float elasticity)
{
	// do the move
	vec2 pos = *inout_pos;
	vec2 vel = *inout_vel;
	
	float distance = length(vel);
	int max = (int)distance;
	
	if(distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float fraction = 1.0f/(float)(max+1);
		for(int i = 0; i <= max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;
			
			vec2 new_pos = pos + vel*fraction; // TODO: this row is not nice
			
			if(test_box(vec2(new_pos.x, new_pos.y), size))
			{
				int hits = 0;
				
				if(test_box(vec2(pos.x, new_pos.y), size))
				{
					new_pos.y = pos.y;
					vel.y *= -elasticity;
					hits++;
				}
				
				if(test_box(vec2(new_pos.x, pos.y), size))
				{
					new_pos.x = pos.x;
					vel.x *= -elasticity;
					hits++;
				}
				
				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(hits == 0)
				{
					new_pos.y = pos.y;
					vel.y *= -elasticity;
					new_pos.x = pos.x;
					vel.x *= -elasticity;
				}
			}
			
			pos = new_pos;
		}
	}
	
	*inout_pos = pos;
	*inout_vel = vel;
}


void player_core::reset()
{
	pos = vec2(0,0);
	vel = vec2(0,0);
	hook_pos = vec2(0,0);
	hook_dir = vec2(0,0);
	hook_tick = 0;
	hook_state = HOOK_IDLE;
	hooked_player = -1;
	jumped = 0;
	triggered_events = 0;
}

void player_core::tick()
{
	float phys_size = 28.0f;
	triggered_events = 0;
	
	bool grounded = false;
	if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
		grounded = true;
	if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
		grounded = true;
	
	vec2 direction = normalize(vec2(input.target_x, input.target_y));

	vel.y += world->tuning.gravity;
	
	float max_speed = grounded ? world->tuning.ground_control_speed : world->tuning.air_control_speed;
	float accel = grounded ? world->tuning.ground_control_accel : world->tuning.air_control_accel;
	float friction = grounded ? world->tuning.ground_friction : world->tuning.air_friction;
	
	// handle movement
	if(input.left)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, -accel);
	if(input.right)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, accel);
		
	if(!input.left && !input.right)
		vel.x *= friction;
	
	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(grounded)
		jumped &= ~2;
	
	if(input.jump)
	{
		if(!(jumped&1))
		{
			if(grounded)
			{
				triggered_events |= COREEVENT_GROUND_JUMP;
				vel.y = -world->tuning.ground_jump_impulse;
				jumped |= 1;
			}
			else if(!(jumped&2))
			{
				triggered_events |= COREEVENT_AIR_JUMP;
				vel.y = -world->tuning.air_jump_impulse;
				jumped |= 3;
			}
		}
	}
	else
		jumped &= ~1;
	
	// do hook
	if(input.hook)
	{
		if(hook_state == HOOK_IDLE)
		{
			hook_state = HOOK_FLYING;
			hook_pos = pos;
			hook_dir = direction;
			hooked_player = -1;
			hook_tick = 0;
			triggered_events |= COREEVENT_HOOK_LAUNCH;
		}
		else if(hook_state == HOOK_FLYING)
		{
			vec2 new_pos = hook_pos+hook_dir*world->tuning.hook_fire_speed;

			// Check against other players first
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				player_core *p = world->players[i];
				if(!p || p == this)
					continue;

				//if(p != this && !p->dead && distance(p->pos, new_pos) < p->phys_size)
				if(distance(p->pos, new_pos) < phys_size)
				{
					triggered_events |= COREEVENT_HOOK_ATTACH_PLAYER;
					hook_state = HOOK_GRABBED;
					hooked_player = i;
					break;
				}
			}
			/*
			for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
			{
				if(ent && ent->objtype == OBJTYPE_PLAYER)
				{
					player *p = (player*)ent;
					if(p != this && !p->dead && distance(p->pos, new_pos) < p->phys_size)
					{
						hook_state = HOOK_GRABBED;
						hooked_player = p;
						break;
					}
				}
			}*/
			
			if(hook_state == HOOK_FLYING)
			{
				// check against ground
				if(col_intersect_line(hook_pos, new_pos, &new_pos))
				{
					triggered_events |= COREEVENT_HOOK_ATTACH_GROUND;
					hook_state = HOOK_GRABBED;
					hook_pos = new_pos;	
				}
				else if(distance(pos, new_pos) > world->tuning.hook_length)
				{
					triggered_events |= COREEVENT_HOOK_RETRACT;
					hook_state = HOOK_RETRACTED;
				}
				else
					hook_pos = new_pos;
			}
			
			if(hook_state == HOOK_GRABBED)
			{
				//create_sound(pos, SOUND_HOOK_ATTACH);
				//hook_tick = server_tick();
			}
		}
	}
	else
	{
		//release_hooked();
		hooked_player = -1;
		hook_state = HOOK_IDLE;
		hook_pos = pos;
	}
	
	if(hook_state == HOOK_GRABBED)
	{
		if(hooked_player != -1)
		{
			player_core *p = world->players[hooked_player];
			if(p)
				hook_pos = p->pos;
			else
			{
				// release hook
				hooked_player = -1;
				hook_state = HOOK_RETRACTED;
				hook_pos = pos;					
			}
			
			// keep players hooked for a max of 1.5sec
			//if(server_tick() > hook_tick+(server_tickspeed()*3)/2)
				//release_hooked();
		}
		
		// don't do this hook rutine when we are hook to a player
		if(hooked_player == -1 && distance(hook_pos, pos) > 46.0f)
		{
			vec2 hookvel = normalize(hook_pos-pos)*world->tuning.hook_drag_accel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(hookvel.y > 0)
				hookvel.y *= 0.3f;
			
			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((hookvel.x < 0 && input.left) || (hookvel.x > 0 && input.right)) 
				hookvel.x *= 0.95f;
			else
				hookvel.x *= 0.75f;
			
			vec2 new_vel = vel+hookvel;

			// check if we are under the legal limit for the hook
			if(length(new_vel) < world->tuning.hook_drag_speed || length(new_vel) < length(vel))
				vel = new_vel; // no problem. apply
				
		}

		// release hook		
		hook_tick++;
		if(hooked_player != -1 && hook_tick > SERVER_TICK_SPEED*2)
		{
			hooked_player = -1;
			hook_state = HOOK_RETRACTED;
			hook_pos = pos;			
		}
	}
	
	if(true)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			player_core *p = world->players[i];
			if(!p)
				continue;
			
			//player *p = (player*)ent;
			if(p == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self
			
			// handle player <-> player collision
			float d = distance(pos, p->pos);
			vec2 dir = normalize(pos - p->pos);
			if(d < phys_size*1.25f && d > 1.0f)
			{
				float a = (phys_size*1.45f - d);
				
				// make sure that we don't add excess force by checking the
				// direction against the current velocity
				vec2 veldir = normalize(vel);
				float v = 1-(dot(veldir, dir)+1)/2;
				vel = vel + dir*a*(v*0.75f);
				vel = vel * 0.85f;
			}
			
			// handle hook influence
			if(hooked_player == i)
			{
				if(d > phys_size*1.50f) // TODO: fix tweakable variable
				{
					float accel = world->tuning.hook_drag_accel * (d/world->tuning.hook_length);
					float drag_speed = world->tuning.hook_drag_speed;
					
					// add force to the hooked player
					p->vel.x = saturated_add(-drag_speed, drag_speed, p->vel.x, accel*dir.x*1.5f);
					p->vel.y = saturated_add(-drag_speed, drag_speed, p->vel.y, accel*dir.y*1.5f);

					// add a little bit force to the guy who has the grip
					vel.x = saturated_add(-drag_speed, drag_speed, vel.x, -accel*dir.x*0.25f);
					vel.y = saturated_add(-drag_speed, drag_speed, vel.y, -accel*dir.y*0.25f);
				}
			}
		}
	}	

	// clamp the velocity to something sane
	if(length(vel) > world->tuning.terminal_velocity)
		vel = normalize(vel) * world->tuning.terminal_velocity;
}

void player_core::move()
{
	move_box(&pos, &vel, vec2(28.0f, 28.0f), 0);
}

void player_core::write(NETOBJ_PLAYER_CORE *obj_core)
{
	obj_core->x = (int)pos.x;
	obj_core->y = (int)pos.y;
	obj_core->vx = (int)(vel.x*256.0f);
	obj_core->vy = (int)(vel.y*256.0f);
	obj_core->hook_state = hook_state;
	obj_core->hook_tick = hook_tick;
	obj_core->hook_x = (int)hook_pos.x;
	obj_core->hook_y = (int)hook_pos.y;
	obj_core->hook_dx = (int)(hook_dir.x*256.0f);
	obj_core->hook_dy = (int)(hook_dir.y*256.0f);
	obj_core->hooked_player = hooked_player;
	obj_core->jumped = jumped;

	float a = 0;
	if(input.target_x == 0)
		a = atan((float)input.target_y);
	else
		a = atan((float)input.target_y/(float)input.target_x);
		
	if(input.target_x < 0)
		a = a+pi;
		
	obj_core->angle = (int)(a*256.0f);
}

void player_core::read(const NETOBJ_PLAYER_CORE *obj_core)
{
	pos.x = obj_core->x;
	pos.y = obj_core->y;
	vel.x = obj_core->vx/256.0f;
	vel.y = obj_core->vy/256.0f;
	hook_state = obj_core->hook_state;
	hook_tick = obj_core->hook_tick;
	hook_pos.x = obj_core->hook_x;
	hook_pos.y = obj_core->hook_y;
	hook_dir.x = obj_core->hook_dx/256.0f;
	hook_dir.y = obj_core->hook_dy/256.0f;
	hooked_player = obj_core->hooked_player;
	jumped = obj_core->jumped;
}

void player_core::quantize()
{
	NETOBJ_PLAYER_CORE c;
	write(&c);
	read(&c);
}

