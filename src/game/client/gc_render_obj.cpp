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
#include "gc_skin.h"


void render_projectile(const NETOBJ_PROJECTILE *current, int itemid)
{
	if(debug_firedelay)
	{
		int64 delay = time_get()-debug_firedelay;
		dbg_msg("game", "firedelay=%.2f ms", delay/(float)time_freq()*1000.0f);
		debug_firedelay = 0;
	}
	
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();

	// get positions
	float curvature = 0;
	float speed = 0;
	if(current->type == WEAPON_GRENADE)
	{
		curvature = tuning.grenade_curvature;
		speed = tuning.grenade_speed;
	}
	else if(current->type == WEAPON_SHOTGUN)
	{
		curvature = tuning.shotgun_curvature;
		speed = tuning.shotgun_speed;
	}
	else if(current->type == WEAPON_GUN)
	{
		curvature = tuning.gun_curvature;
		speed = tuning.gun_speed;
	}

	float ct = (client_tick()-current->start_tick)/(float)SERVER_TICK_SPEED + client_ticktime()*1/(float)SERVER_TICK_SPEED;
	vec2 startpos(current->x, current->y);
	vec2 startvel(current->vx/100.0f, current->vy/100.0f);
	vec2 pos = calc_pos(startpos, startvel, curvature, speed, ct);
	vec2 prevpos = calc_pos(startpos, startvel, curvature, speed, ct-0.001f);

	select_sprite(data->weapons[clamp(current->type, 0, NUM_WEAPONS-1)].sprite_proj);
	vec2 vel = pos-prevpos;
	//vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	

	// add particle for this projectile
	if(current->type == WEAPON_GRENADE)
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

void render_powerup(const NETOBJ_POWERUP *prev, const NETOBJ_POWERUP *current)
{
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	float angle = 0.0f;
	float size = 64.0f;
	if (current->type == POWERUP_WEAPON)
	{
		angle = 0; //-pi/6;//-0.25f * pi * 2.0f;
		select_sprite(data->weapons[clamp(current->subtype, 0, NUM_WEAPONS-1)].sprite_body);
		size = data->weapons[clamp(current->subtype, 0, NUM_WEAPONS-1)].visual_size;
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
			effect_powerupshine(pos, vec2(96,18));
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

void render_flag(const NETOBJ_FLAG *prev, const NETOBJ_FLAG *current)
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
	
	// make sure that the flag isn't interpolated between capture and return
	if(prev->carried_by != current->carried_by)
		pos = vec2(current->x, current->y);

	// make sure to use predicted position if we are the carrier
	if(netobjects.local_info && current->carried_by == netobjects.local_info->cid)
		pos = local_character_pos;

    gfx_quads_draw(pos.x, pos.y-size*0.75f, size, size*2);
    gfx_quads_end();
}


void render_laser(const struct NETOBJ_LASER *current)
{

	vec2 pos = vec2(current->x, current->y);
	vec2 from = vec2(current->from_x, current->from_y);
	vec2 dir = normalize(pos-from);

	float ticks = client_tick() + client_intratick() - current->eval_tick;
	float ms = (ticks/50.0f) * 1000.0f;
	float a =  ms / tuning.laser_bounce_delay;
	a = clamp(a, 0.0f, 1.0f);
	float ia = 1-a;
	
	
	vec2 out, border;
	
	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	
	//vec4 inner_color(0.15f,0.35f,0.75f,1.0f);
	//vec4 outer_color(0.65f,0.85f,1.0f,1.0f);

	// do outline
	vec4 outer_color(0.075f,0.075f,0.25f,1.0f);
	gfx_setcolor(outer_color.r,outer_color.g,outer_color.b,1.0f);
	out = vec2(dir.y, -dir.x) * (7.0f*ia);

	gfx_quads_draw_freeform(
			from.x-out.x, from.y-out.y,
			from.x+out.x, from.y+out.y,
			pos.x-out.x, pos.y-out.y,
			pos.x+out.x, pos.y+out.y
		);

	// do inner	
	vec4 inner_color(0.5f,0.5f,1.0f,1.0f);
	out = vec2(dir.y, -dir.x) * (5.0f*ia);
	gfx_setcolor(inner_color.r, inner_color.g, inner_color.b, 1.0f); // center
	
	gfx_quads_draw_freeform(
			from.x-out.x, from.y-out.y,
			from.x+out.x, from.y+out.y,
			pos.x-out.x, pos.y-out.y,
			pos.x+out.x, pos.y+out.y
		);
		
	gfx_quads_end();
	
	// render head
	{
		gfx_blend_normal();
		gfx_texture_set(data->images[IMAGE_PARTICLES].id);
		gfx_quads_begin();

		int sprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		select_sprite(sprites[client_tick()%3]);
		gfx_quads_setrotation(client_tick());
		gfx_setcolor(outer_color.r,outer_color.g,outer_color.b,1.0f);
		gfx_quads_draw(pos.x, pos.y, 24,24);
		gfx_setcolor(inner_color.r, inner_color.g, inner_color.b, 1.0f);
		gfx_quads_draw(pos.x, pos.y, 20,20);
		gfx_quads_end();
	}
	
	gfx_blend_normal();	
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
	const NETOBJ_PLAYER_CHARACTER *prev_char,
	const NETOBJ_PLAYER_CHARACTER *player_char,
	const NETOBJ_PLAYER_INFO *prev_info,
	const NETOBJ_PLAYER_INFO *player_info
	)
{
	NETOBJ_PLAYER_CHARACTER prev;
	NETOBJ_PLAYER_CHARACTER player;
	prev = *prev_char;
	player = *player_char;

	NETOBJ_PLAYER_INFO info = *player_info;
	tee_render_info render_info = client_datas[info.cid].render_info;

	// check for teamplay modes
	bool is_teamplay = false;
	if(netobjects.gameobj && netobjects.gameobj->gametype != GAMETYPE_DM)
		is_teamplay = true;

	// check for ninja	
	if (player.weapon == WEAPON_NINJA)
	{
		// change the skin for the player to the ninja
		int skin = skin_find("x_ninja");
		if(skin != -1)
		{
			if(is_teamplay)
				render_info.texture = skin_get(skin)->color_texture;
			else
			{
				render_info.texture = skin_get(skin)->org_texture;
				render_info.color_body = vec4(1,1,1,1);
				render_info.color_feet = vec4(1,1,1,1);
			}
		}	
	}
	
	// set size
	render_info.size = 64.0f;

	float intratick = client_intratick();
	float ticktime = client_ticktime();
	
	if(player.health < 0) // dont render dead players
		return;

	//float angle = mix((float)prev.angle, (float)player.angle, intratick)/256.0f;
	
	// TODO: fix this good!
	float mixspeed = 0.05f;
	if(player.attacktick != prev.attacktick)
		mixspeed = 0.1f;
	
	float angle = mix(client_datas[info.cid].angle, player.angle/256.0f, mixspeed);
	client_datas[info.cid].angle = angle;
	vec2 direction = get_direction((int)(angle*256.0f));
	
	if(info.local && config.cl_predict)
	{
		if(!netobjects.local_character || (netobjects.local_character->health < 0) || (netobjects.gameobj && netobjects.gameobj->game_over))
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

	vec2 position = mix(vec2(prev.x, prev.y), vec2(player.x, player.y), intratick);
	vec2 vel = mix(vec2(prev.vx/256.0f, prev.vy/256.0f), vec2(player.vx/256.0f, player.vy/256.0f), intratick);
	
	flow_add(position, vel*100.0f, 10.0f);
	
	render_info.got_airjump = player.jumped&2?0:1;

	if(prev.health < 0) // Don't flicker from previous position
		position = vec2(player.x, player.y);

	bool stationary = player.vx < 1 && player.vx > -1;
	bool inair = col_check_point(player.x, player.y+16) == 0;
	bool want_other_dir = (player.wanted_direction == -1 && vel.x > 0) || (player.wanted_direction == 1 && vel.x < 0);

	// evaluate animation
	float walk_time = fmod(position.x, 100.0f)/100.0f;
	animstate state;
	anim_eval(&data->animations[ANIM_BASE], 0, &state);

	if(inair)
		anim_eval_add(&state, &data->animations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	else if(stationary)
		anim_eval_add(&state, &data->animations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
	else if(!want_other_dir)
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
	
	// do skidding
	if(!inair && want_other_dir && length(vel*50) > 500.0f)
	{
		static int64 skid_sound_time = 0;
		if(time_get()-skid_sound_time > time_freq()/10)
		{
			snd_play_random(CHN_WORLD, SOUND_PLAYER_SKID, 0.25f, position);
			skid_sound_time = time_get();
		}
		
		effect_skidtrail(
			position+vec2(-player.wanted_direction*6,12),
			vec2(-player.wanted_direction*100*length(vel),-50)
		);
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
			if(netobjects.local_info && player_char->hooked_player == netobjects.local_info->cid)
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
		int i = 0;
		for(float f = 24; f < d && i < 1024; f += 24, i++)
		{
			vec2 p = hook_pos + dir*f;
			gfx_quads_draw(p.x, p.y,24,16);
		}

		gfx_quads_setrotation(0);
		gfx_quads_end();

		render_hand(&render_info, position, normalize(hook_pos-pos), -pi/2, vec2(20, 0));
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
			p = position + vec2(state.attach.x, state.attach.y);
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
				effect_powerupshine(p+vec2(32,0), vec2(32,12));
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
				effect_powerupshine(p-vec2(32,0), vec2(32,12));
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
				}
			}
		}
		gfx_quads_end();

		switch (player.weapon)
		{
			case WEAPON_GUN: render_hand(&render_info, p, direction, -3*pi/4, vec2(-15, 4)); break;
			case WEAPON_SHOTGUN: render_hand(&render_info, p, direction, -pi/2, vec2(-5, 4)); break;
			case WEAPON_GRENADE: render_hand(&render_info, p, direction, -pi/2, vec2(-4, 7)); break;
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

	render_info.size = 64.0f; // force some settings
	render_info.color_body.a = 1.0f;
	render_info.color_feet.a = 1.0f;
	render_tee(&state, &render_info, player.emote, direction, position);

	if(player.player_state == PLAYERSTATE_CHATTING)
	{
		gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
		gfx_quads_begin();
		select_sprite(SPRITE_DOTDOT);
		gfx_quads_draw(position.x + 24, position.y - 40, 64,64);
		gfx_quads_end();
	}

	if (client_datas[info.cid].emoticon_start != -1 && client_datas[info.cid].emoticon_start + 2 * client_tickspeed() > client_tick())
	{
		gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
		gfx_quads_begin();

		int since_start = client_tick() - client_datas[info.cid].emoticon_start;
		int from_end = client_datas[info.cid].emoticon_start + 2 * client_tickspeed() - client_tick();

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
		select_sprite(SPRITE_OOP + client_datas[info.cid].emoticon);
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
			
		const char *name = client_datas[info.cid].name;
		float tw = gfx_text_width(0, 28.0f, name, -1);
		gfx_text_color(1,1,1,a);
		gfx_text(0, position.x-tw/2.0f, position.y-60, 28.0f, name, -1);
		
		if(config.debug) // render client id when in debug aswell
		{
			char buf[128];
			str_format(buf, sizeof(buf),"%d", info.cid);
			gfx_text(0, position.x, position.y-90, 28.0f, buf, -1);
		}

		gfx_text_color(1,1,1,1);
	}
}
