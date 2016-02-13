#include "list.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

void CModAPI_Asset_List::InitFromAssetsFile(CModAPI_Client_Graphics* pModAPIGraphics, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_List::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
	
	const CModAPI_AssetPath* pElements = static_cast<CModAPI_AssetPath*>(pAssetsFile->GetData(pItem->m_ElementsData));
	m_Elements.set_size(pItem->m_NumElements);
	for(int i=0; i<pItem->m_NumElements; i++)
	{
		m_Elements[i] = pElements[i];
	}
}

void CModAPI_Asset_List::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position, int ElementType)
{
	CModAPI_Asset_List::CStorageType Item;
	
	Item.m_NumElements = m_Elements.size();
	Item.m_ElementsData = pFileWriter->AddData(Item.m_NumElements * sizeof(CModAPI_AssetPath), m_Elements.base_ptr());
	
	pFileWriter->AddItem(ElementType, Position, sizeof(CModAPI_Asset_List::CStorageType), &Item);
}

void CModAPI_Asset_List::Unload(class CModAPI_Client_Graphics* pModAPIGraphics)
{
	
}
