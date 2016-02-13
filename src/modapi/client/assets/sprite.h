#ifndef MODAPI_CLIENT_ASSETS_SPRITE_H
#define MODAPI_CLIENT_ASSETS_SPRITE_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Sprite : public CModAPI_Asset
{
public:
	static const int TypeId = MODAPI_ASSETTYPE_SPRITE;
	static const int ListId = MODAPI_ASSETTYPE_SPRITE_LIST;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_ImagePath;
		int m_X;
		int m_Y;
		int m_Width;
		int m_Height;
	};
	
	void InitFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics);
	
public:
	CModAPI_AssetPath m_ImagePath;
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;

public:
	CModAPI_Asset_Sprite() :
		m_X(0),
		m_Y(0),
		m_Width(1),
		m_Height(1)
	{
		
	}

	inline void Init(CModAPI_AssetPath ImagePath, int X, int Y, int W, int H)
	{
		m_ImagePath = ImagePath;
		m_X = X;
		m_Y = Y;
		m_Width = W;
		m_Height = H;
	}
};

#endif
