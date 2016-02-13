#ifndef MODAPI_CLIENT_ASSETS_IMAGE_H
#define MODAPI_CLIENT_ASSETS_IMAGE_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Image : public CModAPI_Asset
{
public:
	static const int TypeId = MODAPI_ASSETTYPE_IMAGE;
	static const int ListId = MODAPI_ASSETTYPE_IMAGE_LIST;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_Width;
		int m_Height;
		int m_Format;
		int m_ImageData;
		int m_GridWidth;
		int m_GridHeight;
	};
	
	void InitFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics);
	
public:
	enum
	{
		FORMAT_AUTO=-1,
		FORMAT_RGB=0,
		FORMAT_RGBA=1,
		FORMAT_ALPHA=2,
	};

	int m_Width;
	int m_Height;
	int m_Format;
	void *m_pData;
	
	IGraphics::CTextureHandle m_Texture;
	
	int m_GridWidth;
	int m_GridHeight;
};

#endif
