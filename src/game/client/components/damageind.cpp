#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/gamecore.hpp> // get_angle
#include <game/client/gc_ui.hpp>
#include <game/client/gc_render.hpp>
#include "damageind.hpp"

DAMAGEIND::DAMAGEIND()
{
	lastupdate = 0;
	num_items = 0;
}

DAMAGEIND::ITEM *DAMAGEIND::create_i()
{
	if (num_items < MAX_ITEMS)
	{
		ITEM *p = &items[num_items];
		num_items++;
		return p;
	}
	return 0;
}

void DAMAGEIND::destroy_i(DAMAGEIND::ITEM *i)
{
	num_items--;
	*i = items[num_items];
}

void DAMAGEIND::create(vec2 pos, vec2 dir)
{
	ITEM *i = create_i();
	if (i)
	{
		i->pos = pos;
		i->life = 0.75f;
		i->dir = dir*-1;
		i->startangle = (( (float)rand()/(float)RAND_MAX) - 1.0f) * 2.0f * pi;
	}
}

void DAMAGEIND::on_render()
{
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	for(int i = 0; i < num_items;)
	{
		vec2 pos = mix(items[i].pos+items[i].dir*75.0f, items[i].pos, clamp((items[i].life-0.60f)/0.15f, 0.0f, 1.0f));

		items[i].life -= client_frametime();
		if(items[i].life < 0.0f)
			destroy_i(&items[i]);
		else
		{
			gfx_setcolor(1.0f,1.0f,1.0f, items[i].life/0.1f);
			gfx_quads_setrotation(items[i].startangle + items[i].life * 2.0f);
			select_sprite(SPRITE_STAR1);
			draw_sprite(pos.x, pos.y, 48.0f);
			i++;
		}
	}
	gfx_quads_end();
}
