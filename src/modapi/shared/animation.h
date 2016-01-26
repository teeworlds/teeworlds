#ifndef MODAPI_CLIENT_ANIMATION_H
#define MODAPI_CLIENT_ANIMATION_H

#include <base/tl/array.h>
#include <base/vmath.h>

struct CModAPI_AnimationFrame
{
	float m_Time;
	vec2 m_Pos;
	float m_Angle;
	float m_Opacity;
};
	
class CModAPI_Animation
{
private:
	array<CModAPI_AnimationFrame> m_lKeyFrames;
	int m_CycleType; //MODAPI_ANIMCYCLETYPE_XXXXX

public:
	void AddKeyFrame(float Time, vec2 Pos, float Angle, float Opacity);
	void GetFrame(float Time, CModAPI_AnimationFrame* pFrame) const;
	float GetDuration() const;
};

class CModAPI_AnimationState
{
private:
	CModAPI_Animation* m_pAnimation;
	float m_Duration;
	CModAPI_AnimationFrame m_Frame;
	
public:
	CModAPI_AnimationState();
	
	void SetAnimation(CModAPI_Animation* pAnimation, float Duration);
	void SetTime(float Time);
	
	vec2 GetPos() const;
	float GetAngle() const;
	float GetOpacity() const;
};

#endif
