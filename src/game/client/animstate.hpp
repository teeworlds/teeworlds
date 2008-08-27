#ifndef GAME_CLIENT_ANIMATION_H
#define GAME_CLIENT_ANIMATION_H

class ANIMSTATE
{
public:
	ANIM_KEYFRAME body;
	ANIM_KEYFRAME back_foot;
	ANIM_KEYFRAME front_foot;
	ANIM_KEYFRAME attach;
	
	void set(ANIMATION *anim, float time);
	void add(ANIMATION *added, float time, float amount);
	
	static ANIMSTATE *get_idle();
};

//void anim_seq_eval(ANIM_SEQUENCE *seq, float time, ANIM_KEYFRAME *frame);
//void anim_eval(ANIMATION *anim, float time, ANIM_STATE *state);
//void anim_add_keyframe(ANIM_KEYFRAME *seq, ANIM_KEYFRAME *added, float amount);
//void anim_add(ANIM_STATE *state, ANIM_STATE *added, float amount);
//void anim_eval_add(ANIM_STATE *state, ANIMATION *anim, float time, float amount);

#endif
