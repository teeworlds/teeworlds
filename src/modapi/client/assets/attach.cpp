#include "attach.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

void CModAPI_Asset_Attach::AddBackElement(const CModAPI_Asset_Attach::CElement& Elem)
{
	m_BackElements.add(Elem);
}

void CModAPI_Asset_Attach::AddBackElement(CModAPI_AssetPath SpritePath, int Size, CModAPI_AssetPath AnimationPath, int Alignment, int OffsetX, int OffsetY)
{
	CModAPI_Asset_Attach::CElement Elem;
	Elem.m_SpritePath = SpritePath;
	Elem.m_Size = Size;
	Elem.m_AnimationPath = AnimationPath;
	Elem.m_Alignment = Alignment;
	Elem.m_OffsetX = OffsetX;
	Elem.m_OffsetY = OffsetY;
	
	AddBackElement(Elem);
}

void CModAPI_Asset_Attach::MoveDownBackElement(int Id)
{
	if(Id >= m_BackElements.size() || Id <= 0)
		return;
	
	CModAPI_Asset_Attach::CElement TmpElem = m_BackElements[Id-1];
	m_BackElements[Id-1] = m_BackElements[Id];
	m_BackElements[Id] = TmpElem;
}

void CModAPI_Asset_Attach::MoveUpBackElement(int Id)
{
	if(Id >= m_BackElements.size() -1 || Id < 0)
		return;
	
	CModAPI_Asset_Attach::CElement TmpElem = m_BackElements[Id+1];
	m_BackElements[Id+1] = m_BackElements[Id];
	m_BackElements[Id] = TmpElem;
}

void CModAPI_Asset_Attach::DeleteBackElement(int Id)
{
	if(Id >= 0 && Id < m_BackElements.size())
	{
		m_BackElements.remove_index(Id);
	}
}

void CModAPI_Asset_Attach::InitFromAssetsFile(CModAPI_Client_Graphics* pModAPIGraphics, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_Attach::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_TeeAnimationPath = CModAPI_AssetPath::FromAssetsFile(pItem->m_TeeAnimationPath);
	m_CursorPath = CModAPI_AssetPath::FromAssetsFile(pItem->m_CursorPath);
	
	const CModAPI_Asset_Attach::CElement* pBackElements = static_cast<CModAPI_Asset_Attach::CElement*>(pAssetsFile->GetData(pItem->m_BackElementsData));
	for(int i=0; i<pItem->m_NumBackElements; i++)
	{
		AddBackElement(pBackElements[i]);
	}
}

void CModAPI_Asset_Attach::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_Attach::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	
	Item.m_TeeAnimationPath = m_TeeAnimationPath.ToAssetsFile();
	Item.m_CursorPath = m_CursorPath.ToAssetsFile();
	
	Item.m_NumBackElements = m_BackElements.size();
	Item.m_BackElementsData = pFileWriter->AddData(Item.m_NumBackElements * sizeof(CModAPI_Asset_Attach::CElement), m_BackElements.base_ptr());
	
	pFileWriter->AddItem(CModAPI_Asset_Attach::TypeId, Position, sizeof(CModAPI_Asset_Attach::CStorageType), &Item);
}

void CModAPI_Asset_Attach::Unload(class CModAPI_Client_Graphics* pModAPIGraphics)
{
	
}
