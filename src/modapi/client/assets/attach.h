#ifndef MODAPI_CLIENT_ASSETS_ATTACH_H
#define MODAPI_CLIENT_ASSETS_ATTACH_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Attach : public CModAPI_Asset
{
public:
	static const int TypeId = MODAPI_ASSETTYPE_ATTACH;
	static const int ListId = MODAPI_ASSETTYPE_ATTACH_LIST;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_TeeAnimationPath;
		int m_CursorPath;
		int m_NumBackElements;
		int m_BackElementsData;
	};
	
	void InitFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics);
	
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
};

#endif
