#include "animation.h"

#include <engine/shared/datafile.h>

int CModAPI_Asset_Animation::GetKeyFrameId(float Time) const
{
	if(m_lKeyFrames.size() == 0)
		return 0;
	
	int i;
	for(i=0; i<m_lKeyFrames.size(); i++)
	{
		if(m_lKeyFrames[i].m_Time > Time)
			break;
	}
	
	return i;
}

void CModAPI_Asset_Animation::AddKeyFrame(CModAPI_Asset_Animation::CKeyFrame& Frame)
{
	int i = GetKeyFrameId(Frame.m_Time);
		
	if(i == m_lKeyFrames.size())
		m_lKeyFrames.add(Frame);
	else
		m_lKeyFrames.insertat(Frame, i);
}

void CModAPI_Asset_Animation::AddKeyFrame(float Time, vec2 Pos, float Angle, float Opacity, int ListId)
{
	CModAPI_Asset_Animation::CKeyFrame Frame;
	Frame.m_Time = Time;
	Frame.m_Pos = Pos;
	Frame.m_Angle = Angle;
	Frame.m_Opacity = Opacity;
	Frame.m_ListId = ListId;
		
	AddKeyFrame(Frame);
}

void CModAPI_Asset_Animation::GetFrame(float Time, CModAPI_Asset_Animation::CFrame* pFrame) const
{
	float CycleTime = Time;
	if(m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP)
	{
		CycleTime = fmod(Time, GetDuration());
	}
	
	int i = GetKeyFrameId(CycleTime);
	
	if(i == m_lKeyFrames.size())
	{
		if(i == 0)
		{
			pFrame->m_Pos = vec2(0.0f, 0.0f);
			pFrame->m_Angle = 0.0f;
			pFrame->m_Opacity = 1.0f;
			pFrame->m_ListId = 0;
		}
		else
		{
			pFrame->m_Pos = m_lKeyFrames[m_lKeyFrames.size()-1].m_Pos;
			pFrame->m_Angle = m_lKeyFrames[m_lKeyFrames.size()-1].m_Angle;
			pFrame->m_Opacity = m_lKeyFrames[m_lKeyFrames.size()-1].m_Opacity;
			pFrame->m_ListId = m_lKeyFrames[m_lKeyFrames.size()-1].m_ListId;
		}
	}
	else if(i == 0)
	{
		pFrame->m_Pos = m_lKeyFrames[0].m_Pos;
		pFrame->m_Angle = m_lKeyFrames[0].m_Angle;
		pFrame->m_Opacity = m_lKeyFrames[0].m_Opacity;
		pFrame->m_ListId = m_lKeyFrames[0].m_ListId;
	}
	else
	{
		float alpha = (CycleTime - m_lKeyFrames[i-1].m_Time) / (m_lKeyFrames[i].m_Time - m_lKeyFrames[i-1].m_Time);
		pFrame->m_Pos = mix(m_lKeyFrames[i-1].m_Pos, m_lKeyFrames[i].m_Pos, alpha);
		pFrame->m_Angle = mix(m_lKeyFrames[i-1].m_Angle, m_lKeyFrames[i].m_Angle, alpha); //Need better interpolation
		pFrame->m_Opacity = clamp(mix(m_lKeyFrames[i-1].m_Opacity, m_lKeyFrames[i].m_Opacity, alpha), 0.0f, 1.0f);
		pFrame->m_ListId = m_lKeyFrames[i-1].m_ListId;
	}
}

float CModAPI_Asset_Animation::GetDuration() const
{
	return m_lKeyFrames[m_lKeyFrames.size()-1].m_Time;
}

void CModAPI_Asset_Animation::MoveDownKeyFrame(int Id)
{
	if(Id >= m_lKeyFrames.size() || Id <= 0)
		return;
	
	CModAPI_Asset_Animation::CKeyFrame TmpFrame = m_lKeyFrames[Id-1];
	m_lKeyFrames[Id-1] = m_lKeyFrames[Id];
	m_lKeyFrames[Id] = TmpFrame;
	
	float TmpTime = m_lKeyFrames[Id-1].m_Time;
	m_lKeyFrames[Id-1].m_Time = m_lKeyFrames[Id].m_Time;
	m_lKeyFrames[Id].m_Time = TmpTime;
}

void CModAPI_Asset_Animation::MoveUpKeyFrame(int Id)
{
	if(Id >= m_lKeyFrames.size() -1 || Id < 0)
		return;
	
	CModAPI_Asset_Animation::CKeyFrame TmpFrame = m_lKeyFrames[Id+1];
	m_lKeyFrames[Id+1] = m_lKeyFrames[Id];
	m_lKeyFrames[Id] = TmpFrame;
	
	float TmpTime = m_lKeyFrames[Id+1].m_Time;
	m_lKeyFrames[Id+1].m_Time = m_lKeyFrames[Id].m_Time;
	m_lKeyFrames[Id].m_Time = TmpTime;
}

void CModAPI_Asset_Animation::DuplicateKeyFrame(int Id)
{
	if(Id >= 0 && Id < m_lKeyFrames.size())
	{
		CModAPI_Asset_Animation::CKeyFrame Frame = m_lKeyFrames[Id];
		if(Id == m_lKeyFrames.size()-1)
			Frame.m_Time += 0.1;
		else
			Frame.m_Time = (m_lKeyFrames[Id].m_Time + m_lKeyFrames[Id+1].m_Time)/2.0f;
		
		m_lKeyFrames.insertat(Frame, Id+1);
	}
}

void CModAPI_Asset_Animation::DeleteKeyFrame(int Id)
{
	if(Id >= 0 && Id < m_lKeyFrames.size())
	{
		m_lKeyFrames.remove_index(Id);
	}
}

void CModAPI_Asset_Animation::InitFromAssetsFile(CModAPI_Client_Graphics* pModAPIGraphics, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_Animation::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_CycleType = pItem->m_CycleType;
	
	const CModAPI_Asset_Animation::CKeyFrame* pFrames = static_cast<CModAPI_Asset_Animation::CKeyFrame*>(pAssetsFile->GetData(pItem->m_KeyFrameData));
	for(int f=0; f<pItem->m_NumKeyFrame; f++)
	{
		AddKeyFrame(pFrames[f].m_Time, pFrames[f].m_Pos, pFrames[f].m_Angle, pFrames[f].m_Opacity);
	}
}

void CModAPI_Asset_Animation::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_Animation::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	Item.m_CycleType = m_CycleType;
	Item.m_NumKeyFrame = m_lKeyFrames.size();
	Item.m_KeyFrameData = pFileWriter->AddData(Item.m_NumKeyFrame * sizeof(CModAPI_Asset_Animation::CKeyFrame), m_lKeyFrames.base_ptr());
	pFileWriter->AddItem(CModAPI_Asset_Animation::TypeId, Position, sizeof(CModAPI_Asset_Animation::CStorageType), &Item);
}

void CModAPI_Asset_Animation::Unload(class CModAPI_Client_Graphics* pModAPIGraphics)
{
	
}

