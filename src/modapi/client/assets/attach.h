#ifndef MODAPI_CLIENT_ASSETS_ATTACH_H
#define MODAPI_CLIENT_ASSETS_ATTACH_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Attach : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_ATTACH;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_TeeAnimationPath;
		int m_CursorPath;
		int m_NumBackElements;
		int m_BackElementsData;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
public:
	class CElement
	{
	public:
		CModAPI_AssetPath m_SpritePath;
		int m_Size;
		CModAPI_AssetPath m_AnimationPath;
		int m_Alignment;
		int m_OffsetX;
		int m_OffsetY;
		
		CElement() :
			m_Size(32),
			m_Alignment(MODAPI_TEEALIGN_NONE),
			m_OffsetX(0),
			m_OffsetY(0)
		{
			
		}
	};

	CModAPI_AssetPath m_TeeAnimationPath;
	CModAPI_AssetPath m_CursorPath;
	array<CElement> m_BackElements;
	
public:
	void AddBackElement(const CElement& Elem);
	void AddBackElement(CModAPI_AssetPath SpritePath, int Size, CModAPI_AssetPath AnimationPath, int Alignment, int OffsetX, int OffsetY);
	void MoveUpBackElement(int Id);
	void MoveDownBackElement(int Id);
	void DeleteBackElement(int Id);
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		m_TeeAnimationPath.OnIdDeleted(Path);
		m_CursorPath.OnIdDeleted(Path);
		for(int i=0; i<m_BackElements.size(); i++)
		{
			m_BackElements[i].m_SpritePath.OnIdDeleted(Path);
			m_BackElements[i].m_AnimationPath.OnIdDeleted(Path);
		}
	}
	
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
