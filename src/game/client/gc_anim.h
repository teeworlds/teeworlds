
struct animstate
{
	keyframe body;
	keyframe back_foot;
	keyframe front_foot;
	keyframe attach;
};

void anim_seq_eval(sequence *seq, float time, keyframe *frame);
void anim_eval(animation *anim, float time, animstate *state);
void anim_add_keyframe(keyframe *seq, keyframe *added, float amount);
void anim_add(animstate *state, animstate *added, float amount);
void anim_eval_add(animstate *state, animation *anim, float time, float amount);
