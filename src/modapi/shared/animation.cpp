#include "animation.h"

void CModAPI_Animation::AddKeyFrame(float Time, vec2 Pos, float Angle, float Opacity)
{
	m_lKeyFrames.set_size(m_lKeyFrames.size()+1);
	CModAPI_AnimationKeyFrame& Frame = m_lKeyFrames[m_lKeyFrames.size()-1];
	
	Frame.m_Time = Time;
	Frame.m_Pos = Pos;
	Frame.m_Angle = Angle;
	Frame.m_Opacity = Opacity;
}

void CModAPI_Animation::GetFrame(float Time, CModAPI_AnimationFrame* pFrame) const
{
	if(m_lKeyFrames.size() == 0)
		return;
	
	int i;
	for(i=0; i<m_lKeyFrames.size(); i++)
	{
		if(m_lKeyFrames[i].m_Time > Time)
			break;
	}
	
	if(i == m_lKeyFrames.size())
	{
		pFrame->m_Pos = m_lKeyFrames[m_lKeyFrames.size()-1].m_Pos;
		pFrame->m_Angle = m_lKeyFrames[m_lKeyFrames.size()-1].m_Angle;
		pFrame->m_Opacity = m_lKeyFrames[m_lKeyFrames.size()-1].m_Opacity;
	}
	else if(i == 0)
	{
		pFrame->m_Pos = m_lKeyFrames[0].m_Pos;
		pFrame->m_Angle = m_lKeyFrames[0].m_Angle;
		pFrame->m_Opacity = m_lKeyFrames[0].m_Opacity;
	}
	else
	{
		float alpha = (Time - m_lKeyFrames[i-1].m_Time) / (m_lKeyFrames[i].m_Time - m_lKeyFrames[i-1].m_Time);
		pFrame->m_Pos = mix(m_lKeyFrames[i-1].m_Pos, m_lKeyFrames[i].m_Pos, alpha);
		pFrame->m_Angle = mix(m_lKeyFrames[i-1].m_Angle, m_lKeyFrames[i].m_Angle, alpha); //Need better interpolation
		pFrame->m_Opacity = clamp(mix(m_lKeyFrames[i-1].m_Opacity, m_lKeyFrames[i].m_Opacity, alpha), 0.0f, 1.0f);
	}
}

float CModAPI_Animation::GetDuration() const
{
	return m_lKeyFrames[m_lKeyFrames.size()-1].m_Time;
}
