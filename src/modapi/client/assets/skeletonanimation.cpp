#include "skeleton.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

/* IO *****************************************************************/

void CModAPI_Asset_SkeletonAnimation::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_SkeletonPath = pItem->m_SkeletonPath;
	
	// load bone animations
	const CStorageType::CBoneKeyFrame* pBoneKeyFrames = static_cast<CStorageType::CBoneKeyFrame*>(pAssetsFile->GetData(pItem->m_BoneKeyFramesData));
	const CStorageType::CBoneAnimation* pBoneAnimations = static_cast<CStorageType::CBoneAnimation*>(pAssetsFile->GetData(pItem->m_BoneAnimationsData));
	
	for(int i=0; i<pItem->m_NumBoneAnimations; i++)
	{
		m_BoneAnimations.add(CBoneAnimation());
		CBoneAnimation& Animation = m_BoneAnimations[m_BoneAnimations.size()-1];
		Animation.m_BonePath = pBoneAnimations[i].m_BonePath;
		Animation.m_CycleType = pBoneAnimations[i].m_CycleType;
		
		if(pBoneAnimations[i].m_FirstKeyFrame < pItem->m_NumBoneKeyFrames)
		{
			int NumKeyFrames = min(pItem->m_NumBoneKeyFrames - pBoneAnimations[i].m_FirstKeyFrame, pBoneAnimations[i].m_NumKeyFrames);
			for(int j=0; j<NumKeyFrames; j++)
			{
				Animation.m_KeyFrames.add(CBoneAnimation::CKeyFrame());
				CBoneAnimation::CKeyFrame& KeyFrame = Animation.m_KeyFrames[Animation.m_KeyFrames.size()-1];
				
				int KeyId = pBoneAnimations[i].m_FirstKeyFrame+j;
				KeyFrame.m_Time = pBoneKeyFrames[KeyId].m_Time;
				KeyFrame.m_Translation = vec2(pBoneKeyFrames[KeyId].m_TranslationX, pBoneKeyFrames[KeyId].m_TranslationY);
				KeyFrame.m_Scale = vec2(pBoneKeyFrames[KeyId].m_ScaleX, pBoneKeyFrames[KeyId].m_ScaleY);
				KeyFrame.m_Angle = pBoneKeyFrames[KeyId].m_Angle;
				KeyFrame.m_Alignment = pBoneKeyFrames[KeyId].m_Alignment;
			}
		}
	}
	
	// load layer animations
	const CStorageType::CLayerKeyFrame* pLayerKeyFrames = static_cast<CStorageType::CLayerKeyFrame*>(pAssetsFile->GetData(pItem->m_LayerKeyFramesData));
	const CStorageType::CLayerAnimation* pLayerAnimations = static_cast<CStorageType::CLayerAnimation*>(pAssetsFile->GetData(pItem->m_LayerAnimationsData));
	for(int i=0; i<pItem->m_NumLayerAnimations; i++)
	{
		m_LayerAnimations.add(CLayerAnimation());
		CLayerAnimation& Animation = m_LayerAnimations[m_LayerAnimations.size()-1];
		
		Animation.m_LayerPath = pLayerAnimations[i].m_LayerPath;
		Animation.m_CycleType = pLayerAnimations[i].m_CycleType;
		
		if(pLayerAnimations[i].m_FirstKeyFrame < pItem->m_NumLayerKeyFrames)
		{
			int NumKeyFrames = min(pItem->m_NumLayerKeyFrames - pLayerAnimations[i].m_FirstKeyFrame, pLayerAnimations[i].m_NumKeyFrames);
			for(int j=0; j<NumKeyFrames; j++)
			{
				Animation.m_KeyFrames.add(CLayerAnimation::CKeyFrame());
				CLayerAnimation::CKeyFrame& KeyFrame = Animation.m_KeyFrames[Animation.m_KeyFrames.size()-1];
				
				int KeyId = pLayerAnimations[i].m_FirstKeyFrame+j;
				KeyFrame.m_Time = pBoneKeyFrames[KeyId].m_Time;
				KeyFrame.m_Color = ModAPI_IntToColor(pLayerKeyFrames[KeyId].m_Color);
				KeyFrame.m_State = pLayerKeyFrames[KeyId].m_State;
			}
		}
	}
}

void CModAPI_Asset_SkeletonAnimation::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);

	Item.m_SkeletonPath = m_SkeletonPath.ConvertToInteger();
	
	{
		int KeyFramesIter = 0;
		for(int i=0; i<m_BoneAnimations.size(); i++)
		{
			KeyFramesIter += m_BoneAnimations[i].m_KeyFrames.size();
		}
		
		Item.m_NumBoneKeyFrames = KeyFramesIter;
		Item.m_NumBoneAnimations = m_BoneAnimations.size();
		
		CStorageType::CBoneKeyFrame* pKeyFrames = new CStorageType::CBoneKeyFrame[Item.m_NumBoneKeyFrames];
		CStorageType::CBoneAnimation* pAnimations = new CStorageType::CBoneAnimation[Item.m_NumBoneAnimations];
		
		KeyFramesIter = 0;
		for(int i=0; i<m_BoneAnimations.size(); i++)
		{
			pAnimations[i].m_BonePath = m_BoneAnimations[i].m_BonePath.ConvertToInteger();
			pAnimations[i].m_CycleType = m_BoneAnimations[i].m_CycleType;
			
			pAnimations[i].m_FirstKeyFrame = KeyFramesIter;
			pAnimations[i].m_NumKeyFrames = m_BoneAnimations[i].m_KeyFrames.size();
			
			for(int j=0; j<m_BoneAnimations[i].m_KeyFrames.size(); j++)
			{
				pKeyFrames[KeyFramesIter].m_Time = m_BoneAnimations[i].m_KeyFrames[j].m_Time;
				pKeyFrames[KeyFramesIter].m_TranslationX = m_BoneAnimations[i].m_KeyFrames[j].m_Translation.x;
				pKeyFrames[KeyFramesIter].m_TranslationY = m_BoneAnimations[i].m_KeyFrames[j].m_Translation.y;
				pKeyFrames[KeyFramesIter].m_ScaleX = m_BoneAnimations[i].m_KeyFrames[j].m_Scale.x;
				pKeyFrames[KeyFramesIter].m_ScaleY = m_BoneAnimations[i].m_KeyFrames[j].m_Scale.y;
				pKeyFrames[KeyFramesIter].m_Angle = m_BoneAnimations[i].m_KeyFrames[j].m_Angle;
				pKeyFrames[KeyFramesIter].m_Alignment = m_BoneAnimations[i].m_KeyFrames[j].m_Alignment;
				
				KeyFramesIter++;
			}
		}
		
		Item.m_BoneKeyFramesData = pFileWriter->AddData(Item.m_NumBoneKeyFrames * sizeof(CStorageType::CBoneKeyFrame), pKeyFrames);
		Item.m_BoneAnimationsData = pFileWriter->AddData(Item.m_NumBoneAnimations * sizeof(CStorageType::CBoneAnimation), pAnimations);
		
		delete[] pKeyFrames;
		delete[] pAnimations;
	}
	
	{
		int KeyFramesIter = 0;
		for(int i=0; i<m_LayerAnimations.size(); i++)
		{
			KeyFramesIter += m_LayerAnimations[i].m_KeyFrames.size();
		}
		
		Item.m_NumLayerKeyFrames = KeyFramesIter;
		Item.m_NumLayerAnimations = m_LayerAnimations.size();
		
		CStorageType::CLayerKeyFrame* pKeyFrames = new CStorageType::CLayerKeyFrame[Item.m_NumLayerKeyFrames];
		CStorageType::CLayerAnimation* pAnimations = new CStorageType::CLayerAnimation[Item.m_NumLayerAnimations];
		
		KeyFramesIter = 0;
		for(int i=0; i<m_LayerAnimations.size(); i++)
		{
			pAnimations[i].m_LayerPath = m_LayerAnimations[i].m_LayerPath.ConvertToInteger();
			pAnimations[i].m_CycleType = m_LayerAnimations[i].m_CycleType;
			
			pAnimations[i].m_FirstKeyFrame = KeyFramesIter;
			pAnimations[i].m_NumKeyFrames = m_LayerAnimations[i].m_KeyFrames.size();
			
			for(int j=0; j<m_LayerAnimations[i].m_KeyFrames.size(); j++)
			{
				pKeyFrames[KeyFramesIter].m_Time = m_LayerAnimations[i].m_KeyFrames[j].m_Time;
				pKeyFrames[KeyFramesIter].m_Color = ModAPI_ColorToInt(m_LayerAnimations[i].m_KeyFrames[j].m_Color);
				pKeyFrames[KeyFramesIter].m_State = m_LayerAnimations[i].m_KeyFrames[j].m_State;
				
				KeyFramesIter++;
			}
		}
		
		Item.m_LayerKeyFramesData = pFileWriter->AddData(Item.m_NumLayerKeyFrames * sizeof(CStorageType::CLayerKeyFrame), pKeyFrames);
		Item.m_LayerAnimationsData = pFileWriter->AddData(Item.m_NumLayerAnimations * sizeof(CStorageType::CLayerAnimation), pAnimations);
		
		delete[] pKeyFrames;
		delete[] pAnimations;
	}
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(TypeId), Position, sizeof(CStorageType), &Item);
}

void CModAPI_Asset_SkeletonAnimation::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

/* VALUE INT **********************************************************/

template<>
int CModAPI_Asset_SkeletonAnimation::GetValue<int>(int ValueType, int Path, int DefaultValue)
{
	CModAPI_Asset_SkeletonAnimation::CSubPath SubPath(Path);
	
	switch(ValueType)
	{
		case BONEANIMATION_CYCLETYPE:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEANIMATION)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					return m_BoneAnimations[animId].m_CycleType;
				}
			}
			break;
		case BONEKEYFRAME_TIME:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_BoneAnimations[animId].m_KeyFrames.size())
					{
						return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Time;
					}
				}
			}
			break;
		case BONEKEYFRAME_ALIGNMENT:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_BoneAnimations[animId].m_KeyFrames.size())
					{
						return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Alignment;
					}
				}
			}
			break;
		case LAYERKEYFRAME_TIME:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_LayerAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_LayerAnimations[animId].m_KeyFrames.size())
					{
						return m_LayerAnimations[animId].m_KeyFrames[keyId].m_Time;
					}
				}
			}
			break;
		case LAYERKEYFRAME_STATE:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_LayerAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_LayerAnimations[animId].m_KeyFrames.size())
					{
						return m_LayerAnimations[animId].m_KeyFrames[keyId].m_State;
					}
				}
			}
			break;
		default:
			return CModAPI_Asset::GetValue<int>(ValueType, Path, DefaultValue);
	}
	
	return DefaultValue;
}

template<>
bool CModAPI_Asset_SkeletonAnimation::SetValue<int>(int ValueType, int Path, int Value)
{
	CModAPI_Asset_SkeletonAnimation::CSubPath SubPath(Path);
	
	switch(ValueType)
	{
		case BONEANIMATION_CYCLETYPE:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEANIMATION)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					m_BoneAnimations[animId].m_CycleType = Value;
					return true;
				}
				return false;
			}
			else return false;
		case BONEKEYFRAME_TIME:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId < 0 || animId >= m_BoneAnimations.size())
					return false;
					
				int keyId = SubPath.GetId2();
				if(keyId < 0 || keyId >= m_BoneAnimations[animId].m_KeyFrames.size())
					return false;
				
				if(
					(keyId > 0 && Value > m_BoneAnimations[animId].m_KeyFrames[keyId-1].m_Time) &&
					(keyId+1 < m_BoneAnimations[animId].m_KeyFrames.size() && Value < m_BoneAnimations[animId].m_KeyFrames[keyId+1].m_Time)
				)
				{
					m_BoneAnimations[animId].m_KeyFrames[keyId].m_Time = Value;
					return true;
				}
				else
				{
					for(int i=0; i<m_BoneAnimations[animId].m_KeyFrames.size(); i++)
					{
						if(m_BoneAnimations[animId].m_KeyFrames[i].m_Time == Value)
							return false;
					}
					
					CModAPI_Asset_SkeletonAnimation::CBoneAnimation::CKeyFrame KeyFrame = m_BoneAnimations[animId].m_KeyFrames[keyId];
					KeyFrame.m_Time = Value;
					m_BoneAnimations[animId].m_KeyFrames.remove_index(keyId);
					
					keyId = m_BoneAnimations[animId].IntTimeToKeyFrame(Value);
					if(keyId == m_BoneAnimations[animId].m_KeyFrames.size())
					{
						m_BoneAnimations[animId].m_KeyFrames.add(KeyFrame);
					}
					else
					{
						m_BoneAnimations[animId].m_KeyFrames.insertat(KeyFrame, keyId);
					}
					
					return true;
				}
			}
			else return false;
		case BONEKEYFRAME_ALIGNMENT:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
			{
				if(Value >= NUM_BONEALIGNS)
					return false;
					
				int animId = SubPath.GetId();
				if(animId < 0 || animId >= m_BoneAnimations.size())
					return false;
					
				int keyId = SubPath.GetId2();
				if(keyId < 0 || keyId >= m_BoneAnimations[animId].m_KeyFrames.size())
					return false;
				
				m_BoneAnimations[animId].m_KeyFrames[keyId].m_Alignment = Value;
			}
			else return false;
		case LAYERKEYFRAME_TIME:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId < 0 || animId >= m_LayerAnimations.size())
					return false;
					
				int keyId = SubPath.GetId2();
				if(keyId < 0 || keyId >= m_LayerAnimations[animId].m_KeyFrames.size())
					return false;
				
				if(
					(keyId > 0 && Value > m_LayerAnimations[animId].m_KeyFrames[keyId-1].m_Time) &&
					(keyId+1 < m_LayerAnimations[animId].m_KeyFrames.size() && Value < m_LayerAnimations[animId].m_KeyFrames[keyId+1].m_Time)
				)
				{
					m_LayerAnimations[animId].m_KeyFrames[keyId].m_Time = Value;
					return true;
				}
				else
				{
					for(int i=0; i<m_LayerAnimations[animId].m_KeyFrames.size(); i++)
					{
						if(m_LayerAnimations[animId].m_KeyFrames[i].m_Time == Value)
							return false;
					}
					
					CModAPI_Asset_SkeletonAnimation::CLayerAnimation::CKeyFrame KeyFrame = m_LayerAnimations[animId].m_KeyFrames[keyId];
					KeyFrame.m_Time = Value;
					m_LayerAnimations[animId].m_KeyFrames.remove_index(keyId);
					
					keyId = m_LayerAnimations[animId].IntTimeToKeyFrame(Value);
					if(keyId == m_LayerAnimations[animId].m_KeyFrames.size())
					{
						m_LayerAnimations[animId].m_KeyFrames.add(KeyFrame);
					}
					else
					{
						m_LayerAnimations[animId].m_KeyFrames.insertat(KeyFrame, keyId);
					}
					
					return true;
				}
			}
			else return false;
		case LAYERKEYFRAME_STATE:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
			{
				if(Value >= NUM_LAYERSTATES)
					return false;
					
				int animId = SubPath.GetId();
				if(animId < 0 || animId >= m_LayerAnimations.size())
					return false;
					
				int keyId = SubPath.GetId2();
				if(keyId < 0 || keyId >= m_LayerAnimations[animId].m_KeyFrames.size())
					return false;
				
				m_LayerAnimations[animId].m_KeyFrames[keyId].m_State = Value;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<int>(ValueType, Path, Value);
}

/* VALUE FLOAT ********************************************************/

template<>
float CModAPI_Asset_SkeletonAnimation::GetValue<float>(int ValueType, int Path, float DefaultValue)
{
	CModAPI_Asset_SkeletonAnimation::CSubPath SubPath(Path);
	
	switch(ValueType)
	{
		case BONEKEYFRAME_ANGLE:
		case BONEKEYFRAME_TRANSLATION_X:
		case BONEKEYFRAME_TRANSLATION_Y:
		case BONEKEYFRAME_SCALE_X:
		case BONEKEYFRAME_SCALE_Y:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_BoneAnimations[animId].m_KeyFrames.size())
					{
						switch(ValueType)
						{
							case BONEKEYFRAME_ANGLE:
								return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Angle;
							case BONEKEYFRAME_TRANSLATION_X:
								return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Translation.x;
							case BONEKEYFRAME_TRANSLATION_Y:
								return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Translation.y;
							case BONEKEYFRAME_SCALE_X:
								return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Scale.x;
							case BONEKEYFRAME_SCALE_Y:
								return m_BoneAnimations[animId].m_KeyFrames[keyId].m_Scale.y;
						}
					}
				}
			}
			break;
		default:
			return CModAPI_Asset::GetValue<float>(ValueType, Path, DefaultValue);
	}
	
	return DefaultValue;
}

template<>
bool CModAPI_Asset_SkeletonAnimation::SetValue<float>(int ValueType, int Path, float Value)
{
	CModAPI_Asset_SkeletonAnimation::CSubPath SubPath(Path);
	
	switch(ValueType)
	{
		case BONEKEYFRAME_ANGLE:
		case BONEKEYFRAME_TRANSLATION_X:
		case BONEKEYFRAME_TRANSLATION_Y:
		case BONEKEYFRAME_SCALE_X:
		case BONEKEYFRAME_SCALE_Y:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_BoneAnimations[animId].m_KeyFrames.size())
					{
						switch(ValueType)
						{
							case BONEKEYFRAME_ANGLE:
								m_BoneAnimations[animId].m_KeyFrames[keyId].m_Angle = Value;
								return true;
							case BONEKEYFRAME_TRANSLATION_X:
								m_BoneAnimations[animId].m_KeyFrames[keyId].m_Translation.x = Value;
								return true;
							case BONEKEYFRAME_TRANSLATION_Y:
								m_BoneAnimations[animId].m_KeyFrames[keyId].m_Translation.y = Value;
								return true;
							case BONEKEYFRAME_SCALE_X:
								m_BoneAnimations[animId].m_KeyFrames[keyId].m_Scale.x = Value;
								return true;
							case BONEKEYFRAME_SCALE_Y:
								m_BoneAnimations[animId].m_KeyFrames[keyId].m_Scale.y = Value;
								return true;
						}
					}
					else return false;
				}
				else return false;
			}
			break;
	}
	
	return CModAPI_Asset::SetValue<int>(ValueType, Path, Value);
}

/* VALUE VEC4 *********************************************************/
	
template<>
vec4 CModAPI_Asset_SkeletonAnimation::GetValue<vec4>(int ValueType, int PathInt, vec4 DefaultValue)
{
	CModAPI_Asset_SkeletonAnimation::CSubPath SubPath(PathInt);
	switch(ValueType)
	{
		case LAYERKEYFRAME_COLOR:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_LayerAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_LayerAnimations[animId].m_KeyFrames.size())
					{
						return m_LayerAnimations[animId].m_KeyFrames[keyId].m_Color;
					}
				}
			}
		default:
			return CModAPI_Asset::GetValue<vec4>(ValueType, PathInt, DefaultValue);
	}
	
	return DefaultValue;
}
	
template<>
bool CModAPI_Asset_SkeletonAnimation::SetValue<vec4>(int ValueType, int PathInt, vec4 Value)
{
	CModAPI_Asset_SkeletonAnimation::CSubPath SubPath(PathInt);
	switch(ValueType)
	{
		case LAYERKEYFRAME_COLOR:
			if(SubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
			{
				int animId = SubPath.GetId();
				if(animId >= 0 && animId < m_LayerAnimations.size())
				{
					int keyId = SubPath.GetId2();
					if(keyId >= 0 && keyId < m_LayerAnimations[animId].m_KeyFrames.size())
					{
						m_LayerAnimations[animId].m_KeyFrames[keyId].m_Color = Value;
					}
					else return false;
				}
				else return false;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<vec4>(ValueType, PathInt, Value);
}

/* VALUE ASSETPATH ****************************************************/

template<>
CModAPI_AssetPath CModAPI_Asset_SkeletonAnimation::GetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath DefaultValue)
{
	switch(ValueType)
	{
		case SKELETONPATH:
			return m_SkeletonPath;
	}
	
	return CModAPI_Asset::GetValue<CModAPI_AssetPath>(ValueType, Path, DefaultValue);
}

template<>
bool CModAPI_Asset_SkeletonAnimation::SetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath Value)
{
	switch(ValueType)
	{
		case SKELETONPATH:
			m_SkeletonPath = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_AssetPath>(ValueType, Path, Value);
}
