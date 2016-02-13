#ifndef MODAPI_CLIENT_ASSETS_LIST_H
#define MODAPI_CLIENT_ASSETS_LIST_H

#include <modapi/client/assets.h>

class CModAPI_Asset_List : public CModAPI_Asset
{
public:
	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_NumElements;
		int m_ElementsData;
	};
	
	void InitFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position, int ElementType);
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics);
	
public:
	array<CModAPI_AssetPath> m_Elements;

public:
	inline void Add(CModAPI_AssetPath Path)
	{
		m_Elements.add(Path);
	}
};

#endif
