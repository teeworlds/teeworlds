#include "image.h"

#include <engine/shared/datafile.h>
#include <modapi/client/assetsmanager.h>

void CModAPI_Asset_Image::InitFromAssetsFile(CModAPI_AssetManager* pAssetManager, IModAPI_AssetsFile* pAssetsFile, const CModAPI_Asset_Image::CStorageType* pItem)
{
	// copy name
	SetName((char *)pAssetsFile->GetData(pItem->m_Name));
				
	// copy info
	m_Width = pItem->m_Width;
	m_Height = pItem->m_Height;
	m_Format = pItem->m_Format;
	m_GridWidth = max(1, pItem->m_GridWidth);
	m_GridHeight = max(1, pItem->m_GridHeight);
	int PixelSize = (pItem->m_Format == CImageInfo::FORMAT_RGB ? 3 : 4);
	int ImageSize = m_Width * m_Height * PixelSize;
	
	// copy image data
	void *pData = pAssetsFile->GetData(pItem->m_ImageData);
	m_pData = mem_alloc(ImageSize, 1);
	mem_copy(m_pData, pData, ImageSize);
	
	m_Texture = pAssetManager->Graphics()->LoadTextureRaw(m_Width, m_Height, m_Format, m_pData, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);

	// unload image
	pAssetsFile->UnloadData(pItem->m_ImageData);
}

void CModAPI_Asset_Image::SaveInAssetsFile(CDataFileWriter* pFileWriter, int Position)
{
	CModAPI_Asset_Image::CStorageType Item;
	Item.m_Name = pFileWriter->AddData(str_length(m_aName)+1, m_aName);
	Item.m_Width = m_Width; // ignore_convention
	Item.m_Height = m_Height; // ignore_convention
	Item.m_Format = m_Format;
	Item.m_GridWidth = m_GridWidth;
	Item.m_GridHeight = m_GridHeight;
	
	int PixelSize = (m_Format == CImageInfo::FORMAT_RGB ? 3 : 4);
	int ImageSize = m_Width * m_Height * PixelSize;
	
	Item.m_ImageData = pFileWriter->AddData(ImageSize, m_pData);
	pFileWriter->AddItem(CModAPI_AssetPath::TypeToStoredType(CModAPI_Asset_Image::TypeId), Position, sizeof(CModAPI_Asset_Image::CStorageType), &Item);
}

void CModAPI_Asset_Image::Unload(class CModAPI_AssetManager* pAssetManager)
{
	pAssetManager->Graphics()->UnloadTexture(m_Texture);
	if(m_pData)
	{
		mem_free(m_pData);
		m_pData = 0;
	}
}
	
template<>
bool CModAPI_Asset_Image::SetValue<int>(int ValueType, int Path, int Value)
{
	switch(ValueType)
	{
		case GRIDWIDTH:
			m_GridWidth = max(1, Value);
			return true;
		case GRIDHEIGHT:
			m_GridHeight = max(1, Value);
			return true;
	}
	
	return CModAPI_Asset::SetValue<int>(ValueType, Path, Value);
}

template<>
int CModAPI_Asset_Image::GetValue(int ValueType, int Path, int DefaultValue)
{
	switch(ValueType)
	{
		case GRIDWIDTH:
			return max(1, m_GridWidth);
		case GRIDHEIGHT:
			return max(1, m_GridHeight);
		case WIDTH:
			return m_Width;
		case HEIGHT:
			return m_Height;
		default:
			return CModAPI_Asset::GetValue<int>(ValueType, Path, DefaultValue);
	}
}
