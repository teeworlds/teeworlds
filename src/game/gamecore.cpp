/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include "gamecore.hpp"

const char *TUNING_PARAMS::names[] =
{
	#define MACRO_TUNING_PARAM(name,value) #name,
	#include "tuning.hpp"
	#undef MACRO_TUNING_PARAM
};


bool TUNING_PARAMS::set(int index, float value)
{
	if(index < 0 || index >= num())
		return false;
	((tune_param *)this)[index] = value;
	return true;
}

bool TUNING_PARAMS::get(int index, float *value)
{
	if(index < 0 || index >= num())
		return false;
	*value = (float)((tune_param *)this)[index];
	return true;
}

bool TUNING_PARAMS::set(const char *name, float value)
{
	for(int i = 0; i < num(); i++)
		if(strcmp(name, names[i]) == 0)
			return set(i, value);
	return false;
}

bool TUNING_PARAMS::get(const char *name, float *value)
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

float hermite_basis1(float v)
{
	return 2*v*v*v - 3*v*v+1;
}

float velocity_ramp(float value, float start, float range, float curvature)
{
	if(value < start)
		return 1.0f;
	return 1.0f/pow(curvature, (value-start)/range);
}

void CHARACTER_CORE::reset()
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

void CHARACTER_CORE::tick(bool use_input)
{
	float phys_size = 28.0f;
	triggered_events = 0;
	
	// get ground state
	bool grounded = false;
	if(col_check_point(pos.x+phys_size/2, pos.y+phys_size/2+5))
		grounded = true;
	if(col_check_point(pos.x-phys_size/2, pos.y+phys_size/2+5))
		grounded = true;
	
	vec2 target_direction = normalize(vec2(input.target_x, input.target_y));

	vel.y += world->tuning.gravity;
	
	float max_speed = grounded ? world->tuning.ground_control_speed : world->tuning.air_control_speed;
	float accel = grounded ? world->tuning.ground_control_accel : world->tuning.air_control_accel;
	float friction = grounded ? world->tuning.ground_friction : world->tuning.air_friction;
	
	// handle input
	if(use_input)
	{
		direction = input.direction;

		// setup angle
		float a = 0;
		if(input.target_x == 0)
			a = atan((float)input.target_y);
		else
			a = atan((float)input.target_y/(float)input.target_x);
			
		if(input.target_x < 0)
			a = a+pi;
			
		angle = (int)(a*256.0f);

		// handle jump
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

		// handle hook
		if(input.hook)
		{
			if(hook_state == HOOK_IDLE)
			{
				hook_state = HOOK_FLYING;
				hook_pos = pos+target_direction*phys_size*1.5f;
				hook_dir = target_direction;
				hooked_player = -1;
				hook_tick = 0;
				triggered_events |= COREEVENT_HOOK_LAUNCH;
			}		
		}
		else
		{
			hooked_player = -1;
			hook_state = HOOK_IDLE;
			hook_pos = pos;			
		}		
	}
	
	// add the speed modification according to players wanted direction
	if(direction < 0)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, -accel);
	if(direction > 0)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, accel);
	if(direction == 0)
		vel.x *= friction;
	
	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(grounded)
		jumped &= ~2;
	
	// do hook
	if(hook_state == HOOK_IDLE)
	{
		hooked_player = -1;
		hook_state = HOOK_IDLE;
		hook_pos = pos;
	}
	else if(hook_state >= HOOK_RETRACT_START && hook_state < HOOK_RETRACT_END)
	{
		hook_state++;
	}
	else if(hook_state == HOOK_RETRACT_END)
	{
		hook_state = HOOK_RETRACTED;
		triggered_events |= COREEVENT_HOOK_RETRACT;
		hook_state = HOOK_RETRACTED;
	}
	else if(hook_state == HOOK_FLYING)
	{
		vec2 new_pos = hook_pos+hook_dir*world->tuning.hook_fire_speed;
		if(distance(pos, new_pos) > world->tuning.hook_length)
		{
			hook_state = HOOK_RETRACT_START;
			new_pos = pos + normalize(new_pos-pos) * world->tuning.hook_length;
		}
		
		// make sure that the hook doesn't go though the ground
		bool going_to_hit_ground = false;
		bool going_to_retract = false;
		int hit = col_intersect_line(hook_pos, new_pos, &new_pos, 0);
		if(hit)
		{
			if(hit&COLFLAG_NOHOOK)
				going_to_retract = true;
			else
				going_to_hit_ground = true;
		}

		// Check against other players first
		if(world)
		{
			float dist = 0.0f;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CHARACTER_CORE *p = world->characters[i];
				if(!p || p == this)
					continue;

				vec2 closest_point = closest_point_on_line(hook_pos, new_pos, p->pos);
				if(distance(p->pos, closest_point) < phys_size+2.0f)
				{
					if (hooked_player == -1 || distance (hook_pos, p->pos) < dist)
					{
						triggered_events |= COREEVENT_HOOK_ATTACH_PLAYER;
						hook_state = HOOK_GRABBED;
						hooked_player = i;
						dist = distance (hook_pos, p->pos);
					}
				}
			}
		}
		
		if(hook_state == HOOK_FLYING)
		{
			// check against ground
			if(going_to_hit_ground)
			{
				triggered_events |= COREEVENT_HOOK_ATTACH_GROUND;
				hook_state = HOOK_GRABBED;
			}
			else if(going_to_retract)
			{
				triggered_events |= COREEVENT_HOOK_HIT_NOHOOK;
				hook_state = HOOK_RETRACT_START;
			}
			
			hook_pos = new_pos;
		}
	}
	
	if(hook_state == HOOK_GRABBED)
	{
		if(hooked_player != -1)
		{
			CHARACTER_CORE *p = world->characters[hooked_player];
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
			if((hookvel.x < 0 && direction < 0) || (hookvel.x > 0 && direction > 0)) 
				hookvel.x *= 0.95f;
			else
				hookvel.x *= 0.75f;
			
			vec2 new_vel = vel+hookvel;

			// check if we are under the legal limit for the hook
			if(length(new_vel) < world->tuning.hook_drag_speed || length(new_vel) < length(vel))
				vel = new_vel; // no problem. apply
				
		}

		// release hook (max hook time is 1.25
		hook_tick++;
		if(hooked_player != -1 && (hook_tick > SERVER_TICK_SPEED+SERVER_TICK_SPEED/5 || !world->characters[hooked_player]))
		{
			hooked_player = -1;
			hook_state = HOOK_RETRACTED;
			hook_pos = pos;			
		}
	}
	
	if(world)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CHARACTER_CORE *p = world->characters[i];
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
	if(length(vel) > 6000)
		vel = normalize(vel) * 6000;
}

void CHARACTER_CORE::move()
{
	float rampvalue = velocity_ramp(length(vel)*50, world->tuning.velramp_start, world->tuning.velramp_range, world->tuning.velramp_curvature);
	
	vel.x = vel.x*rampvalue;
	move_box(&pos, &vel, vec2(28.0f, 28.0f), 0);
	vel.x = vel.x*(1.0f/rampvalue);
}

void CHARACTER_CORE::write(NETOBJ_CHARACTER_CORE *obj_core)
{
	obj_core->x = round(pos.x);
	obj_core->y = round(pos.y);
	
	obj_core->vx = round(vel.x*256.0f);
	obj_core->vy = round(vel.y*256.0f);
	obj_core->hook_state = hook_state;
	obj_core->hook_tick = hook_tick;
	obj_core->hook_x = round(hook_pos.x);
	obj_core->hook_y = round(hook_pos.y);
	obj_core->hook_dx = round(hook_dir.x*256.0f);
	obj_core->hook_dy = round(hook_dir.y*256.0f);
	obj_core->hooked_player = hooked_player;
	obj_core->jumped = jumped;
	obj_core->direction = direction;
	obj_core->angle = angle;
}

void CHARACTER_CORE::read(const NETOBJ_CHARACTER_CORE *obj_core)
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
	direction = obj_core->direction;
	angle = obj_core->angle;
}

void CHARACTER_CORE::quantize()
{
	NETOBJ_CHARACTER_CORE c;
	write(&c);
	read(&c);
}

