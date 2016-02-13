#include "sprite.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

void CModAPI_Asset_Sprite::InitFromAssetsFile(CModAPI_Client_Graphics* pModAPIGraphics, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_Sprite::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_ImagePath = CModAPI_AssetPath::FromAssetsFile(pItem->m_ImagePath);
	m_X = pItem->m_X;
	m_Y = pItem->m_Y;
	m_Width = pItem->m_Width;
	m_Height = pItem->m_Height;
}

void CModAPI_Asset_Sprite::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_Sprite::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	Item.m_ImagePath = m_ImagePath.ToAssetsFile();
	Item.m_X = m_X;
	Item.m_Y = m_Y;
	Item.m_Width = m_Width;
	Item.m_Height = m_Height;
	
	pFileWriter->AddItem(CModAPI_Asset_Sprite::TypeId, Position, sizeof(CModAPI_Asset_Sprite::CStorageType), &Item);
}

void CModAPI_Asset_Sprite::Unload(class CModAPI_Client_Graphics* pModAPIGraphics)
{
	
}
