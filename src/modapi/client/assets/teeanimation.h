#ifndef MODAPI_CLIENT_ASSETS_TEEANIMATION_H
#define MODAPI_CLIENT_ASSETS_TEEANIMATION_H

#include <modapi/client/assets.h>

class CModAPI_Asset_TeeAnimation : public CModAPI_Asset
{
public:
	static const int TypeId = MODAPI_ASSETTYPE_TEEANIMATION;
	static const int ListId = MODAPI_ASSETTYPE_TEEANIMATION_LIST;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_BodyAnimationPath;
		int m_BackFootAnimationPath;
		int m_FrontFootAnimationPath;
		int m_BackHandAnimationPath;
		int m_BackHandAlignment;
		int m_BackHandOffsetX;
		int m_BackHandOffsetY;
		int m_FrontHandAnimationPath;
		int m_FrontHandAlignment;
		int m_FrontHandOffsetX;
		int m_FrontHandOffsetY;
	};
	
	void InitFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics);
	
public:
	CModAPI_AssetPath m_BodyAnimationPath;
	CModAPI_AssetPath m_BackFootAnimationPath;
	CModAPI_AssetPath m_FrontFootAnimationPath;
	
	CModAPI_AssetPath m_BackHandAnimationPath;
	int m_BackHandAlignment;
	int m_BackHandOffsetX;
	int m_BackHandOffsetY;
	
	CModAPI_AssetPath m_FrontHandAnimationPath;
	int m_FrontHandAlignment;
	int m_FrontHandOffsetX;
	int m_FrontHandOffsetY;
	
public:
	CModAPI_Asset_TeeAnimation() :
		m_BackHandAlignment(MODAPI_TEEALIGN_NONE),
		m_BackHandOffsetX(0),
		m_BackHandOffsetY(0),
		m_FrontHandAlignment(MODAPI_TEEALIGN_NONE),
		m_FrontHandOffsetX(0),
		m_FrontHandOffsetY(0)
	{
		
	}
};

#endif
