/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <math.h>
#include <stdio.h>
#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include "../generated/gc_data.h"
#include "../g_protocol.h"
#include "../g_math.h"
#include "gc_render.h"
#include "gc_anim.h"
#include "gc_client.h"


void render_projectile(const obj_projectile *current, int itemid)
{
	/*
	if(debug_firedelay)
	{
		debug_firedelay = time_get()-debug_firedelay;
		dbg_msg("game", "firedelay=%.2f ms", debug_firedelay/(float)time_freq()*1000.0f);
		debug_firedelay = 0;
	}*/
	
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();

	// get positions
	float gravity = -400;
	if(current->type != WEAPON_ROCKET)
		gravity = -100;

	float ct = (client_tick()-current->start_tick)/(float)SERVER_TICK_SPEED + client_ticktime()*1/(float)SERVER_TICK_SPEED;
	vec2 startpos(current->x, current->y);
	vec2 startvel(current->vx, current->vy);
	vec2 pos = calc_pos(startpos, startvel, gravity, ct);
	vec2 prevpos = calc_pos(startpos, startvel, gravity, ct-0.001f);

	select_sprite(data->weapons[current->type%data->num_weapons].sprite_proj);
	vec2 vel = pos-prevpos;
	//vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	

	// add particle for this projectile
	if(current->type == WEAPON_ROCKET)
	{
		effect_smoketrail(pos, vel*-1);
		flow_add(pos, vel*1000*client_frametime(), 10.0f);
		gfx_quads_setrotation(client_localtime()*pi*2*2 + itemid);
	}
	else
	{
		effect_bullettrail(pos);
		flow_add(pos, vel*1000*client_frametime(), 10.0f);

		if(length(vel) > 0.00001f)
			gfx_quads_setrotation(get_angle(vel));
		else
			gfx_quads_setrotation(0);

	}

	gfx_quads_draw(pos.x, pos.y, 32, 32);
	gfx_quads_setrotation(0);
	gfx_quads_end();
}

void render_powerup(const obj_powerup *prev, const obj_powerup *current)
{
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	float angle = 0.0f;
	float size = 64.0f;
	if (current->type == POWERUP_WEAPON)
	{
		angle = 0; //-pi/6;//-0.25f * pi * 2.0f;
		select_sprite(data->weapons[current->subtype%data->num_weapons].sprite_body);
		size = data->weapons[current->subtype%data->num_weapons].visual_size;
	}
	else
	{
		const int c[] = {
			SPRITE_POWERUP_HEALTH,
			SPRITE_POWERUP_ARMOR,
			SPRITE_POWERUP_WEAPON,
			SPRITE_POWERUP_NINJA,
			SPRITE_POWERUP_TIMEFIELD
			};
		select_sprite(c[current->type]);

		if(c[current->type] == SPRITE_POWERUP_NINJA)
		{
			/*
			proj_particles.addparticle(0, 0,
				pos+vec2((frandom()-0.5f)*80.0f, (frandom()-0.5f)*20.0f),
				vec2((frandom()-0.5f)*10.0f, (frandom()-0.5f)*10.0f));*/
			size *= 2.0f;
			pos.x += 10.0f;
		}
	}

	gfx_quads_setrotation(angle);

	float offset = pos.y/32.0f + pos.x/32.0f;
	pos.x += cosf(client_localtime()*2.0f+offset)*2.5f;
	pos.y += sinf(client_localtime()*2.0f+offset)*2.5f;
	draw_sprite(pos.x, pos.y, size);
	gfx_quads_end();
}

void render_flag(const obj_flag *prev, const obj_flag *current)
{
	float angle = 0.0f;
	float size = 42.0f;

    gfx_blend_normal();
    gfx_texture_set(data->images[IMAGE_GAME].id);
    gfx_quads_begin();

	if(current->team == 0) // red team
		select_sprite(SPRITE_FLAG_RED);
	else
		select_sprite(SPRITE_FLAG_BLUE);

	gfx_quads_setrotation(angle);

	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());

	if(local_info && current->carried_by == local_info->clientid)
		pos = local_character_pos;

    //gfx_setcolor(current->team ? 0 : 1,0,current->team ? 1 : 0,1);
    //draw_sprite(pos.x, pos.y, size);
    gfx_quads_draw(pos.x, pos.y-size*0.75f, size, size*2);
    gfx_quads_end();
}



static void render_hand(tee_render_info *info, vec2 center_pos, vec2 dir, float angle_offset, vec2 post_rot_offset)
{
	// for drawing hand
	//const skin *s = skin_get(skin_id);
	
	float basesize = 10.0f;
	//dir = normalize(hook_pos-pos);

	vec2 hand_pos = center_pos + dir;
	float angle = get_angle(dir);
	if (dir.x < 0)
		angle -= angle_offset;
	else
		angle += angle_offset;

	vec2 dirx = dir;
	vec2 diry(-dir.y,dir.x);

	if (dir.x < 0)
		diry = -diry;

	hand_pos += dirx * post_rot_offset.x;
	hand_pos += diry * post_rot_offset.y;

	//gfx_texture_set(data->images[IMAGE_CHAR_DEFAULT].id);
	gfx_texture_set(info->texture);
	gfx_quads_begin();
	gfx_setcolor(info->color_body.r, info->color_body.g, info->color_body.b, info->color_body.a);

	// two passes
	for (int i = 0; i < 2; i++)
	{
		bool outline = i == 0;

		select_sprite(outline?SPRITE_TEE_HAND_OUTLINE:SPRITE_TEE_HAND, 0, 0, 0);
		gfx_quads_setrotation(angle);
		gfx_quads_draw(hand_pos.x, hand_pos.y, 2*basesize, 2*basesize);
	}

	gfx_quads_setrotation(0);
	gfx_quads_end();
}

void render_player(
	const obj_player_character *prev_char,
	const obj_player_character *player_char,
	const obj_player_info *prev_info,
	const obj_player_info *player_info
	)
{
	obj_player_character prev;
	obj_player_character player;
	prev = *prev_char;
	player = *player_char;

	obj_player_info info = *player_info;
	tee_render_info render_info = client_datas[info.clientid].render_info;

	float intratick = client_intratick();
	float ticktime = client_ticktime();

	if(player.health < 0) // dont render dead players
		return;

	if(info.local && config.cl_predict)
	{
		if(!local_character || (local_character->health < 0) || (gameobj && gameobj->game_over))
		{
		}
		else
		{
			// apply predicted results
			predicted_player.write(&player);
			predicted_prev_player.write(&prev);
			intratick = client_predintratick();
		}
	}

	vec2 direction = get_direction(player.angle);
	float angle = player.angle/256.0f;
	vec2 position = mix(vec2(prev.x, prev.y), vec2(player.x, player.y), intratick);
	vec2 vel = vec2(player.x, player.y)-vec2(prev.x, prev.y);
	
	flow_add(position, vel*100.0f, 10.0f);
	
	render_info.got_airjump = player.jumped&2?0:1;

	if(prev.health < 0) // Don't flicker from previous position
		position = vec2(player.x, player.y);

	bool stationary = player.vx < 1 && player.vx > -1;
	bool inair = col_check_point(player.x, player.y+16) == 0;

	// evaluate animation
	float walk_time = fmod(position.x, 100.0f)/100.0f;
	animstate state;
	anim_eval(&data->animations[ANIM_BASE], 0, &state);

	if(inair)
		anim_eval_add(&state, &data->animations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	else if(stationary)
		anim_eval_add(&state, &data->animations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
	else
		anim_eval_add(&state, &data->animations[ANIM_WALK], walk_time, 1.0f);

	if (player.weapon == WEAPON_HAMMER)
	{
		float a = clamp((client_tick()-player.attacktick+ticktime)/10.0f, 0.0f, 1.0f);
		anim_eval_add(&state, &data->animations[ANIM_HAMMER_SWING], a, 1.0f);
	}
	if (player.weapon == WEAPON_NINJA)
	{
		float a = clamp((client_tick()-player.attacktick+ticktime)/40.0f, 0.0f, 1.0f);
		anim_eval_add(&state, &data->animations[ANIM_NINJA_SWING], a, 1.0f);
	}

	// draw hook
	if (prev.hook_state>0 && player.hook_state>0)
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		//gfx_quads_begin();

		vec2 pos = position;
		vec2 hook_pos;
		
		if(player_char->hooked_player != -1)
		{
			if(local_info && player_char->hooked_player == local_info->clientid)
			{
				hook_pos = mix(vec2(predicted_prev_player.pos.x, predicted_prev_player.pos.y),
					vec2(predicted_player.pos.x, predicted_player.pos.y), client_predintratick());
			}
			else
				hook_pos = mix(vec2(prev_char->hook_x, prev_char->hook_y), vec2(player_char->hook_x, player_char->hook_y), client_intratick());
		}
		else
			hook_pos = mix(vec2(prev.hook_x, prev.hook_y), vec2(player.hook_x, player.hook_y), intratick);

		float d = distance(pos, hook_pos);
		vec2 dir = normalize(pos-hook_pos);

		gfx_quads_setrotation(get_angle(dir)+pi);

		// render head
		select_sprite(SPRITE_HOOK_HEAD);
		gfx_quads_draw(hook_pos.x, hook_pos.y, 24,16);

		// render chain
		select_sprite(SPRITE_HOOK_CHAIN);
		for(float f = 24; f < d; f += 24)
		{
			vec2 p = hook_pos + dir*f;
			gfx_quads_draw(p.x, p.y,24,16);
		}

		gfx_quads_setrotation(0);
		gfx_quads_end();

		render_hand(&client_datas[info.clientid].render_info, position, normalize(hook_pos-pos), -pi/2, vec2(20, 0));
	}

	// draw gun
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		gfx_quads_setrotation(state.attach.angle*pi*2+angle);

		// normal weapons
		int iw = clamp(player.weapon, 0, NUM_WEAPONS-1);
		select_sprite(data->weapons[iw].sprite_body, direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

		vec2 dir = direction;
		float recoil = 0.0f;
		vec2 p;
		if (player.weapon == WEAPON_HAMMER)
		{
			// Static position for hammer
			p = position;
			p.y += data->weapons[iw].offsety;
			// if attack is under way, bash stuffs
			if(direction.x < 0)
			{
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
				p.x -= data->weapons[iw].offsetx;
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
			}
			draw_sprite(p.x, p.y, data->weapons[iw].visual_size);
		}
		else if (player.weapon == WEAPON_NINJA)
		{
			p = position;
			p.y += data->weapons[iw].offsety;

			if(direction.x < 0)
			{
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
				p.x -= data->weapons[iw].offsetx;
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
			}
			draw_sprite(p.x, p.y, data->weapons[iw].visual_size);

			// HADOKEN
			if ((client_tick()-player.attacktick) <= (SERVER_TICK_SPEED / 6) && data->weapons[iw].nummuzzlesprites)
			{
				int itex = rand() % data->weapons[iw].nummuzzlesprites;
				float alpha = 1.0f;
				if (alpha > 0.0f && data->weapons[iw].sprite_muzzle[itex].psprite)
				{
					vec2 dir = vec2(player_char->x,player_char->y) - vec2(prev_char->x, prev_char->y);
					dir = normalize(dir);
					float hadokenangle = get_angle(dir);
					gfx_quads_setrotation(hadokenangle);
					//float offsety = -data->weapons[iw].muzzleoffsety;
					select_sprite(data->weapons[iw].sprite_muzzle[itex].psprite, 0);
					vec2 diry(-dir.y,dir.x);
					p = position;
					float offsetx = data->weapons[iw].muzzleoffsetx;
					p -= dir * offsetx;
					draw_sprite(p.x, p.y, 160.0f);
				}
			}
		}
		else
		{
			// TODO: should be an animation
			recoil = 0;
			float a = (client_tick()-player.attacktick+intratick)/5.0f;
			if(a < 1)
				recoil = sinf(a*pi);
			p = position + dir * data->weapons[iw].offsetx - dir*recoil*10.0f;
			p.y += data->weapons[iw].offsety;
			draw_sprite(p.x, p.y, data->weapons[iw].visual_size);
		}

		if (player.weapon == WEAPON_GUN || player.weapon == WEAPON_SHOTGUN)
		{
			// check if we're firing stuff
			if (true)//prev.attackticks)
			{
				float alpha = 0.0f;
				int phase1tick = (client_tick() - player.attacktick);
				if (phase1tick < (data->weapons[iw].muzzleduration + 3))
				{
					float t = ((((float)phase1tick) + intratick)/(float)data->weapons[iw].muzzleduration);
					alpha = LERP(2.0, 0.0f, min(1.0f,max(0.0f,t)));
				}

				int itex = rand() % data->weapons[iw].nummuzzlesprites;
				if (alpha > 0.0f && data->weapons[iw].sprite_muzzle[itex].psprite)
				{
					float offsety = -data->weapons[iw].muzzleoffsety;
					select_sprite(data->weapons[iw].sprite_muzzle[itex].psprite, direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
					if(direction.x < 0)
						offsety = -offsety;

					vec2 diry(-dir.y,dir.x);
					vec2 muzzlepos = p + dir * data->weapons[iw].muzzleoffsetx + diry * offsety;

					draw_sprite(muzzlepos.x, muzzlepos.y, data->weapons[iw].visual_size);
					/*gfx_setcolor(1.0f,1.0f,1.0f,alpha);
					vec2 diry(-dir.y,dir.x);
					p += dir * muzzleparams[player.weapon].offsetx + diry * offsety;
					gfx_quads_draw(p.x,p.y,muzzleparams[player.weapon].sizex, muzzleparams[player.weapon].sizey);*/
				}
			}
		}
		gfx_quads_end();

		switch (player.weapon)
		{
			case WEAPON_GUN: render_hand(&client_datas[info.clientid].render_info, p, direction, -3*pi/4, vec2(-15, 4)); break;
			case WEAPON_SHOTGUN: render_hand(&client_datas[info.clientid].render_info, p, direction, -pi/2, vec2(-5, 4)); break;
			case WEAPON_ROCKET: render_hand(&client_datas[info.clientid].render_info, p, direction, -pi/2, vec2(-4, 7)); break;
		}

	}

	// render the "shadow" tee
	if(info.local && config.debug)
	{
		vec2 ghost_position = mix(vec2(prev_char->x, prev_char->y), vec2(player_char->x, player_char->y), client_intratick());
		tee_render_info ghost = render_info;
		ghost.color_body.a = 0.5f;
		ghost.color_feet.a = 0.5f;
		render_tee(&state, &ghost, player.emote, direction, ghost_position); // render ghost
	}

	// render the tee
	render_tee(&state, &render_info, player.emote, direction, position);

	if(player.player_state == PLAYERSTATE_CHATTING)
	{
		gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
		gfx_quads_begin();
		select_sprite(SPRITE_DOTDOT);
		gfx_quads_draw(position.x + 24, position.y - 40, 64,64);
		gfx_quads_end();
	}

	if (client_datas[info.clientid].emoticon_start != -1 && client_datas[info.clientid].emoticon_start + 2 * client_tickspeed() > client_tick())
	{
		gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
		gfx_quads_begin();

		int since_start = client_tick() - client_datas[info.clientid].emoticon_start;
		int from_end = client_datas[info.clientid].emoticon_start + 2 * client_tickspeed() - client_tick();

		float a = 1;

		if (from_end < client_tickspeed() / 5)
			a = from_end / (client_tickspeed() / 5.0);

		float h = 1;
		if (since_start < client_tickspeed() / 10)
			h = since_start / (client_tickspeed() / 10.0);

		float wiggle = 0;
		if (since_start < client_tickspeed() / 5)
			wiggle = since_start / (client_tickspeed() / 5.0);

		float wiggle_angle = sin(5*wiggle);

		gfx_quads_setrotation(pi/6*wiggle_angle);

		gfx_setcolor(1.0f,1.0f,1.0f,a);
		// client_datas::emoticon is an offset from the first emoticon
		select_sprite(SPRITE_OOP + client_datas[info.clientid].emoticon);
		gfx_quads_draw(position.x, position.y - 23 - 32*h, 64, 64*h);
		gfx_quads_end();
	}
	
	// render name plate
	if(!info.local && config.cl_nameplates)
	{
		//gfx_text_color
		float a = 1;
		if(config.cl_nameplates_always == 0)
			a = clamp(1-powf(distance(local_target_pos, position)/200.0f,16.0f), 0.0f, 1.0f);
			
		const char *name = client_datas[info.clientid].name;
		float tw = gfx_text_width(0, 28.0f, name, -1);
		gfx_text_color(1,1,1,a);
		gfx_text(0, position.x-tw/2.0f, position.y-60, 28.0f, name, -1);
		gfx_text_color(1,1,1,1);
	}
}
