#include "character.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

/* IO *****************************************************************/

void CModAPI_Asset_Character::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem)
{
	// load name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
	
	// load parts
	const CStorageType::CPart* pParts = static_cast<CStorageType::CPart*>(pAssetsFile->GetData(pItem->m_PartsData));
	for(int i=0; i<pItem->m_NumParts; i++)
	{
		m_Parts.add(CPart());
		CPart& Part = m_Parts[m_Parts.size()-1];
		str_copy(Part.m_aName, pParts[i].m_aName, sizeof(Part.m_aName));
	}
}

void CModAPI_Asset_Character::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	
	{
		CStorageType::CPart* pParts = new CStorageType::CPart[m_Parts.size()];
		for(int i=0; i<m_Parts.size(); i++)
		{
			str_copy(pParts[i].m_aName, m_Parts[i].m_aName, sizeof(pParts[i].m_aName));
		}
		Item.m_NumParts = m_Parts.size();
		Item.m_PartsData = pFileWriter->AddData(Item.m_NumParts * sizeof(CStorageType::CPart), pParts);
		delete[] pParts;
	}
	
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(TypeId), Position, sizeof(CStorageType), &Item);
}

void CModAPI_Asset_Character::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}

/* IMPLEMENTATION *****************************************************/

CModAPI_Asset_Character::CPart& CModAPI_Asset_Character::AddPart()
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "part%d", m_Parts.size());
	m_Parts.add(CModAPI_Asset_Character::CPart());
	return m_Parts[m_Parts.size()-1].Name(aBuf);
}

/* VALUE STRING *******************************************************/

template<>
char* CModAPI_Asset_Character::GetValue(int ValueType, int PathInt, char* DefaultValue)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case PART_NAME:
			if(Path.GetType() == CSubPath::TYPE_PART && Path.GetId() >= 0 && Path.GetId() < m_Parts.size())
				return m_Parts[Path.GetId()].m_aName;
			else
				return DefaultValue;
		default:
			return CModAPI_Asset::GetValue<char*>(ValueType, PathInt, DefaultValue);
	}
}
	
template<>
bool CModAPI_Asset_Character::SetValue<const char*>(int ValueType, int PathInt, const char* pText)
{
	CSubPath Path(PathInt);
	switch(ValueType)
	{
		case PART_NAME:
			if(Path.GetType() == CSubPath::TYPE_PART && Path.GetId() >= 0 && Path.GetId() < m_Parts.size())
			{
				str_copy(m_Parts[Path.GetId()].m_aName, pText, sizeof(m_Parts[Path.GetId()].m_aName));
				return true;
			}
			else return false;
	}
	
	return CModAPI_Asset::SetValue<const char*>(ValueType, PathInt, pText);
}
