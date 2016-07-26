#include "linestyle.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

void CModAPI_Asset_LineStyle::AddElement(const CModAPI_Asset_LineStyle::CElement& Elem)
{
	m_Elements.add(Elem);
}

void CModAPI_Asset_LineStyle::AddElement(CModAPI_AssetPath SpritePath, int Size, CModAPI_AssetPath AnimationPath, float StartPosition, float EndPosition, int TilingType, int TileSpacing)
{
	CModAPI_Asset_LineStyle::CElement Elem;
	Elem.m_SpritePath = SpritePath;
	Elem.m_Size = Size;
	Elem.m_AnimationPath = AnimationPath;
	Elem.m_StartPosition = StartPosition;
	Elem.m_EndPosition = EndPosition;
	Elem.m_TilingType = TilingType;
	Elem.m_TileSpacing = TileSpacing;
	
	AddElement(Elem);
}

void CModAPI_Asset_LineStyle::MoveDownElement(int Id)
{
	if(Id >= m_Elements.size() || Id <= 0)
		return;
	
	CModAPI_Asset_LineStyle::CElement TmpElem = m_Elements[Id-1];
	m_Elements[Id-1] = m_Elements[Id];
	m_Elements[Id] = TmpElem;
}

void CModAPI_Asset_LineStyle::MoveUpElement(int Id)
{
	if(Id >= m_Elements.size() -1 || Id < 0)
		return;
	
	CModAPI_Asset_LineStyle::CElement TmpElem = m_Elements[Id+1];
	m_Elements[Id+1] = m_Elements[Id];
	m_Elements[Id] = TmpElem;
}

void CModAPI_Asset_LineStyle::DeleteElement(int Id)
{
	if(Id >= 0 && Id < m_Elements.size())
	{
		m_Elements.remove_index(Id);
	}
}

void CModAPI_Asset_LineStyle::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_LineStyle::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info	
	const CModAPI_Asset_LineStyle::CElement* pElements = static_cast<CModAPI_Asset_LineStyle::CElement*>(pAssetsFile->GetData(pItem->m_ElementsData));
	for(int i=0; i<pItem->m_NumElements; i++)
	{
		AddElement(pElements[i]);
	}
}
		
void CModAPI_Asset_LineStyle::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_LineStyle::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	
	Item.m_NumElements = m_Elements.size();
	Item.m_ElementsData = pFileWriter->AddData(Item.m_NumElements * sizeof(CModAPI_Asset_LineStyle::CElement), m_Elements.base_ptr());
	
	pFileWriter->AddItem(CModAPI_Asset_LineStyle::TypeId, Position, sizeof(CModAPI_Asset_LineStyle::CStorageType), &Item);
}

void CModAPI_Asset_LineStyle::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}
