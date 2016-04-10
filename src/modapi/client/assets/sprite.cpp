#include "sprite.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

void CModAPI_Asset_Sprite::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_Sprite::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_ImagePath = CModAPI_AssetPath(pItem->m_ImagePath);
	m_X = pItem->m_X;
	m_Y = pItem->m_Y;
	m_Width = pItem->m_Width;
	m_Height = pItem->m_Height;
}

void CModAPI_Asset_Sprite::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_Sprite::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	Item.m_ImagePath = m_ImagePath.ConvertToInteger();
	Item.m_X = m_X;
	Item.m_Y = m_Y;
	Item.m_Width = m_Width;
	Item.m_Height = m_Height;
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(CModAPI_Asset_Sprite::TypeId), Position, sizeof(CModAPI_Asset_Sprite::CStorageType), &Item);
}

void CModAPI_Asset_Sprite::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}
	
template<>
bool CModAPI_Asset_Sprite::SetValue<int>(int ValueType, int Path, int Value)
{
	switch(ValueType)
	{
		case X:
			m_X = Value;
			return true;
		case Y:
			m_Y = Value;
			return true;
		case WIDTH:
			m_Width = Value;
			return true;
		case HEIGHT:
			m_Height = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<int>(ValueType, Path, Value);
}
	
template<>
bool CModAPI_Asset_Sprite::SetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath Value)
{
	switch(ValueType)
	{
		case IMAGEPATH:
			m_ImagePath = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_AssetPath>(ValueType, Path, Value);
}

template<>
int CModAPI_Asset_Sprite::GetValue(int ValueType, int Path, int DefaultValue)
{
	switch(ValueType)
	{
		case X:
			return m_X;
		case Y:
			return m_Y;
		case WIDTH:
			return m_Width;
		case HEIGHT:
			return m_Height;
		default:
			return CModAPI_Asset::GetValue<int>(ValueType, Path, DefaultValue);
	}
}

template<>
CModAPI_AssetPath CModAPI_Asset_Sprite::GetValue(int ValueType, int Path, CModAPI_AssetPath DefaultValue)
{
	switch(ValueType)
	{
		case IMAGEPATH:
			return m_ImagePath;
		default:
			return CModAPI_Asset::GetValue<CModAPI_AssetPath>(ValueType, Path, DefaultValue);
	}
}
