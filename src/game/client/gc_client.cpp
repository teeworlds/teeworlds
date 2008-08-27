/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/math.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
	#include <engine/e_client_interface.h>
};

#include "../gamecore.hpp"
#include "../version.hpp"
#include "../layers.hpp"
#include "gc_map_image.hpp"
#include "../generated/gc_data.hpp"
#include "gc_ui.hpp"
#include "gc_client.hpp"
#include "gc_render.hpp"

#include "components/skins.hpp"
#include "components/damageind.hpp"
#include "gameclient.hpp"

TUNING_PARAMS tuning;

void snd_play_random(int chn, int setid, float vol, vec2 pos)
{
	SOUNDSET *set = &data->sounds[setid];

	if(!set->num_sounds)
		return;

	if(set->num_sounds == 1)
	{
		snd_play_at(chn, set->sounds[0].id, 0, pos.x, pos.y);
		return;
	}

	// play a random one
	int id;
	do {
		id = rand() % set->num_sounds;
	} while(id == set->last);
	snd_play_at(chn, set->sounds[id].id, 0, pos.x, pos.y);
	set->last = id;
}
