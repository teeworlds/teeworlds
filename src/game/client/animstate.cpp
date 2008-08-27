
#include <base/math.hpp>
#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include "animstate.hpp"

static void anim_seq_eval(ANIM_SEQUENCE *seq, float time, ANIM_KEYFRAME *frame)
{
	if(seq->num_frames == 0)
	{
		frame->time = 0;
		frame->x = 0;
		frame->y = 0;
		frame->angle = 0;
	}
	else if(seq->num_frames == 1)
	{
		*frame = seq->frames[0];
	}
	else
	{
		//time = max(0.0f, min(1.0f, time / duration)); // TODO: use clamp
		ANIM_KEYFRAME *frame1 = 0;
		ANIM_KEYFRAME *frame2 = 0;
		float blend = 0.0f;

		// TODO: make this smarter.. binary search
		for (int i = 1; i < seq->num_frames; i++)
		{
			if (seq->frames[i-1].time <= time && seq->frames[i].time >= time)
			{
				frame1 = &seq->frames[i-1];
				frame2 = &seq->frames[i];
				blend = (time - frame1->time) / (frame2->time - frame1->time);
				break;
			}
		}

		if (frame1 && frame2)
		{
			frame->time = time;
			frame->x = mix(frame1->x, frame2->x, blend);
			frame->y = mix(frame1->y, frame2->y, blend);
			frame->angle = mix(frame1->angle, frame2->angle, blend);
		}
	}
}

static void anim_add_keyframe(ANIM_KEYFRAME *seq, ANIM_KEYFRAME *added, float amount)
{
	seq->x += added->x*amount;
	seq->y += added->y*amount;
	seq->angle += added->angle*amount;
}

static void anim_add(ANIMSTATE *state, ANIMSTATE *added, float amount)
{
	anim_add_keyframe(&state->body, &added->body, amount);
	anim_add_keyframe(&state->back_foot, &added->back_foot, amount);
	anim_add_keyframe(&state->front_foot, &added->front_foot, amount);
	anim_add_keyframe(&state->attach, &added->attach, amount);
}


void ANIMSTATE::set(ANIMATION *anim, float time)
{
	anim_seq_eval(&anim->body, time, &body);
	anim_seq_eval(&anim->back_foot, time, &back_foot);
	anim_seq_eval(&anim->front_foot, time, &front_foot);
	anim_seq_eval(&anim->attach, time, &attach);
}

void ANIMSTATE::add(ANIMATION *anim, float time, float amount)
{
	ANIMSTATE add;
	add.set(anim, time);
	anim_add(this, &add, amount);
}

ANIMSTATE *ANIMSTATE::get_idle()
{
	static ANIMSTATE state;
	static bool init = true;
	
	if(init)
	{
		state.set(&data->animations[ANIM_BASE], 0);
		state.add(&data->animations[ANIM_IDLE], 0, 1.0f);
		init = false;
	}
	
	return &state;
}
