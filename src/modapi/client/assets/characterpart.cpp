#include "characterpart.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>
#include <modapi/client/assets/character.h>

/* IO *****************************************************************/

void CModAPI_Asset_CharacterPart::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem)
{
	// load name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
	
	m_CharacterPath = CModAPI_AssetPath(pItem->m_CharacterPath);
	m_CharacterPart = CModAPI_Asset_Character::CSubPath(pItem->m_CharacterPart);
	m_SkeletonSkinPath = CModAPI_AssetPath(pItem->m_SkeletonSkinPath);
}

void CModAPI_Asset_CharacterPart::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	
	Item.m_CharacterPath = m_CharacterPath.ConvertToInteger();
	Item.m_CharacterPart = m_CharacterPart.ConvertToInteger();
	Item.m_SkeletonSkinPath = m_SkeletonSkinPath.ConvertToInteger();
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(TypeId), Position, sizeof(CStorageType), &Item);
}

void CModAPI_Asset_CharacterPart::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

/* VALUE INT **********************************************************/

template<>
int CModAPI_Asset_CharacterPart::GetValue(int ValueType, int Path, int DefaultValue)
{
	switch(ValueType)
	{
		case CHARACTERPART:
			return m_CharacterPart.ConvertToInteger();
		default:
			return CModAPI_Asset::GetValue<int>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_CharacterPart::SetValue<int>(int ValueType, int Path, int Value)
{
	switch(ValueType)
	{
		case CHARACTERPART:
			m_CharacterPart = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<int>(ValueType, Path, Value);
}

/* VALUE ASSETPATH *******************************************************/

template<>
CModAPI_AssetPath CModAPI_Asset_CharacterPart::GetValue(int ValueType, int Path, CModAPI_AssetPath DefaultValue)
{
	switch(ValueType)
	{
		case CHARACTERPATH:
			return m_CharacterPath;
		case SKELETONSKINPATH:
			return m_SkeletonSkinPath;
		default:
			return CModAPI_Asset::GetValue<CModAPI_AssetPath>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_CharacterPart::SetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath Value)
{
	switch(ValueType)
	{
		case CHARACTERPATH:
			m_CharacterPath = Value;
			return true;
		case SKELETONSKINPATH:
			m_SkeletonSkinPath = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_AssetPath>(ValueType, Path, Value);
}
