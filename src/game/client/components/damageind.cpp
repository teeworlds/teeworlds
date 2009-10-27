#include <engine/e_client_interface.h>
#include <engine/client/graphics.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/gamecore.hpp> // get_angle
#include <game/client/ui.hpp>
#include <game/client/render.hpp>
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
	Graphics()->TextureSet(data->images[IMAGE_GAME].id);
	Graphics()->QuadsBegin();
	for(int i = 0; i < num_items;)
	{
		vec2 pos = mix(items[i].pos+items[i].dir*75.0f, items[i].pos, clamp((items[i].life-0.60f)/0.15f, 0.0f, 1.0f));

		items[i].life -= client_frametime();
		if(items[i].life < 0.0f)
			destroy_i(&items[i]);
		else
		{
			Graphics()->SetColor(1.0f,1.0f,1.0f, items[i].life/0.1f);
			Graphics()->QuadsSetRotation(items[i].startangle + items[i].life * 2.0f);
			RenderTools()->select_sprite(SPRITE_STAR1);
			RenderTools()->draw_sprite(pos.x, pos.y, 48.0f);
			i++;
		}
	}
	Graphics()->QuadsEnd();
}
