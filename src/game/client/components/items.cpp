#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/gamecore.hpp> // get_angle
#include <game/client/gameclient.hpp>
#include <game/client/ui.hpp>
#include <game/client/gc_render.hpp>

#include <game/client/components/flow.hpp>
#include <game/client/components/effects.hpp>

#include "items.hpp"

void ITEMS::render_projectile(const NETOBJ_PROJECTILE *current, int itemid)
{

	// get positions
	float curvature = 0;
	float speed = 0;
	if(current->type == WEAPON_GRENADE)
	{
		curvature = gameclient.tuning.grenade_curvature;
		speed = gameclient.tuning.grenade_speed;
	}
	else if(current->type == WEAPON_SHOTGUN)
	{
		curvature = gameclient.tuning.shotgun_curvature;
		speed = gameclient.tuning.shotgun_speed;
	}
	else if(current->type == WEAPON_GUN)
	{
		curvature = gameclient.tuning.gun_curvature;
		speed = gameclient.tuning.gun_speed;
	}

	float ct = (client_prevtick()-current->start_tick)/(float)SERVER_TICK_SPEED + client_ticktime();
	if(ct < 0)
		return; // projectile havn't been shot yet
		
	vec2 startpos(current->x, current->y);
	vec2 startvel(current->vx/100.0f, current->vy/100.0f);
	vec2 pos = calc_pos(startpos, startvel, curvature, speed, ct);
	vec2 prevpos = calc_pos(startpos, startvel, curvature, speed, ct-0.001f);


	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	
	select_sprite(data->weapons.id[clamp(current->type, 0, NUM_WEAPONS-1)].sprite_proj);
	vec2 vel = pos-prevpos;
	//vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	

	// add particle for this projectile
	if(current->type == WEAPON_GRENADE)
	{
		gameclient.effects->smoketrail(pos, vel*-1);
		gameclient.flow->add(pos, vel*1000*client_frametime(), 10.0f);
		gfx_quads_setrotation(client_localtime()*pi*2*2 + itemid);
	}
	else
	{
		gameclient.effects->bullettrail(pos);
		gameclient.flow->add(pos, vel*1000*client_frametime(), 10.0f);

		if(length(vel) > 0.00001f)
			gfx_quads_setrotation(get_angle(vel));
		else
			gfx_quads_setrotation(0);

	}

	gfx_quads_draw(pos.x, pos.y, 32, 32);
	gfx_quads_setrotation(0);
	gfx_quads_end();
}

void ITEMS::render_pickup(const NETOBJ_PICKUP *prev, const NETOBJ_PICKUP *current)
{
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	float angle = 0.0f;
	float size = 64.0f;
	if (current->type == POWERUP_WEAPON)
	{
		angle = 0; //-pi/6;//-0.25f * pi * 2.0f;
		select_sprite(data->weapons.id[clamp(current->subtype, 0, NUM_WEAPONS-1)].sprite_body);
		size = data->weapons.id[clamp(current->subtype, 0, NUM_WEAPONS-1)].visual_size;
	}
	else
	{
		const int c[] = {
			SPRITE_PICKUP_HEALTH,
			SPRITE_PICKUP_ARMOR,
			SPRITE_PICKUP_WEAPON,
			SPRITE_PICKUP_NINJA
			};
		select_sprite(c[current->type]);

		if(c[current->type] == SPRITE_PICKUP_NINJA)
		{
			gameclient.effects->powerupshine(pos, vec2(96,18));
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

void ITEMS::render_flag(const NETOBJ_FLAG *prev, const NETOBJ_FLAG *current)
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
	if(gameclient.snap.local_info && current->carried_by == gameclient.snap.local_info->cid)
		pos = gameclient.local_character_pos;

	gfx_quads_draw(pos.x, pos.y-size*0.75f, size, size*2);
	gfx_quads_end();
}


void ITEMS::render_laser(const struct NETOBJ_LASER *current)
{
	vec2 pos = vec2(current->x, current->y);
	vec2 from = vec2(current->from_x, current->from_y);
	vec2 dir = normalize(pos-from);

	float ticks = client_tick() + client_intratick() - current->start_tick;
	float ms = (ticks/50.0f) * 1000.0f;
	float a =  ms / gameclient.tuning.laser_bounce_delay;
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

void ITEMS::on_render()
{
	int num = snap_num_items(SNAP_CURRENT);
	for(int i = 0; i < num; i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PROJECTILE)
		{
			render_projectile((const NETOBJ_PROJECTILE *)data, item.id);
		}
		else if(item.type == NETOBJTYPE_PICKUP)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_pickup((const NETOBJ_PICKUP *)prev, (const NETOBJ_PICKUP *)data);
		}
		else if(item.type == NETOBJTYPE_LASER)
		{
			render_laser((const NETOBJ_LASER *)data);
		}
		else if(item.type == NETOBJTYPE_FLAG)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if (prev)
				render_flag((const NETOBJ_FLAG *)prev, (const NETOBJ_FLAG *)data);
		}
	}

	// render extra projectiles
	/*
	for(int i = 0; i < extraproj_num; i++)
	{
		if(extraproj_projectiles[i].start_tick < client_tick())
		{
			extraproj_projectiles[i] = extraproj_projectiles[extraproj_num-1];
			extraproj_num--;
		}
		else
			render_projectile(&extraproj_projectiles[i], 0);
	}*/
}
