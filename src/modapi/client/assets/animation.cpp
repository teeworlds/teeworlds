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

void CModAPI_Asset_Animation::AddKeyFrame(const CModAPI_Asset_Animation::CKeyFrame& Frame)
{
	int i = GetKeyFrameId(Frame.m_Time);
		
	if(i == m_lKeyFrames.size())
		m_lKeyFrames.add(Frame);
	else
		m_lKeyFrames.insertat(Frame, i);
}

CModAPI_Asset_Animation::CKeyFrame& CModAPI_Asset_Animation::AddKeyFrame(float Time)
{
	int i = GetKeyFrameId(Time);
		
	if(i == m_lKeyFrames.size())
		m_lKeyFrames.add(CModAPI_Asset_Animation::CKeyFrame());
	else
		m_lKeyFrames.insertat(CModAPI_Asset_Animation::CKeyFrame(), i);
	
	m_lKeyFrames[i].m_Time = Time;
	
	return m_lKeyFrames[i];
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
			*pFrame = CModAPI_Asset_Animation::CFrame();
		}
		else
		{
			*pFrame = m_lKeyFrames[m_lKeyFrames.size()-1];
		}
	}
	else if(i == 0)
	{
		*pFrame = m_lKeyFrames[0];
	}
	else
	{
		float alpha = (CycleTime - m_lKeyFrames[i-1].m_Time) / (m_lKeyFrames[i].m_Time - m_lKeyFrames[i-1].m_Time);
		pFrame->m_Pos = mix(m_lKeyFrames[i-1].m_Pos, m_lKeyFrames[i].m_Pos, alpha);
		pFrame->m_Size = mix(m_lKeyFrames[i-1].m_Size, m_lKeyFrames[i].m_Size, alpha);
		pFrame->m_Angle = mix(m_lKeyFrames[i-1].m_Angle, m_lKeyFrames[i].m_Angle, alpha); //Need better interpolation
		pFrame->m_Color.r = clamp(mix(m_lKeyFrames[i-1].m_Color.r, m_lKeyFrames[i].m_Color.r, alpha), 0.0f, 1.0f);
		pFrame->m_Color.g = clamp(mix(m_lKeyFrames[i-1].m_Color.g, m_lKeyFrames[i].m_Color.g, alpha), 0.0f, 1.0f);
		pFrame->m_Color.b = clamp(mix(m_lKeyFrames[i-1].m_Color.b, m_lKeyFrames[i].m_Color.b, alpha), 0.0f, 1.0f);
		pFrame->m_Color.a = clamp(mix(m_lKeyFrames[i-1].m_Color.a, m_lKeyFrames[i].m_Color.a, alpha), 0.0f, 1.0f);
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

void CModAPI_Asset_Animation::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_Animation::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_CycleType = pItem->m_CycleType;
	
	const CModAPI_Asset_Animation::CKeyFrame* pFrames = static_cast<CModAPI_Asset_Animation::CKeyFrame*>(pAssetsFile->GetData(pItem->m_KeyFrameData));
	for(int f=0; f<pItem->m_NumKeyFrame; f++)
	{
		AddKeyFrame(pFrames[f]);
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

void CModAPI_Asset_Animation::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

