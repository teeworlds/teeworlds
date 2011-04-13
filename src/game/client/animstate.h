/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_ANIMSTATE_H
#define GAME_CLIENT_ANIMSTATE_H

class CAnimState
{
	ANIM_KEYFRAME m_Body;
	ANIM_KEYFRAME m_BackFoot;
	ANIM_KEYFRAME m_FrontFoot;
	ANIM_KEYFRAME m_Attach;

public:
	ANIM_KEYFRAME *GetBody() { return &m_Body; };
	ANIM_KEYFRAME *GetBackFoot() { return &m_BackFoot; };
	ANIM_KEYFRAME *GetFrontFoot() { return &m_FrontFoot; };
	ANIM_KEYFRAME *GetAttach() { return &m_Attach; };
	void Set(ANIMATION *pAnim, float Time);
	void Add(ANIMATION *pAdded, float Time, float Amount);

	static CAnimState *GetIdle();
};

#endif
