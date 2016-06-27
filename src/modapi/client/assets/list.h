#ifndef MODAPI_CLIENT_ASSETS_LIST_H
#define MODAPI_CLIENT_ASSETS_LIST_H

#include <modapi/client/assets.h>

class CModAPI_Asset_List : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_LIST;
	
	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_NumElements;
		int m_ElementsData;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position, int ElementType);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
public:
	array<CModAPI_AssetPath> m_Elements;

public:
	inline void Add(CModAPI_AssetPath Path)
	{
		m_Elements.add(Path);
	}
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		for(int i=0; i<m_Elements.size(); i++)
		{
			m_Elements[i].OnIdDeleted(Path);
		}
	}
	
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
