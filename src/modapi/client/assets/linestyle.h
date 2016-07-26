#ifndef MODAPI_CLIENT_ASSETS_LINESTYLE_H
#define MODAPI_CLIENT_ASSETS_LINESTYLE_H

#include <modapi/client/assets.h>

class CModAPI_Asset_LineStyle : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_LINESTYLE;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_StartPointSpritePath;
		int m_StartPointSize;
		int m_StartPointAnimationPath;
		int m_EndPointSpritePath;
		int m_EndPointSize;
		int m_EndPointAnimationPath;
		int m_NumElements;
		int m_ElementsData;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
public:	
	enum
	{
		TILINGTYPE_NONE=0,
		TILINGTYPE_STRETCH,
		TILINGTYPE_REPEATED,
	};

	class CElement
	{
	public:
		CModAPI_AssetPath m_SpritePath;
		int m_Size;
		CModAPI_AssetPath m_AnimationPath;
		float m_StartPosition;
		float m_EndPosition;
		int m_TilingType;
		int m_TileSpacing;
		
		CElement() :
			m_Size(32)
		{
			
		}
	};
	
	array<CElement> m_Elements;
	
public:
	void AddElement(const CElement& Elem);
	void AddElement(CModAPI_AssetPath SpritePath, int Size, CModAPI_AssetPath AnimationPath, float StartPosition, float EndPosition, int TilingType, int TileSpacing);
	void MoveUpElement(int Id);
	void MoveDownElement(int Id);
	void DeleteElement(int Id);
	
	CModAPI_Asset_LineStyle()
	{
		
	}
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		for(int i=0; i<m_Elements.size(); i++)
		{
			m_Elements[i].m_SpritePath.OnIdDeleted(Path);
			m_Elements[i].m_AnimationPath.OnIdDeleted(Path);
		}
	}
	
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
