#include "skeletonskin.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

/* IO *****************************************************************/

void CModAPI_Asset_SkeletonSkin::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_SkeletonSkin::CStorageType* pItem)
{
	// load name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// load info
	m_SkeletonPath = pItem->m_SkeletonPath;
	
	// load sprites
	const CStorageType::CSprite* pSprites = static_cast<CStorageType::CSprite*>(pAssetsFile->GetData(pItem->m_SpritesData));
	for(int i=0; i<pItem->m_NumSprites; i++)
	{
		m_Sprites.add(CSprite());
		CSprite& Sprite = m_Sprites[m_Sprites.size()-1];
		
		Sprite.m_SpritePath = pSprites[i].m_SpritePath;
		Sprite.m_BonePath = pSprites[i].m_BonePath;
		Sprite.m_LayerPath = pSprites[i].m_LayerPath;
		Sprite.m_Translation = vec2(pSprites[i].m_TranslationX, pSprites[i].m_TranslationY);
		Sprite.m_Scale = vec2(pSprites[i].m_ScaleX, pSprites[i].m_ScaleY);
		Sprite.m_Angle = pSprites[i].m_Angle;
		Sprite.m_Anchor = pSprites[i].m_Anchor;
		Sprite.m_Color = ModAPI_IntToColor(pSprites[i].m_Color);
		Sprite.m_Alignment = pSprites[i].m_Alignment;
	}
}

void CModAPI_Asset_SkeletonSkin::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);

	Item.m_SkeletonPath = m_SkeletonPath.ConvertToInteger();
	
	//save sprites
	{
		CStorageType::CSprite* pSprites = new CStorageType::CSprite[m_Sprites.size()];
		for(int i=0; i<m_Sprites.size(); i++)
		{
			pSprites[i].m_SpritePath = m_Sprites[i].m_SpritePath.ConvertToInteger();
			pSprites[i].m_BonePath = m_Sprites[i].m_BonePath.ConvertToInteger();
			pSprites[i].m_LayerPath = m_Sprites[i].m_LayerPath.ConvertToInteger();
			pSprites[i].m_TranslationX = m_Sprites[i].m_Translation.x;
			pSprites[i].m_TranslationY = m_Sprites[i].m_Translation.y;
			pSprites[i].m_ScaleX = m_Sprites[i].m_Scale.x;
			pSprites[i].m_ScaleY = m_Sprites[i].m_Scale.y;
			pSprites[i].m_Angle = m_Sprites[i].m_Angle;
			pSprites[i].m_Anchor = m_Sprites[i].m_Anchor;
			pSprites[i].m_Color = ModAPI_ColorToInt(m_Sprites[i].m_Color);
			pSprites[i].m_Alignment = m_Sprites[i].m_Alignment;
		}
		Item.m_NumSprites = m_Sprites.size();
		Item.m_SpritesData = pFileWriter->AddData(Item.m_NumSprites * sizeof(CStorageType::CSprite), pSprites);
		delete[] pSprites;
	}
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(TypeId), Position, sizeof(CStorageType), &Item);
}

void CModAPI_Asset_SkeletonSkin::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

/* IMPLEMENTATION *****************************************************/
	
void CModAPI_Asset_SkeletonSkin::AddSprite(const CModAPI_Asset_SkeletonSkin::CSprite& Sprite)
{
	m_Sprites.add(Sprite);
}
	
CModAPI_Asset_SkeletonSkin::CSprite& CModAPI_Asset_SkeletonSkin::AddSprite(
	CModAPI_AssetPath SkeletonPath,
	CModAPI_Asset_Skeleton::CBonePath BonePath,
	CModAPI_Asset_Skeleton::CBonePath LayerPath)
{
	m_Sprites.add(CModAPI_Asset_SkeletonSkin::CSprite(SkeletonPath));
	return m_Sprites[m_Sprites.size()-1].Bone(BonePath).Layer(LayerPath);
}

/* VALUE INT **********************************************************/
	
template<>
int CModAPI_Asset_SkeletonSkin::GetValue<int>(int ValueType, int Path, int DefaultValue)
{
	switch(ValueType)
	{
		case SPRITE_ALIGNMENT:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Alignment;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<int>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_SkeletonSkin::SetValue<int>(int ValueType, int Path, int Value)
{
	switch(ValueType)
	{
		case SPRITE_ALIGNMENT:
			if(Path >= 0 && Path < m_Sprites.size() && Value >= -1)
			{
				m_Sprites[Path].m_Alignment = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<int>(ValueType, Path, Value);
}

/* VALUE FLOAT ********************************************************/

template<>
float CModAPI_Asset_SkeletonSkin::GetValue<float>(int ValueType, int Path, float DefaultValue)
{
	switch(ValueType)
	{
		case SPRITE_TRANSLATION_X:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Translation.x;
			else
				return DefaultValue;
		case SPRITE_TRANSLATION_Y:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Translation.y;
			else
				return DefaultValue;
		case SPRITE_SCALE_X:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Scale.x;
			else
				return DefaultValue;
		case SPRITE_SCALE_Y:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Scale.y;
			else
				return DefaultValue;
		case SPRITE_ANGLE:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Angle;
			else
				return DefaultValue;
		case SPRITE_ANCHOR:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_Anchor;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<float>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_SkeletonSkin::SetValue<float>(int ValueType, int Path, float Value)
{
	switch(ValueType)
	{
		case SPRITE_TRANSLATION_X:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_Translation.x = Value;
				return true;
			}
			else return false;
		case SPRITE_TRANSLATION_Y:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_Translation.y = Value;
				return true;
			}
			else return false;
		case SPRITE_SCALE_X:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_Scale.x = Value;
				return true;
			}
			else return false;
		case SPRITE_SCALE_Y:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_Scale.y = Value;
				return true;
			}
			else return false;
		case SPRITE_ANGLE:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_Angle = Value;
				return true;
			}
			else return false;
		case SPRITE_ANCHOR:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_Anchor = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<float>(ValueType, Path, Value);
}

/* VALUE ASSETPATH ****************************************************/

template<>
CModAPI_AssetPath CModAPI_Asset_SkeletonSkin::GetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath DefaultValue)
{
	switch(ValueType)
	{
		case SKELETONPATH:
			return m_SkeletonPath;
		case SPRITE_PATH:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_SpritePath;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<CModAPI_AssetPath>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_SkeletonSkin::SetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath Value)
{
	switch(ValueType)
	{
		case SKELETONPATH:
			m_SkeletonPath = Value;
			return true;
		case SPRITE_PATH:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_SpritePath = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_AssetPath>(ValueType, Path, Value);
}

/* VALUE BONEPATH *****************************************************/
	
template<>
CModAPI_Asset_Skeleton::CBonePath CModAPI_Asset_SkeletonSkin::GetValue<CModAPI_Asset_Skeleton::CBonePath>(int ValueType, int Path, CModAPI_Asset_Skeleton::CBonePath DefaultValue)
{
	switch(ValueType)
	{
		case SPRITE_BONE:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_BonePath;
			else
				return DefaultValue;
		case SPRITE_LAYER:
			if(Path >= 0 && Path < m_Sprites.size())
				return m_Sprites[Path].m_LayerPath;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<CModAPI_Asset_Skeleton::CBonePath>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_SkeletonSkin::SetValue<CModAPI_Asset_Skeleton::CBonePath>(int ValueType, int Path, CModAPI_Asset_Skeleton::CBonePath Value)
{
	switch(ValueType)
	{
		case SPRITE_BONE:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_BonePath = Value;
				return true;
			}
			else return false;
		case SPRITE_LAYER:
			if(Path >= 0 && Path < m_Sprites.size())
			{
				m_Sprites[Path].m_LayerPath = Value;
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_Asset_Skeleton::CBonePath>(ValueType, Path, Value);
}
