#include "teeanimation.h"

#include <engine/shared/datafile.h>
#include <modapi/client/graphics.h>

void CModAPI_Asset_TeeAnimation::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_TeeAnimation::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_BodyAnimationPath = CModAPI_AssetPath(pItem->m_BodyAnimationPath);
	m_BackFootAnimationPath = CModAPI_AssetPath(pItem->m_BackFootAnimationPath);
	m_FrontFootAnimationPath = CModAPI_AssetPath(pItem->m_FrontFootAnimationPath);
	
	m_BackHandAnimationPath = CModAPI_AssetPath(pItem->m_BackHandAnimationPath);
	m_BackHandAlignment = pItem->m_BackHandAlignment;
	m_BackHandOffsetX = pItem->m_BackHandOffsetX;
	m_BackHandOffsetY = pItem->m_BackHandOffsetY;
	
	m_FrontHandAnimationPath = CModAPI_AssetPath(pItem->m_FrontHandAnimationPath);
	m_FrontHandAlignment = pItem->m_FrontHandAlignment;
	m_FrontHandOffsetX = pItem->m_FrontHandOffsetX;
	m_FrontHandOffsetY = pItem->m_FrontHandOffsetY;
}

void CModAPI_Asset_TeeAnimation::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_TeeAnimation::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	
	Item.m_BodyAnimationPath = m_BodyAnimationPath.ConvertToInteger();
	Item.m_BackFootAnimationPath = m_BackFootAnimationPath.ConvertToInteger();
	Item.m_FrontFootAnimationPath = m_FrontFootAnimationPath.ConvertToInteger();
	
	Item.m_BackHandAnimationPath = m_BackHandAnimationPath.ConvertToInteger();
	Item.m_BackHandAlignment = m_BackHandAlignment;
	Item.m_BackHandOffsetX = m_BackHandOffsetX;
	Item.m_BackHandOffsetY = m_BackHandOffsetY;
	
	Item.m_FrontHandAnimationPath = m_FrontHandAnimationPath.ConvertToInteger();
	Item.m_FrontHandAlignment = m_FrontHandAlignment;
	Item.m_FrontHandOffsetX = m_FrontHandOffsetX;
	Item.m_FrontHandOffsetY = m_FrontHandOffsetY;
	
	pFileWriter->AddItem(CModAPI_Asset_TeeAnimation::TypeId, Position, sizeof(CModAPI_Asset_TeeAnimation::CStorageType), &Item);
}

void CModAPI_Asset_TeeAnimation::Unload(class CModAPI_AssetManager* pAssetManager)
{
	
}
