#include "skeleton.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

/* IO *****************************************************************/

void CModAPI_Asset_Skeleton::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem)
{
	// load name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// load info
	m_ParentPath = pItem->m_ParentPath;
	m_DefaultSkinPath = pItem->m_DefaultSkinPath;
	
	// load bones
	const CStorageType::CBone* pBones = static_cast<CStorageType::CBone*>(pAssetsFile->GetData(pItem->m_BonesData));
	for(int i=0; i<pItem->m_NumBones; i++)
	{
		m_Bones.add(CBone());
		CBone& Bone = m_Bones[m_Bones.size()-1];
		
		Bone.m_ParentPath = pBones[i].m_ParentPath;
		Bone.m_Length = pBones[i].m_Length;
		Bone.m_Anchor = pBones[i].m_Anchor;
		Bone.m_Translation = vec2(pBones[i].m_TranslationX, pBones[i].m_TranslationY);
		Bone.m_Scale = vec2(pBones[i].m_ScaleX, pBones[i].m_ScaleY);
		Bone.m_Angle = pBones[i].m_Angle;
		str_copy(Bone.m_aName, pBones[i].m_aName, sizeof(Bone.m_aName));
		Bone.m_Color = ModAPI_IntToColor(pBones[i].m_Color);
	}
	
	// load layers
	const CStorageType::CLayer* pLayers = static_cast<CStorageType::CLayer*>(pAssetsFile->GetData(pItem->m_LayersData));
	for(int i=0; i<pItem->m_NumLayers; i++)
	{
		m_Layers.add(CLayer());
		CLayer& Layer = m_Layers[m_Layers.size()-1];
		
		str_copy(Layer.m_aName, pLayers[i].m_aName, sizeof(Layer.m_aName));
	}
}

void CModAPI_Asset_Skeleton::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);

	Item.m_ParentPath = m_ParentPath.ConvertToInteger();
	Item.m_DefaultSkinPath = m_DefaultSkinPath.ConvertToInteger();
	
	{
		CStorageType::CBone* pBones = new CStorageType::CBone[m_Bones.size()];
		for(int i=0; i<m_Bones.size(); i++)
		{
			pBones[i].m_ParentPath = m_Bones[i].m_ParentPath.ConvertToInteger();
			pBones[i].m_Length = m_Bones[i].m_Length;
			pBones[i].m_Anchor = m_Bones[i].m_Anchor;
			pBones[i].m_TranslationX = m_Bones[i].m_Translation.x;
			pBones[i].m_TranslationY = m_Bones[i].m_Translation.y;
			pBones[i].m_ScaleX = m_Bones[i].m_Scale.x;
			pBones[i].m_ScaleY = m_Bones[i].m_Scale.y;
			pBones[i].m_Angle = m_Bones[i].m_Angle;
			str_copy(pBones[i].m_aName, m_Bones[i].m_aName, sizeof(pBones[i].m_aName));
			pBones[i].m_Color = ModAPI_ColorToInt(m_Bones[i].m_Color);
		}
		Item.m_NumBones = m_Bones.size();
		Item.m_BonesData = pFileWriter->AddData(Item.m_NumBones * sizeof(CStorageType::CBone), pBones);
		delete[] pBones;
	}
	
	{
		CStorageType::CLayer* pLayers = new CStorageType::CLayer[m_Layers.size()];
		for(int i=0; i<m_Layers.size(); i++)
		{
			str_copy(pLayers[i].m_aName, m_Layers[i].m_aName, sizeof(pLayers[i].m_aName));
		}
		Item.m_NumLayers = m_Layers.size();
		Item.m_LayersData = pFileWriter->AddData(Item.m_NumLayers * sizeof(CStorageType::CLayer), pLayers);
		delete[] pLayers;
	}
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(TypeId), Position, sizeof(CStorageType), &Item);
}

void CModAPI_Asset_Skeleton::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

/* IMPLEMENTATION *****************************************************/
	
void CModAPI_Asset_Skeleton::AddBone(const CModAPI_Asset_Skeleton::CBone& Bone)
{
	m_Bones.add(Bone);
}
	
CModAPI_Asset_Skeleton::CBone& CModAPI_Asset_Skeleton::AddBone()
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "bone%d", m_Bones.size());
	m_Bones.add(CModAPI_Asset_Skeleton::CBone());
	return m_Bones[m_Bones.size()-1].Name(aBuf);
}
	
CModAPI_Asset_Skeleton::CLayer& CModAPI_Asset_Skeleton::AddLayer()
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "layer%d", m_Layers.size());
	m_Layers.add(CModAPI_Asset_Skeleton::CLayer());
	return m_Layers[m_Layers.size()-1].Name(aBuf);
}

/* VALUE FLOAT ********************************************************/

template<>
float CModAPI_Asset_Skeleton::GetValue<float>(int ValueType, int PathInt, float DefaultValue)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_LENGTH:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Length;
			else
				return DefaultValue;
		case BONE_ANCHOR:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Anchor;
			else
				return DefaultValue;
		case BONE_TRANSLATION_X:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Translation.x;
			else
				return DefaultValue;
		case BONE_TRANSLATION_Y:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Translation.y;
			else
				return DefaultValue;
		case BONE_SCALE_X:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Scale.x;
			else
				return DefaultValue;
		case BONE_SCALE_Y:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Scale.y;
			else
				return DefaultValue;
		case BONE_ANGLE:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Angle;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<float>(ValueType, PathInt, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_Skeleton::SetValue<float>(int ValueType, int PathInt, float Value)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_LENGTH:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Length = Value;
				return true;
			}
			else return false;
		case BONE_ANCHOR:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Anchor = Value;
				return true;
			}
			else return false;
		case BONE_TRANSLATION_X:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Translation.x = Value;
				return true;
			}
			else return false;
		case BONE_TRANSLATION_Y:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Translation.y = Value;
				return true;
			}
			else return false;
		case BONE_SCALE_X:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Scale.x = Value;
				return true;
			}
			else return false;
		case BONE_SCALE_Y:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Scale.y = Value;
				return true;
			}
			else return false;
		case BONE_ANGLE:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Angle = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<float>(ValueType, PathInt, Value);
}

/* VALUE VEC4 *********************************************************/
	
template<>
vec4 CModAPI_Asset_Skeleton::GetValue<vec4>(int ValueType, int PathInt, vec4 DefaultValue)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_COLOR:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_Color;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<vec4>(ValueType, PathInt, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_Skeleton::SetValue<vec4>(int ValueType, int PathInt, vec4 Value)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_COLOR:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_Color = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<vec4>(ValueType, PathInt, Value);
}

/* VALUE STRING *******************************************************/

template<>
char* CModAPI_Asset_Skeleton::GetValue(int ValueType, int PathInt, char* DefaultValue)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_NAME:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_aName;
			else
				return DefaultValue;
		case LAYER_NAME:
			if(Path.GetType() == CSubPath::TYPE_LAYER && Path.GetId() >= 0 && Path.GetId() < m_Layers.size())
				return m_Layers[Path.GetId()].m_aName;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<char*>(ValueType, PathInt, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_Skeleton::SetValue<const char*>(int ValueType, int PathInt, const char* pText)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_NAME:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				str_copy(m_Bones[Path.GetId()].m_aName, pText, sizeof(m_Bones[Path.GetId()].m_aName));
				return true;
			}
			else return false;
		case LAYER_NAME:
			if(Path.GetType() == CSubPath::TYPE_LAYER && Path.GetId() >= 0 && Path.GetId() < m_Layers.size())
			{
				str_copy(m_Layers[Path.GetId()].m_aName, pText, sizeof(m_Layers[Path.GetId()].m_aName));
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<const char*>(ValueType, PathInt, pText);
}

/* VALUE ASSETPATH ****************************************************/
	
template<>
CModAPI_AssetPath CModAPI_Asset_Skeleton::GetValue<CModAPI_AssetPath>(int ValueType, int PathInt, CModAPI_AssetPath DefaultValue)
{
	switch(ValueType)
	{
		case PARENTPATH:
			return m_ParentPath;
		case SKINPATH:
			return m_DefaultSkinPath;
		default:
			return CModAPI_Asset::GetValue<CModAPI_AssetPath>(ValueType, PathInt, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_Skeleton::SetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath Value)
{
	switch(ValueType)
	{
		case PARENTPATH:
			m_ParentPath = Value;
			return true;
		case SKINPATH:
			m_DefaultSkinPath = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_AssetPath>(ValueType, Path, Value);
}

/* VALUE BONEPATH *****************************************************/
	
template<>
CModAPI_Asset_Skeleton::CBonePath CModAPI_Asset_Skeleton::GetValue<CModAPI_Asset_Skeleton::CBonePath>(int ValueType, int PathInt, CModAPI_Asset_Skeleton::CBonePath DefaultValue)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_PARENT:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
				return m_Bones[Path.GetId()].m_ParentPath;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<CModAPI_Asset_Skeleton::CBonePath>(ValueType, PathInt, DefaultValue);
	}
}

template<>
bool CModAPI_Asset_Skeleton::SetValue<CModAPI_Asset_Skeleton::CBonePath>(int ValueType, int PathInt, CModAPI_Asset_Skeleton::CBonePath Value)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case BONE_PARENT:
			if(Path.GetType() == CSubPath::TYPE_BONE && Path.GetId() >= 0 && Path.GetId() < m_Bones.size())
			{
				m_Bones[Path.GetId()].m_ParentPath = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_Asset_Skeleton::CBonePath>(ValueType, PathInt, Value);
}
