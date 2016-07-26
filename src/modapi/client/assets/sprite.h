#ifndef MODAPI_CLIENT_ASSETS_SPRITE_H
#define MODAPI_CLIENT_ASSETS_SPRITE_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Sprite : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_SPRITE;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_ImagePath;
		int m_X;
		int m_Y;
		int m_Width;
		int m_Height;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
public:
	enum
	{
		FLAG_FLIP_Y = (0x1 << 0),
		FLAG_FLIP_X = (0x1 << 1),
		FLAG_FLIP_ANIM_Y = (0x1 << 2),
		FLAG_FLIP_ANIM_X = (0x1 << 3),
		FLAG_SIZE_HEIGHT = (0x1 << 4),
	};

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
	
	//Getter/Setter for AssetsEditor
public:
	enum
	{
		IMAGEPATH = CModAPI_Asset::NUM_MEMBERS, //Path
		X, //Int
		Y, //Int
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
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		m_ImagePath.OnIdDeleted(Path);
	}
	
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
