#ifndef MODAPI_CLIENT_ASSETS_TEEANIMATION_H
#define MODAPI_CLIENT_ASSETS_TEEANIMATION_H

#include <modapi/client/assets.h>

class CModAPI_Asset_TeeAnimation : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_TEEANIMATION;

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
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
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
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		m_BodyAnimationPath.OnIdDeleted(Path);
		m_BackFootAnimationPath.OnIdDeleted(Path);
		m_FrontFootAnimationPath.OnIdDeleted(Path);
		m_BackHandAnimationPath.OnIdDeleted(Path);
		m_FrontHandAnimationPath.OnIdDeleted(Path);
	}
	
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
