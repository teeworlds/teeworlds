#include <engine/e_client_interface.h>
#include <game/generated/gc_data.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/components/camera.hpp>
#include "sounds.hpp"

void SOUNDS::on_init()
{
	// setup sound channels
	snd_set_channel(SOUNDS::CHN_GUI, 1.0f, 0.0f);
	snd_set_channel(SOUNDS::CHN_MUSIC, 1.0f, 0.0f);
	snd_set_channel(SOUNDS::CHN_WORLD, 0.9f, 1.0f);
	snd_set_channel(SOUNDS::CHN_GLOBAL, 1.0f, 0.0f);

	snd_set_listener_pos(0.0f, 0.0f);
}

void SOUNDS::on_render()
{
	// set listner pos
	snd_set_listener_pos(gameclient.camera->center.x, gameclient.camera->center.y);
}

void SOUNDS::play_and_record(int chn, int setid, float vol, vec2 pos)
{
	NETMSG_SV_SOUNDGLOBAL msg;
	msg.soundid = setid;
	msg.pack(MSGFLAG_NOSEND|MSGFLAG_RECORD);
	client_send_msg();
	
	play(chn, setid, vol, pos);
}

void SOUNDS::play(int chn, int setid, float vol, vec2 pos)
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
