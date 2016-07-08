#ifndef MODAPI_CLIENT_ASSETS_ATTACH_H
#define MODAPI_CLIENT_ASSETS_ATTACH_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Attach : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_ATTACH;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_CharacterPath;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);

public:
	CModAPI_AssetPath m_CharacterPath;

public:
	CModAPI_Asset_Attach()
	{
		
	}
	
	//Getter/Setter for AssetsEdtiro
public:
	enum
	{
		CHARACTERPATH = CModAPI_Asset::NUM_MEMBERS, //Path
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
