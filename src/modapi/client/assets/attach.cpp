#include "attach.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

/* IO *****************************************************************/

void CModAPI_Asset_Attach::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem)
{
	// load name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
	
	m_CharacterPath = CModAPI_AssetPath(pItem->m_CharacterPath);
}

void CModAPI_Asset_Attach::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	
	Item.m_CharacterPath = m_CharacterPath.ConvertToInteger();
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(TypeId), Position, sizeof(CStorageType), &Item);
}

void CModAPI_Asset_Attach::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

/* VALUE ASSETPATH *******************************************************/

template<>
CModAPI_AssetPath CModAPI_Asset_Attach::GetValue(int ValueType, int Path, CModAPI_AssetPath DefaultValue)
{
	switch(ValueType)
	{
		case CHARACTERPATH:
			return m_CharacterPath;
		default:
			return CModAPI_Asset::GetValue<CModAPI_AssetPath>(ValueType, Path, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_Attach::SetValue<CModAPI_AssetPath>(int ValueType, int Path, CModAPI_AssetPath Value)
{
	switch(ValueType)
	{
		case CHARACTERPATH:
			m_CharacterPath = Value;
			return true;
	}
	
	return CModAPI_Asset::SetValue<CModAPI_AssetPath>(ValueType, Path, Value);
}


