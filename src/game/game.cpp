/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "game.h"

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

static bool test_box(vec2 pos, vec2 size)
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
			
			if(test_box(new_pos, size))
			{
				if(test_box(vec2(pos.x, new_pos.y), size))
				{
					new_pos.y = pos.y;
					vel.y *= -elasticity;
				}
				
				if(test_box(vec2(new_pos.x, pos.y), size))
				{
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

void player_core::tick()
{
	float phys_size = 28.0f;
	
	#define MACRO_CHECK_VELOCITY { dbg_assert(length(vel) < 1000.0f, "velocity error"); }
	
	MACRO_CHECK_VELOCITY
	
	bool grounded = false;
	if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
		grounded = true;
	if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
		grounded = true;
	
	vec2 direction = normalize(vec2(input.target_x, input.target_y));

	vel.y += gravity;
	
	MACRO_CHECK_VELOCITY
	
	float max_speed = grounded ? ground_control_speed : air_control_speed;
	float accel = grounded ? ground_control_accel : air_control_accel;
	float friction = grounded ? ground_friction : air_friction;
	
	// handle movement
	if(input.left)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, -accel);
	if(input.right)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, accel);
		
	MACRO_CHECK_VELOCITY
		
	if(!input.left && !input.right)
		vel.x *= friction;
	
	MACRO_CHECK_VELOCITY
	
	// handle jumping
	if(input.jump)
	{
		if(!jumped && grounded)
		{
			//create_sound(pos, SOUND_PLAYER_JUMP);
			vel.y = -ground_jump_speed;
			jumped++;
		}
	}
	else
		jumped = 0;
	
	MACRO_CHECK_VELOCITY
	
	// do hook
	if(input.hook)
	{
		if(hook_state == HOOK_IDLE)
		{
			hook_state = HOOK_FLYING;
			hook_pos = pos;
			hook_dir = direction;
			hooked_player = -1;
			hook_tick = -1;
		}
		else if(hook_state == HOOK_FLYING)
		{
			vec2 new_pos = hook_pos+hook_dir*hook_fire_speed;

			// Check against other players first
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				player_core *p = world->players[i];
				if(!p || p == this)
					continue;

				//if(p != this && !p->dead && distance(p->pos, new_pos) < p->phys_size)
				if(distance(p->pos, new_pos) < phys_size)
				{
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
					hook_state = HOOK_GRABBED;
					hook_pos = new_pos;	
				}
				else if(distance(pos, new_pos) > hook_length)
				{
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
			
			// keep players hooked for a max of 1.5sec
			//if(server_tick() > hook_tick+(server_tickspeed()*3)/2)
				//release_hooked();
		}
		
		// Old version feels much better (to me atleast)
		if(distance(hook_pos, pos) > 46.0f)
		{
			vec2 hookvel = normalize(hook_pos-pos)*hook_drag_accel;
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
			if(length(new_vel) < hook_drag_speed || length(new_vel) < length(vel))
				vel = new_vel; // no problem. apply
				
		}
	}
	
	MACRO_CHECK_VELOCITY
	
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
				float a = phys_size*1.25f - d;
				vel = vel + dir*a;
			}
			
			MACRO_CHECK_VELOCITY
			
			// handle hook influence
			if(hooked_player == i)
			{
				if(d > phys_size*1.50f) // TODO: fix tweakable variable
				{
					float accel = hook_drag_accel * (d/hook_length);
					vel.x = saturated_add(-hook_drag_speed, hook_drag_speed, vel.x, -accel*dir.x);
					vel.y = saturated_add(-hook_drag_speed, hook_drag_speed, vel.y, -accel*dir.y);
					
					MACRO_CHECK_VELOCITY
				}
			}
		}
	}	
}

void player_core::move()
{
	move_box(&pos, &vel, vec2(28.0f, 28.0f), 0);
}

void player_core::write(obj_player_core *obj_core)
{
	obj_core->x = (int)pos.x;
	obj_core->y = (int)pos.y;
	obj_core->vx = (int)(vel.x*256.0f);
	obj_core->vy = (int)(vel.y*256.0f);
	obj_core->hook_state = hook_state;
	obj_core->hook_x = (int)hook_pos.x;
	obj_core->hook_y = (int)hook_pos.y;
	obj_core->hook_dx = (int)(hook_dir.x*256.0f);
	obj_core->hook_dy = (int)(hook_dir.y*256.0f);
	obj_core->hooked_player = hooked_player;

	float a = 0;
	if(input.target_x == 0)
		a = atan((float)input.target_y);
	else
		a = atan((float)input.target_y/(float)input.target_x);
		
	if(input.target_x < 0)
		a = a+pi;
		
	obj_core->angle = (int)(a*256.0f);
}

void player_core::read(const obj_player_core *obj_core)
{
	pos.x = obj_core->x;
	pos.y = obj_core->y;
	vel.x = obj_core->vx/256.0f;
	vel.y = obj_core->vy/256.0f;
	hook_state = obj_core->hook_state;
	hook_pos.x = obj_core->hook_x;
	hook_pos.y = obj_core->hook_y;
	hook_dir.x = obj_core->hook_dx/256.0f;
	hook_dir.y = obj_core->hook_dy/256.0f;
	hooked_player = obj_core->hooked_player;
	jumped = 0;
}

void player_core::quantize()
{
	obj_player_core c;
	write(&c);
	read(&c);
}

