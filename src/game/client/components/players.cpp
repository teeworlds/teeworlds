#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/gamecore.hpp> // get_angle
#include <game/client/animstate.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/ui.hpp>
#include <game/client/gc_render.hpp>

#include <game/client/components/flow.hpp>
#include <game/client/components/skins.hpp>
#include <game/client/components/effects.hpp>
#include <game/client/components/sounds.hpp>

#include "players.hpp"

void PLAYERS::render_hand(TEE_RENDER_INFO *info, vec2 center_pos, vec2 dir, float angle_offset, vec2 post_rot_offset)
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

inline float normalize_angular(float f)
{
	return fmod(f, pi*2);
}

inline float mix_angular(float src, float dst, float amount)
{
	src = normalize_angular(src);
	dst = normalize_angular(dst);
	float d0 = dst-src;
	float d1 = dst-(src+pi*2);
	if(fabs(d0) < fabs(d1))
		return src+d0*amount;
	return src+d1*amount;
}

void PLAYERS::render_player(
	const NETOBJ_CHARACTER *prev_char,
	const NETOBJ_CHARACTER *player_char,
	const NETOBJ_PLAYER_INFO *prev_info,
	const NETOBJ_PLAYER_INFO *player_info
	)
{
	NETOBJ_CHARACTER prev;
	NETOBJ_CHARACTER player;
	prev = *prev_char;
	player = *player_char;

	NETOBJ_PLAYER_INFO info = *player_info;
	TEE_RENDER_INFO render_info = gameclient.clients[info.cid].render_info;

	// check for teamplay modes
	bool is_teamplay = false;
	if(gameclient.snap.gameobj)
		is_teamplay = gameclient.snap.gameobj->flags&GAMEFLAG_TEAMS != 0;

	// check for ninja	
	if (player.weapon == WEAPON_NINJA)
	{
		// change the skin for the player to the ninja
		int skin = gameclient.skins->find("x_ninja");
		if(skin != -1)
		{
			if(is_teamplay)
				render_info.texture = gameclient.skins->get(skin)->color_texture;
			else
			{
				render_info.texture = gameclient.skins->get(skin)->org_texture;
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
	
	float angle = mix_angular(gameclient.clients[info.cid].angle, player.angle/256.0f, mixspeed);
	gameclient.clients[info.cid].angle = angle;
	vec2 direction = get_direction((int)(angle*256.0f));
	
	if(info.local && config.cl_predict)
	{
		if(!gameclient.snap.local_character || (gameclient.snap.local_character->health < 0) || (gameclient.snap.gameobj && gameclient.snap.gameobj->game_over))
		{
		}
		else
		{
			// apply predicted results
			gameclient.predicted_char.write(&player);
			gameclient.predicted_prev_char.write(&prev);
			intratick = client_predintratick();
		}
	}

	vec2 position = mix(vec2(prev.x, prev.y), vec2(player.x, player.y), intratick);
	vec2 vel = mix(vec2(prev.vx/256.0f, prev.vy/256.0f), vec2(player.vx/256.0f, player.vy/256.0f), intratick);
	
	gameclient.flow->add(position, vel*100.0f, 10.0f);
	
	render_info.got_airjump = player.jumped&2?0:1;

	if(prev.health < 0) // Don't flicker from previous position
		position = vec2(player.x, player.y);

	bool stationary = player.vx < 1 && player.vx > -1;
	bool inair = col_check_point(player.x, player.y+16) == 0;
	bool want_other_dir = (player.direction == -1 && vel.x > 0) || (player.direction == 1 && vel.x < 0);

	// evaluate animation
	float walk_time = fmod(position.x, 100.0f)/100.0f;
	ANIMSTATE state;
	state.set(&data->animations[ANIM_BASE], 0);

	if(inair)
		state.add(&data->animations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	else if(stationary)
		state.add(&data->animations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
	else if(!want_other_dir)
		state.add(&data->animations[ANIM_WALK], walk_time, 1.0f);

	if (player.weapon == WEAPON_HAMMER)
	{
		float a = clamp((client_tick()-player.attacktick+ticktime)/10.0f, 0.0f, 1.0f);
		state.add(&data->animations[ANIM_HAMMER_SWING], a, 1.0f);
	}
	if (player.weapon == WEAPON_NINJA)
	{
		float a = clamp((client_tick()-player.attacktick+ticktime)/40.0f, 0.0f, 1.0f);
		state.add(&data->animations[ANIM_NINJA_SWING], a, 1.0f);
	}
	
	// do skidding
	if(!inair && want_other_dir && length(vel*50) > 500.0f)
	{
		static int64 skid_sound_time = 0;
		if(time_get()-skid_sound_time > time_freq()/10)
		{
			gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_PLAYER_SKID, 0.25f, position);
			skid_sound_time = time_get();
		}
		
		gameclient.effects->skidtrail(
			position+vec2(-player.direction*6,12),
			vec2(-player.direction*100*length(vel),-50)
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
			if(gameclient.snap.local_info && player_char->hooked_player == gameclient.snap.local_info->cid)
			{
				hook_pos = mix(vec2(gameclient.predicted_prev_char.pos.x, gameclient.predicted_prev_char.pos.y),
					vec2(gameclient.predicted_char.pos.x, gameclient.predicted_char.pos.y), client_predintratick());
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
		select_sprite(data->weapons.id[iw].sprite_body, direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

		vec2 dir = direction;
		float recoil = 0.0f;
		vec2 p;
		if (player.weapon == WEAPON_HAMMER)
		{
			// Static position for hammer
			p = position + vec2(state.attach.x, state.attach.y);
			p.y += data->weapons.id[iw].offsety;
			// if attack is under way, bash stuffs
			if(direction.x < 0)
			{
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
				p.x -= data->weapons.id[iw].offsetx;
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
			}
			draw_sprite(p.x, p.y, data->weapons.id[iw].visual_size);
		}
		else if (player.weapon == WEAPON_NINJA)
		{
			p = position;
			p.y += data->weapons.id[iw].offsety;

			if(direction.x < 0)
			{
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
				p.x -= data->weapons.id[iw].offsetx;
				gameclient.effects->powerupshine(p+vec2(32,0), vec2(32,12));
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
				gameclient.effects->powerupshine(p-vec2(32,0), vec2(32,12));
			}
			draw_sprite(p.x, p.y, data->weapons.id[iw].visual_size);

			// HADOKEN
			if ((client_tick()-player.attacktick) <= (SERVER_TICK_SPEED / 6) && data->weapons.id[iw].num_sprite_muzzles)
			{
				int itex = rand() % data->weapons.id[iw].num_sprite_muzzles;
				float alpha = 1.0f;
				if (alpha > 0.0f && data->weapons.id[iw].sprite_muzzles[itex])
				{
					vec2 dir = vec2(player_char->x,player_char->y) - vec2(prev_char->x, prev_char->y);
					dir = normalize(dir);
					float hadokenangle = get_angle(dir);
					gfx_quads_setrotation(hadokenangle);
					//float offsety = -data->weapons[iw].muzzleoffsety;
					select_sprite(data->weapons.id[iw].sprite_muzzles[itex], 0);
					vec2 diry(-dir.y,dir.x);
					p = position;
					float offsetx = data->weapons.id[iw].muzzleoffsetx;
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
			p = position + dir * data->weapons.id[iw].offsetx - dir*recoil*10.0f;
			p.y += data->weapons.id[iw].offsety;
			draw_sprite(p.x, p.y, data->weapons.id[iw].visual_size);
		}

		if (player.weapon == WEAPON_GUN || player.weapon == WEAPON_SHOTGUN)
		{
			// check if we're firing stuff
			if(data->weapons.id[iw].num_sprite_muzzles)//prev.attackticks)
			{
				float alpha = 0.0f;
				int phase1tick = (client_tick() - player.attacktick);
				if (phase1tick < (data->weapons.id[iw].muzzleduration + 3))
				{
					float t = ((((float)phase1tick) + intratick)/(float)data->weapons.id[iw].muzzleduration);
					alpha = LERP(2.0, 0.0f, min(1.0f,max(0.0f,t)));
				}

				int itex = rand() % data->weapons.id[iw].num_sprite_muzzles;
				if (alpha > 0.0f && data->weapons.id[iw].sprite_muzzles[itex])
				{
					float offsety = -data->weapons.id[iw].muzzleoffsety;
					select_sprite(data->weapons.id[iw].sprite_muzzles[itex], direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
					if(direction.x < 0)
						offsety = -offsety;

					vec2 diry(-dir.y,dir.x);
					vec2 muzzlepos = p + dir * data->weapons.id[iw].muzzleoffsetx + diry * offsety;

					draw_sprite(muzzlepos.x, muzzlepos.y, data->weapons.id[iw].visual_size);
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
		TEE_RENDER_INFO ghost = render_info;
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

	if (gameclient.clients[info.cid].emoticon_start != -1 && gameclient.clients[info.cid].emoticon_start + 2 * client_tickspeed() > client_tick())
	{
		gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
		gfx_quads_begin();

		int since_start = client_tick() - gameclient.clients[info.cid].emoticon_start;
		int from_end = gameclient.clients[info.cid].emoticon_start + 2 * client_tickspeed() - client_tick();

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
		select_sprite(SPRITE_OOP + gameclient.clients[info.cid].emoticon);
		gfx_quads_draw(position.x, position.y - 23 - 32*h, 64, 64*h);
		gfx_quads_end();
	}
}

void PLAYERS::on_render()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// only render active characters
		if(!gameclient.snap.characters[i].active)
			continue;

		const void *prev_info = snap_find_item(SNAP_PREV, NETOBJTYPE_PLAYER_INFO, i);
		const void *info = snap_find_item(SNAP_CURRENT, NETOBJTYPE_PLAYER_INFO, i);

		if(prev_info && info)
		{
			NETOBJ_CHARACTER prev_char = gameclient.snap.characters[i].prev;
			NETOBJ_CHARACTER cur_char = gameclient.snap.characters[i].cur;

			render_player(
					&prev_char,
					&cur_char,
					(const NETOBJ_PLAYER_INFO *)prev_info,
					(const NETOBJ_PLAYER_INFO *)info
				);
		}		
	}
}
