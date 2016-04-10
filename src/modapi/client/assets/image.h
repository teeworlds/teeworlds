#ifndef MODAPI_CLIENT_ASSETS_IMAGE_H
#define MODAPI_CLIENT_ASSETS_IMAGE_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Image : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_IMAGE;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_Width;
		int m_Height;
		int m_Format;
		int m_ImageData;
		int m_GridWidth;
		int m_GridHeight;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
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
	
	//Getter/Setter for AssetsEditor
public:
	enum
	{
		GRIDWIDTH = CModAPI_Asset::NUM_MEMBERS, //Int
		GRIDHEIGHT, //Int
		WIDTH, //Int
		HEIGHT, //Int
	};
	
	template<typename T>
	T GetValue(int ValueType, int Path, T DefaultValue)
	{
		return CModAPI_Asset::GetValue<T>(ValueType, Path, DefaultValue);
	}
	
	template<typename T>
	bool SetValue(int ValueType, int Path, T Value)
	{
		return CModAPI_Asset::SetValue<T>(ValueType, Path, Value);
	}
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path) { }
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
