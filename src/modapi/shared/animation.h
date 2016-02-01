#ifndef MODAPI_CLIENT_ANIMATION_H
#define MODAPI_CLIENT_ANIMATION_H

#include <base/tl/array.h>
#include <base/vmath.h>

struct CModAPI_AnimationFrame
{
	vec2 m_Pos;
	float m_Angle;
	float m_Opacity;
};

struct CModAPI_AnimationKeyFrame : CModAPI_AnimationFrame
{
	float m_Time;
};
	
class CModAPI_Animation
{
private:
	array<CModAPI_AnimationKeyFrame> m_lKeyFrames;
	int m_CycleType; //MODAPI_ANIMCYCLETYPE_XXXXX

public:
	void AddKeyFrame(float Time, vec2 Pos, float Angle, float Opacity);
	void GetFrame(float Time, CModAPI_AnimationFrame* pFrame) const;
	float GetDuration() const;
};

#endif
