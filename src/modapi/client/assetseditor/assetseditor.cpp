#include <base/tl/sorted_array.h>

#include <engine/shared/config.h>
#include <engine/shared/datafile.h>

#include <engine/client.h>
#include <engine/input.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/keys.h>

#include <modapi/client/clientmode.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/label.h>
#include <modapi/client/gui/layout.h>
#include <modapi/client/gui/integer-edit.h>
#include <modapi/client/gui/text-edit.h>

#include <cstddef>

#include "assetseditor.h"
#include "editor.h"
#include "view.h"
#include "timeline.h"

/* FUNCTIONS **********************************************************/

void GetAssetName(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, char* pText, int TextSize)
{
	if(AssetType < 0)
	{
		str_format(pText, TextSize, "Unknown Asset");
	}
	else if(AssetPath.IsNull())
	{
		str_format(pText, TextSize, "None");
	}
	else
	{
		switch(AssetType)
		{
			case MODAPI_ASSETTYPE_IMAGE:
			{
				if(!AssetPath.IsList())
				{
					CModAPI_Asset_Image* pImage = pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.GetAsset(AssetPath);
					if(pImage)
						str_format(pText, TextSize, pImage->m_aName);
					else
						str_format(pText, TextSize, "Unknown Image");
				}
				else
				{
					CModAPI_Asset_List* pList = pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.GetList(AssetPath);
					if(pList)
						str_format(pText, TextSize, pList->m_aName);
					else
						str_format(pText, TextSize, "Unknown List");
				}
				return;
			}
			case MODAPI_ASSETTYPE_SPRITE:
			{
				if(!AssetPath.IsList())
				{
					CModAPI_Asset_Sprite* pSprite = pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetAsset(AssetPath);
					if(pSprite)
						str_format(pText, TextSize, pSprite->m_aName);
					else
						str_format(pText, TextSize, "Unknown Sprite");
				}
				else
				{
					CModAPI_Asset_List* pList = pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetList(AssetPath);
					if(pList)
						str_format(pText, TextSize, pList->m_aName);
					else
						str_format(pText, TextSize, "Unknown List");
				}
				return;
			}
			case MODAPI_ASSETTYPE_ANIMATION:
			{
				if(!AssetPath.IsList())
				{
					CModAPI_Asset_Animation* pAnim = pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(AssetPath);
					if(pAnim)
						str_format(pText, TextSize, pAnim->m_aName);
					else
						str_format(pText, TextSize, "Unknown Animation");
				}
				else
				{
					CModAPI_Asset_List* pList = pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetList(AssetPath);
					if(pList)
						str_format(pText, TextSize, pList->m_aName);
					else
						str_format(pText, TextSize, "Unknown List");
				}
				return;
			}
			case MODAPI_ASSETTYPE_TEEANIMATION:
			{
				if(!AssetPath.IsList())
				{
					CModAPI_Asset_TeeAnimation* pAnim = pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(AssetPath);
					if(pAnim)
						str_format(pText, TextSize, pAnim->m_aName);
					else
						str_format(pText, TextSize, "Unknown Tee Animation");
				}
				else
				{
					CModAPI_Asset_List* pList = pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.GetList(AssetPath);
					if(pList)
						str_format(pText, TextSize, pList->m_aName);
					else
						str_format(pText, TextSize, "Unknown List");
				}
				return;
			}
			case MODAPI_ASSETTYPE_ATTACH:
			{
				if(!AssetPath.IsList())
				{
					CModAPI_Asset_Attach* pAttach = pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(AssetPath);
					if(pAttach)
						str_format(pText, TextSize, pAttach->m_aName);
					else
						str_format(pText, TextSize, "Unknown Attach");
				}
				else
				{
					CModAPI_Asset_List* pList = pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetList(AssetPath);
					if(pList)
						str_format(pText, TextSize, pList->m_aName);
					else
						str_format(pText, TextSize, "Unknown List");
				}
				return;
			}
		}
	}
}

/* BUTTONS ************************************************************/

class CModAPI_AssetsEditorGui_TextButton : public CModAPI_ClientGui_TextButton
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;

public:
	CModAPI_AssetsEditorGui_TextButton(CModAPI_AssetsEditor* pAssetsEditor, const char* pText = 0, int IconId = -1) :
		CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, pText, IconId),
		m_pAssetsEditor(pAssetsEditor)
	{
		
	}
};

class CModAPI_AssetsEditorGui_IconButton : public CModAPI_ClientGui_IconButton
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;

public:
	CModAPI_AssetsEditorGui_IconButton(CModAPI_AssetsEditor* pAssetsEditor, int IconId) :
		CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, IconId),
		m_pAssetsEditor(pAssetsEditor)
	{
		
	}
};

class CModAPI_AssetsEditorGui_Button_ToolbarExit : public CModAPI_AssetsEditorGui_TextButton
{	
protected:
	virtual void MouseClickAction()
	{
		m_pAssetsEditor->CloseEditor();
	}

public:
	CModAPI_AssetsEditorGui_Button_ToolbarExit(CModAPI_AssetsEditor* pAssetsEditor) :
		CModAPI_AssetsEditorGui_TextButton(pAssetsEditor, "Exit", -1)
	{
		SetWidth(100);
	}
};

class CModAPI_AssetsEditorGui_Button_ToolbarLoad : public CModAPI_AssetsEditorGui_TextButton
{	
protected:
	virtual void MouseClickAction()
	{
		m_pAssetsEditor->LoadAssetsFile();
	}

public:
	CModAPI_AssetsEditorGui_Button_ToolbarLoad(CModAPI_AssetsEditor* pAssetsEditor) :
		CModAPI_AssetsEditorGui_TextButton(pAssetsEditor, "Load", -1)
	{
		SetWidth(100);
	}
};

class CModAPI_AssetsEditorGui_Button_ToolbarSave : public CModAPI_AssetsEditorGui_TextButton
{	
protected:
	virtual void MouseClickAction()
	{
		m_pAssetsEditor->SaveAssetsFile();
	}

public:
	CModAPI_AssetsEditorGui_Button_ToolbarSave(CModAPI_AssetsEditor* pAssetsEditor) :
		CModAPI_AssetsEditorGui_TextButton(pAssetsEditor, "Save", -1)
	{
		SetWidth(100);
	}
};

class CModAPI_AssetsEditorGui_Button_DeleteAsset : public CModAPI_AssetsEditorGui_IconButton
{
protected:
	int m_AssetType;
	CModAPI_AssetPath m_AssetPath;
	
protected:
	virtual void MouseClickAction()
	{
		m_pAssetsEditor->DeleteAsset(m_AssetType, m_AssetPath);
	}

public:
	CModAPI_AssetsEditorGui_Button_DeleteAsset(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath) :
		CModAPI_AssetsEditorGui_IconButton(pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_DELETE),
		m_AssetType(AssetType),
		m_AssetPath(AssetPath)
	{
		
	}
};

class CModAPI_AssetsEditorGui_Button_FileListElement : public CModAPI_AssetsEditorGui_TextButton
{
protected:
	char m_aFilename[128];
	int m_StorageType;
	int m_IsDirectory;
	
protected:
	virtual void MouseClickAction()
	{
		if(!m_IsDirectory)
		{
			CModAPI_AssetPath Path = m_pAssetsEditor->ModAPIGraphics()->AddImage(m_pAssetsEditor->Storage(), m_StorageType, m_aFilename);
			if(!Path.IsNull())
			{
				m_pAssetsEditor->NewAsset(MODAPI_ASSETTYPE_IMAGE, Path);
			}
			m_pAssetsEditor->ClosePopup();
		}
	}
	
public:
	CModAPI_AssetsEditorGui_Button_FileListElement(CModAPI_AssetsEditor* pAssetsEditor, const char* pFilename, int StorageType, int IsDir) :
		CModAPI_AssetsEditorGui_TextButton(pAssetsEditor, pFilename)
	{
		str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
		m_StorageType = StorageType;
		m_IsDirectory = IsDir;
	}
};

class CModAPI_AssetsEditorGui_AssetEditListButton : public CModAPI_AssetsEditorGui_TextButton
{
protected:
	int m_ParentAssetType;
	CModAPI_AssetPath m_ParentAssetPath;
	int m_ParentAssetMember;
	int m_ParentAssetSubId;
	int m_FieldAssetType;
	CModAPI_AssetPath m_FieldAssetPath;
	
protected:
	virtual void MouseClickAction()
	{
		m_pAssetsEditor->ClosePopup();
		
		CModAPI_AssetPath* ptr = GetAssetMemberPointer<CModAPI_AssetPath>(m_pAssetsEditor, m_ParentAssetType, m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId);
		if(ptr)
		{
			*ptr = m_FieldAssetPath;
		}
		
		m_pAssetsEditor->RefreshAssetEditor();
	}
	
public:
	CModAPI_AssetsEditorGui_AssetEditListButton(CModAPI_AssetsEditor* pAssetsEditor, int ParentAssetType, CModAPI_AssetPath ParentAssetPath, int ParentAssetMember, int ParentAssetSubId, int FieldAssetType, CModAPI_AssetPath FieldAssetPath) :
		CModAPI_AssetsEditorGui_TextButton(pAssetsEditor, 0),
		m_ParentAssetType(ParentAssetType),
		m_ParentAssetPath(ParentAssetPath),
		m_ParentAssetMember(ParentAssetMember),
		m_ParentAssetSubId(ParentAssetSubId),
		m_FieldAssetType(FieldAssetType),
		m_FieldAssetPath(FieldAssetPath)
	{
		char aText[256];
		GetAssetName(m_pAssetsEditor, m_FieldAssetType, m_FieldAssetPath, aText, sizeof(aText));
		SetText(aText);
	}
};

/* POPUP **************************************************************/

CModAPI_AssetsEditorGui_Popup::CModAPI_AssetsEditorGui_Popup(CModAPI_AssetsEditor* pAssetsEditor) :
	CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig),
	m_pAssetsEditor(pAssetsEditor)
{
	m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
	m_Layout->SetRect(CModAPI_ClientGui_Rect(0, 0, 500, 500));
	m_Layout->Update();
	Add(m_Layout);
}

CModAPI_AssetsEditorGui_Popup_AddImage::CCancel::CCancel(CModAPI_AssetsEditorGui_Popup_AddImage* pPopup) :
	CModAPI_ClientGui_TextButton(pPopup->m_pConfig, "Cancel", MODAPI_ASSETSEDITOR_ICON_DELETE),
	m_pPopup(pPopup)
{
	SetWidth(120);
}

void CModAPI_AssetsEditorGui_Popup_AddImage::CCancel::MouseClickAction()
{
	m_pPopup->Cancel();
}

CModAPI_AssetsEditorGui_Popup_AddImage::CModAPI_AssetsEditorGui_Popup_AddImage(CModAPI_AssetsEditor* pAssetsEditor) :
	CModAPI_AssetsEditorGui_Popup(pAssetsEditor)
{
	m_Toolbar->Add(new CModAPI_AssetsEditorGui_Popup_AddImage::CCancel(this));
	m_Toolbar->Update();
}

int CModAPI_AssetsEditorGui_Popup_AddImage::FileListFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	CModAPI_AssetsEditorGui_Popup_AddImage* pAddImageView = static_cast<CModAPI_AssetsEditorGui_Popup_AddImage*>(pUser);
	
	int Length = str_length(pName);
	if(pName[0] == '.' && (pName[1] == 0))
		return 0;
	
	if(Length < 4 || str_comp(pName+Length-4, ".png"))
		return 0;

	CModAPI_AssetsEditorGui_Button_FileListElement* pItem = new CModAPI_AssetsEditorGui_Button_FileListElement(pAddImageView->m_pAssetsEditor, pName, StorageType, IsDir);
	pAddImageView->m_Layout->Add(pItem);

	return 0;
}

void CModAPI_AssetsEditorGui_Popup_AddImage::Update()
{
	m_pAssetsEditor->Storage()->ListDirectory(IStorage::TYPE_ALL, ".", FileListFetchCallback, this);
	
	CModAPI_AssetsEditorGui_Popup::Update();
}

void CModAPI_AssetsEditorGui_Popup_AddImage::Cancel()
{
	m_pAssetsEditor->ClosePopup();
}

CModAPI_AssetsEditorGui_Popup_AssetEditList::CCancel::CCancel(CModAPI_AssetsEditorGui_Popup_AssetEditList* pPopup) :
	CModAPI_ClientGui_TextButton(pPopup->m_pConfig, "Cancel", MODAPI_ASSETSEDITOR_ICON_DELETE),
	m_pPopup(pPopup)
{
	SetWidth(120);
}

void CModAPI_AssetsEditorGui_Popup_AssetEditList::CCancel::MouseClickAction()
{
	m_pPopup->Cancel();
}

CModAPI_AssetsEditorGui_Popup_AssetEditList::CModAPI_AssetsEditorGui_Popup_AssetEditList(
	CModAPI_AssetsEditor* pAssetsEditor,
	int ParentAssetType,
	CModAPI_AssetPath ParentAssetPath,
	int ParentAssetMember,
	int ParentAssetSubId,
	int FieldAssetType
) :
	CModAPI_AssetsEditorGui_Popup(pAssetsEditor),
	m_ParentAssetType(ParentAssetType),
	m_ParentAssetPath(ParentAssetPath),
	m_ParentAssetMember(ParentAssetMember),
	m_ParentAssetSubId(ParentAssetSubId),
	m_FieldAssetType(FieldAssetType)
{
	m_Toolbar->Add(new CModAPI_AssetsEditorGui_Popup_AssetEditList::CCancel(this));
	m_Toolbar->Update();
}

void CModAPI_AssetsEditorGui_Popup_AssetEditList::AddListElement(CModAPI_AssetPath Path)
{
	char aText[256];
	GetAssetName(m_pAssetsEditor, m_FieldAssetType, Path, aText, sizeof(aText));
	
	CModAPI_AssetsEditorGui_AssetEditListButton* pItem = new CModAPI_AssetsEditorGui_AssetEditListButton(
		m_pAssetsEditor,
		m_ParentAssetType,
		m_ParentAssetPath,
		m_ParentAssetMember,
		m_ParentAssetSubId,
		m_FieldAssetType,
		Path
	);
	m_Layout->Add(pItem);
}

void CModAPI_AssetsEditorGui_Popup_AssetEditList::Update()
{
	AddListElement(CModAPI_AssetPath::Null());
	switch(m_FieldAssetType)
	{
		case MODAPI_ASSETTYPE_IMAGE:
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal Images", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.m_InternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::Internal(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External Images", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.m_ExternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::External(i));
			break;
		case MODAPI_ASSETTYPE_SPRITE:
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal Sprites", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_InternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::Internal(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal Sprite Lists", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_InternalLists.size(); i++)
				AddListElement(CModAPI_AssetPath::InternalList(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External Sprites", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_ExternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::External(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External Sprite Lists", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_ExternalLists.size(); i++)
				AddListElement(CModAPI_AssetPath::ExternalList(i));
			break;
		case MODAPI_ASSETTYPE_ANIMATION:
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.m_InternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::Internal(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.m_ExternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::External(i));
			break;
		case MODAPI_ASSETTYPE_TEEANIMATION:
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal Tee Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.m_InternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::Internal(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External Tee Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.m_ExternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::External(i));
			break;
		case MODAPI_ASSETTYPE_ATTACH:
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal Attaches", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.m_InternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::Internal(i));
			m_Layout->AddSeparator();
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External Attaches", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.m_ExternalAssets.size(); i++)
				AddListElement(CModAPI_AssetPath::External(i));
			break;
	}
	
	CModAPI_AssetsEditorGui_Popup::Update();
}

void CModAPI_AssetsEditorGui_Popup_AssetEditList::Cancel()
{
	m_pAssetsEditor->ClosePopup();
}

/* ITEM LIST **********************************************************/

class CModAPI_AssetsEditorGui_AssetListHeader : public CModAPI_ClientGui_HListLayout
{
protected:
	class CAddButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditorGui_AssetListHeader* m_pAssetListHeader;
	
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetListHeader->AddAsset();
		}
		
	public:
		CAddButton(CModAPI_AssetsEditorGui_AssetListHeader* pAssetListHeader) :
			CModAPI_ClientGui_IconButton(pAssetListHeader->m_pConfig, MODAPI_ASSETSEDITOR_ICON_INCREASE),
			m_pAssetListHeader(pAssetListHeader)
		{
			
		}
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	int m_AssetType;

public:
	CModAPI_AssetsEditorGui_AssetListHeader(CModAPI_AssetsEditor* pAssetsEditor, int AssetType) :
		CModAPI_ClientGui_HListLayout(pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetType(AssetType)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		switch(m_AssetType)
		{
			case MODAPI_ASSETTYPE_IMAGE:
				Add(new CModAPI_ClientGui_Label(m_pConfig, "Images", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
				break;
			case MODAPI_ASSETTYPE_SPRITE:
				Add(new CModAPI_ClientGui_Label(m_pConfig, "Sprites", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
				break;
			case MODAPI_ASSETTYPE_SPRITE_LIST:
				Add(new CModAPI_ClientGui_Label(m_pConfig, "Sprite Lists", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
				break;
			case MODAPI_ASSETTYPE_ANIMATION:
				Add(new CModAPI_ClientGui_Label(m_pConfig, "Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
				break;
			case MODAPI_ASSETTYPE_TEEANIMATION:
				Add(new CModAPI_ClientGui_Label(m_pConfig, "Tee Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
				break;
			case MODAPI_ASSETTYPE_ATTACH:
				Add(new CModAPI_ClientGui_Label(m_pConfig, "Attaches", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
				break;
		}
		Add(new CAddButton(this));
		
		Update();
	}
	
	void AddAsset()
	{
		CModAPI_AssetPath NewAssetPath;
		
		switch(m_AssetType)
		{
			case MODAPI_ASSETTYPE_IMAGE:
				m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_AddImage(m_pAssetsEditor));
				break;
			case MODAPI_ASSETTYPE_SPRITE:
			{
				CModAPI_Asset_Sprite* pAsset = m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.NewExternalAsset(&NewAssetPath);
				
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "sprite%d", NewAssetPath.GetId());
				pAsset->SetName(aBuf);
				
				m_pAssetsEditor->NewAsset(MODAPI_ASSETTYPE_SPRITE, NewAssetPath);
				break;
			}
			case MODAPI_ASSETTYPE_SPRITE_LIST:
			{
				
			}
			case MODAPI_ASSETTYPE_ANIMATION:
			{
				CModAPI_Asset_Animation* pAsset = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.NewExternalAsset(&NewAssetPath);
				
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "animation%d", NewAssetPath.GetId());
				pAsset->SetName(aBuf);
				
				m_pAssetsEditor->NewAsset(MODAPI_ASSETTYPE_ANIMATION, NewAssetPath);
				break;
			}
			case MODAPI_ASSETTYPE_TEEANIMATION:
			{
				CModAPI_Asset_TeeAnimation* pAsset = m_pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.NewExternalAsset(&NewAssetPath);
				
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "teeAnimation%d", NewAssetPath.GetId());
				pAsset->SetName(aBuf);
				
				m_pAssetsEditor->NewAsset(MODAPI_ASSETTYPE_TEEANIMATION, NewAssetPath);
				break;
			}
			case MODAPI_ASSETTYPE_ATTACH:
			{
				CModAPI_Asset_Attach* pAsset = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.NewExternalAsset(&NewAssetPath);
				
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "attach%d", NewAssetPath.GetId());
				pAsset->SetName(aBuf);
				
				m_pAssetsEditor->NewAsset(MODAPI_ASSETTYPE_ATTACH, NewAssetPath);
				break;
			}
		}
	}
};

class CModAPI_AssetsEditorGui_AssetListItem : public CModAPI_ClientGui_HListLayout
{
protected:
	class CEditButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditorGui_AssetListItem* m_pAssetListItem;
	
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetListItem->EditAsset();
		}
		
	public:
		CEditButton(CModAPI_AssetsEditorGui_AssetListItem* pAssetListItem) :
			CModAPI_ClientGui_IconButton(pAssetListItem->m_pConfig, MODAPI_ASSETSEDITOR_ICON_EDIT),
			m_pAssetListItem(pAssetListItem)
		{
			
		}
	};
	
	class CDisplayButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditorGui_AssetListItem* m_pAssetListItem;
	
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetListItem->DisplayAsset();
		}
		
	public:
		CDisplayButton(CModAPI_AssetsEditorGui_AssetListItem* pAssetListItem) :
			CModAPI_ClientGui_IconButton(pAssetListItem->m_pConfig, MODAPI_ASSETSEDITOR_ICON_VIEW),
			m_pAssetListItem(pAssetListItem)
		{
			
		}
	};
	
	class CItemListButton : public CModAPI_ClientGui_ExternalTextButton
	{
	protected:
		CModAPI_AssetsEditorGui_AssetListItem* m_pAssetListItem;
	
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetListItem->EditAsset();
			m_pAssetListItem->DisplayAsset();
		}
		
	public:
		CItemListButton(CModAPI_AssetsEditorGui_AssetListItem* pAssetListItem, char* pText) :
			CModAPI_ClientGui_ExternalTextButton(pAssetListItem->m_pConfig, pText),
			m_pAssetListItem(pAssetListItem)
		{
			m_Centered = false;
		}
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	int m_AssetType;
	CModAPI_AssetPath m_AssetPath;
	
	CEditButton* m_pEditButton;
	CDisplayButton* m_pDisplayButton;

public:
	CModAPI_AssetsEditorGui_AssetListItem(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath) :
		CModAPI_ClientGui_HListLayout(pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetType(AssetType),
		m_AssetPath(AssetPath)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		Add(new CItemListButton(this, GetAssetMemberPointer<char>(m_pAssetsEditor, m_AssetType, m_AssetPath, MODAPI_ASSETSEDITOR_MEMBER_NAME, -1)));
		
		m_pDisplayButton = new CDisplayButton(this);
		Add(m_pDisplayButton);
		
		m_pEditButton = new CEditButton(this);
		Add(m_pEditButton);
		
		Update();
	}
	
	void EditAsset()
	{
		m_pAssetsEditor->EditAsset(m_AssetType, m_AssetPath);
	}
	
	void DisplayAsset()
	{
		m_pAssetsEditor->DisplayAsset(m_AssetType, m_AssetPath);
	}
	
	virtual void Render()
	{
		if(m_pAssetsEditor->IsEditedAsset(m_AssetType, m_AssetPath))
		{
			m_pEditButton->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_HIGHLIGHT);
		}
		else
		{
			m_pEditButton->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL);
		}
		
		if(m_pAssetsEditor->IsDisplayedAsset(m_AssetType, m_AssetPath))
		{
			m_pDisplayButton->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_HIGHLIGHT);
		}
		else
		{
			m_pDisplayButton->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL);
		}
		
		CModAPI_ClientGui_HListLayout::Render();
	}
};

/* ASSETS EDITOR ******************************************************/

IModAPI_AssetsEditor *CreateAssetsEditor() { return new CModAPI_AssetsEditor; }

CModAPI_AssetsEditor::CModAPI_AssetsEditor()
{
	m_pGuiConfig = 0;
}

CModAPI_AssetsEditor::~CModAPI_AssetsEditor()
{
	delete m_pGuiAssetList;
	delete m_pGuiAssetEditor;
	delete m_pGuiView;
	delete m_pGuiToolbar;
	if(m_pGuiPopup) delete m_pGuiPopup;
	if(m_pGuiConfig) delete m_pGuiConfig;
}
	
void CModAPI_AssetsEditor::Init(CModAPI_Client_Graphics* pModAPIGraphics)
{
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pAssetsFile = Kernel()->RequestInterface<IModAPI_AssetsFile>();
	m_pModAPIGraphics = pModAPIGraphics;
	
	m_RenderTools.m_pGraphics = m_pGraphics;
		
	m_CursorTexture = Graphics()->LoadTexture("editor/cursor.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_ModEditorTexture = Graphics()->LoadTexture("modapi/modeditor.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_SkinTexture[0] = Graphics()->LoadTexture("skins/body/bear.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_SkinTexture[1] = Graphics()->LoadTexture("skins/marking/bear.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_SkinTexture[2] = Graphics()->LoadTexture("skins/decoration/hair.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_SkinTexture[3] = Graphics()->LoadTexture("skins/hands/standard.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_SkinTexture[4] = Graphics()->LoadTexture("skins/feet/standard.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_SkinTexture[5] = Graphics()->LoadTexture("skins/eyes/standard.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	
	m_pGuiConfig = new CModAPI_ClientGui_Config(Graphics(), RenderTools(), TextRender(), Input(), m_ModEditorTexture);
	
	
	int Margin = 5;
	
	int MenuBarHeight = 30;
	int PanelWidth = 250;
	int PanelHeight = 250;
	CModAPI_ClientGui_Rect ToolbarRect(Margin, Margin, Graphics()->ScreenWidth()-2*Margin, MenuBarHeight);
	CModAPI_ClientGui_Rect AssetListRect(Margin, ToolbarRect.y+ToolbarRect.h+Margin, PanelWidth, Graphics()->ScreenHeight()-3*Margin-ToolbarRect.h);
	CModAPI_ClientGui_Rect ViewRect(AssetListRect.x + AssetListRect.w + Margin, AssetListRect.y, Graphics()->ScreenWidth() - 2*PanelWidth - 4*Margin, Graphics()->ScreenHeight() - 4*Margin - PanelHeight - MenuBarHeight);
	CModAPI_ClientGui_Rect AssetEditorRect(ViewRect.x + ViewRect.w + Margin, AssetListRect.y, PanelWidth, AssetListRect.h);
	CModAPI_ClientGui_Rect TimelineRect(AssetListRect.x + AssetListRect.w + Margin, ViewRect.y + ViewRect.h + Margin, ViewRect.w, PanelHeight);
	
	m_pGuiToolbar = new CModAPI_ClientGui_HListLayout(m_pGuiConfig);
	m_pGuiToolbar->SetRect(ToolbarRect);
	m_pGuiToolbar->Add(new CModAPI_AssetsEditorGui_Button_ToolbarLoad(this));
	m_pGuiToolbar->Add(new CModAPI_AssetsEditorGui_Button_ToolbarSave(this));
	m_pGuiToolbar->Add(new CModAPI_AssetsEditorGui_Button_ToolbarExit(this));
	m_pGuiToolbar->Update();
	
	m_pGuiAssetList = new CModAPI_ClientGui_VListLayout(m_pGuiConfig);
	m_pGuiAssetList->SetRect(AssetListRect);
	RefreshAssetList();
	
	m_pGuiView = new CModAPI_AssetsEditorGui_View(this);
	m_pGuiView->SetRect(ViewRect);
	m_pGuiView->Update();
	
	m_pGuiTimeline = new CModAPI_AssetsEditorGui_Timeline(this);
	m_pGuiTimeline->SetRect(TimelineRect);
	m_pGuiTimeline->Update();
	
	m_pGuiAssetEditor = new CModAPI_AssetsEditorGui_Editor(this);
	m_pGuiAssetEditor->SetRect(AssetEditorRect);
	m_pGuiAssetEditor->Update();
	
	m_RefreshAssetEditor = false;
	m_ClosePopup = false;
	m_pGuiPopup = 0;
	
	m_ShowCursor = true;
	
	m_LastTime = -1.0f;
	m_Time = 0.0f;
	m_Paused = true;
	
	m_EditedAssetFrame = -1;
}

void CModAPI_AssetsEditor::RefreshAssetList()
{
	m_pGuiAssetList->Clear();
	
	bool ShowInternal = false;
	
	//Images
	m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, MODAPI_ASSETTYPE_IMAGE));
	if(ShowInternal)
	{
		for(int i=0; i<ModAPIGraphics()->m_ImagesCatalog.m_InternalAssets.size(); i++)
		{
			m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_IMAGE, CModAPI_AssetPath::Internal(i)));
		}
	}
	for(int i=0; i<ModAPIGraphics()->m_ImagesCatalog.m_ExternalAssets.size(); i++)
	{
		m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_IMAGE, CModAPI_AssetPath::External(i)));
	}
	
	//Sprites
	m_pGuiAssetList->AddSeparator();
	m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, MODAPI_ASSETTYPE_SPRITE));
	if(ShowInternal)
	{
		for(int i=0; i<ModAPIGraphics()->m_SpritesCatalog.m_InternalAssets.size(); i++)
		{
			m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_SPRITE, CModAPI_AssetPath::Internal(i)));
		}
	}
	for(int i=0; i<ModAPIGraphics()->m_SpritesCatalog.m_ExternalAssets.size(); i++)
	{
		m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_SPRITE, CModAPI_AssetPath::External(i)));
	}
	
	//Sprite lists
	m_pGuiAssetList->AddSeparator();
	m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, MODAPI_ASSETTYPE_SPRITE_LIST));
	if(ShowInternal)
	{
		for(int i=0; i<ModAPIGraphics()->m_SpritesCatalog.m_InternalLists.size(); i++)
		{
			m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_SPRITE_LIST, CModAPI_AssetPath::InternalList(i)));
		}
	}
	for(int i=0; i<ModAPIGraphics()->m_SpritesCatalog.m_ExternalLists.size(); i++)
	{
		m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_SPRITE_LIST, CModAPI_AssetPath::ExternalList(i)));
	}
	
	//Animations
	m_pGuiAssetList->AddSeparator();
	m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, MODAPI_ASSETTYPE_ANIMATION));
	if(ShowInternal)
	{
		for(int i=0; i<ModAPIGraphics()->m_AnimationsCatalog.m_InternalAssets.size(); i++)
		{
			m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_ANIMATION, CModAPI_AssetPath::Internal(i)));
		}
	}
	for(int i=0; i<ModAPIGraphics()->m_AnimationsCatalog.m_ExternalAssets.size(); i++)
	{
		m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_ANIMATION, CModAPI_AssetPath::External(i)));
	}
	
	//TeeAnimations
	m_pGuiAssetList->AddSeparator();
	m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, MODAPI_ASSETTYPE_TEEANIMATION));
	if(ShowInternal)
	{
		for(int i=0; i<ModAPIGraphics()->m_TeeAnimationsCatalog.m_InternalAssets.size(); i++)
		{
			m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_TEEANIMATION, CModAPI_AssetPath::Internal(i)));
		}
	}
	for(int i=0; i<ModAPIGraphics()->m_TeeAnimationsCatalog.m_ExternalAssets.size(); i++)
	{
		m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_TEEANIMATION, CModAPI_AssetPath::External(i)));
	}
	
	//Attaches
	m_pGuiAssetList->AddSeparator();
	m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, MODAPI_ASSETTYPE_ATTACH));
	if(ShowInternal)
	{
		for(int i=0; i<ModAPIGraphics()->m_AttachesCatalog.m_InternalAssets.size(); i++)
		{
			m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_ATTACH, CModAPI_AssetPath::Internal(i)));
		}
	}
	for(int i=0; i<ModAPIGraphics()->m_AttachesCatalog.m_ExternalAssets.size(); i++)
	{
		m_pGuiAssetList->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, MODAPI_ASSETTYPE_ATTACH, CModAPI_AssetPath::External(i)));
	}
	
	m_pGuiAssetList->Update();
}

void CModAPI_AssetsEditor::RefreshAssetEditor()
{
	m_RefreshAssetEditor = true;
}

void CModAPI_AssetsEditor::Render()
{
	Graphics()->MapScreen(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight());
	m_pGuiToolbar->Render();
	m_pGuiAssetList->Render();
	m_pGuiAssetEditor->Render();
	m_pGuiView->Render();
	m_pGuiTimeline->Render();
	
	if(m_pGuiPopup)
	{
		m_pGuiPopup->Render();
	}
	
	//Cursor
	if(m_ShowCursor)
	{
		Graphics()->TextureSet(m_CursorTexture);
		Graphics()->QuadsBegin();
		IGraphics::CQuadItem QuadItem(m_MousePos.x,m_MousePos.y, 16.0f, 16.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

void CModAPI_AssetsEditor::HideCursor()
{
	//~ m_ShowCursor = false;
}

void CModAPI_AssetsEditor::ShowCursor()
{
	//~ m_ShowCursor = true;
}

void CModAPI_AssetsEditor::TimeWrap()
{
	const float TimeShift = 0.5f;
	switch(m_ViewedAssetType)
	{
		case MODAPI_ASSETTYPE_ANIMATION:
		{
			CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_ViewedAssetPath);
			if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
			{
				float MaxTime = pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time;
				
				if(pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP)
					m_Time = fmod(m_Time, MaxTime);
				else
					m_Time = fmod(m_Time, MaxTime + TimeShift);
			}
			break;
		}
		case MODAPI_ASSETTYPE_TEEANIMATION:
		{
			CModAPI_Asset_TeeAnimation* pTeeAnimation = ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(m_ViewedAssetPath);
			if(pTeeAnimation)
			{
				float MaxTime = 0.0f;
				bool Loop = true;
				
				//Body
				{
					CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_BodyAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//BackFoot
				{
					CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_BackFootAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//FrontFoot
				{
					CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_FrontFootAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//BackHand
				{
					CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_BackHandAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//FrontHand
				{
					CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_FrontHandAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				
				if(Loop)
					m_Time = fmod(m_Time, MaxTime);
				else
					m_Time = fmod(m_Time, MaxTime + TimeShift);
			}
			break;
		}
		case MODAPI_ASSETTYPE_ATTACH:
		{
			CModAPI_Asset_Attach* pAttach = ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_ViewedAssetPath);
			if(pAttach)
			{
				float MaxTime = 0.0f;
				bool Loop = true;
				
				CModAPI_Asset_TeeAnimation* pTeeAnimation = ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(pAttach->m_TeeAnimationPath);
				if(pTeeAnimation)
				{
					//Body
					{
						CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_BodyAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//BackFoot
					{
						CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_BackFootAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//FrontFoot
					{
						CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_FrontFootAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//BackHand
					{
						CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_BackHandAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//FrontHand
					{
						CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pTeeAnimation->m_FrontHandAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
				}
				
				for(int i=0; i<pAttach->m_BackElements.size(); i++)
				{
					CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(pAttach->m_BackElements[i].m_AnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				
				if(Loop)
					m_Time = fmod(m_Time, MaxTime);
				else
					m_Time = fmod(m_Time, MaxTime + TimeShift);
			}
			break;
		}
	}
}

void CModAPI_AssetsEditor::UpdateAndRender()
{
	//Update time
	if(!m_Paused)
	{
		if(m_LastTime < 0.0f) m_Time = 0.0f;
		else m_Time += (Client()->LocalTime() - m_LastTime);
	}
	m_LastTime = Client()->LocalTime();
	TimeWrap();
	
	
	//Update popup state
	if(m_ClosePopup && m_pGuiPopup)
	{
		delete m_pGuiPopup;
		m_pGuiPopup = 0;
		m_ClosePopup = false;
	}
	
	if(m_RefreshAssetEditor)
	{
		m_pGuiAssetEditor->Refresh();
		m_RefreshAssetEditor = false;
	}
	
	//Input
	int Keys = 0x0;
		
	if(Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))
		Keys |= MODAPI_INPUT_CTRL;
		
	if(Input()->KeyIsPressed(KEY_LSHIFT) || Input()->KeyIsPressed(KEY_RSHIFT))
		Keys |= MODAPI_INPUT_SHIFT;
	
	if(Input()->KeyIsPressed(KEY_LALT) || Input()->KeyIsPressed(KEY_RALT))
		Keys |= MODAPI_INPUT_ALT;
	
	Input()->MouseRelative(&m_MouseDelta.x, &m_MouseDelta.y);

	m_MousePos += m_MouseDelta;
	m_MousePos.x = clamp(m_MousePos.x, 0.0f, static_cast<float>(Graphics()->ScreenWidth()));
	m_MousePos.y = clamp(m_MousePos.y, 0.0f, static_cast<float>(Graphics()->ScreenHeight()));
	
	if(m_pGuiPopup)
	{
		m_pGuiPopup->OnMouseOver(m_MousePos.x, m_MousePos.y, Keys);
		m_pGuiPopup->OnMouseMotion(m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiPopup->OnInputEvent();
	}
	else
	{
		m_pGuiToolbar->OnMouseOver(m_MousePos.x, m_MousePos.y, Keys);
		m_pGuiAssetList->OnMouseOver(m_MousePos.x, m_MousePos.y, Keys);
		m_pGuiAssetEditor->OnMouseOver(m_MousePos.x, m_MousePos.y, Keys);
		m_pGuiView->OnMouseOver(m_MousePos.x, m_MousePos.y, Keys);
		m_pGuiTimeline->OnMouseOver(m_MousePos.x, m_MousePos.y, Keys);
		
		m_pGuiToolbar->OnMouseMotion(m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiAssetList->OnMouseMotion(m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiAssetEditor->OnMouseMotion(m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiView->OnMouseMotion(m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiTimeline->OnMouseMotion(m_MouseDelta.x, m_MouseDelta.y, Keys);
		
		m_pGuiToolbar->OnInputEvent();
		m_pGuiAssetList->OnInputEvent();
		m_pGuiAssetEditor->OnInputEvent();
		m_pGuiView->OnInputEvent();
		m_pGuiTimeline->OnInputEvent();
	}
	
	if(Input()->KeyIsPressed(KEY_MOUSE_1))
	{
		if(m_MouseButton1 == 0)
		{
			m_MouseButton1 = 1;
			
			if(m_pGuiPopup)
			{
				m_pGuiPopup->OnMouseButtonClick(m_MousePos.x, m_MousePos.y);
			}
			else
			{
				m_pGuiToolbar->OnMouseButtonClick(m_MousePos.x, m_MousePos.y);
				m_pGuiAssetList->OnMouseButtonClick(m_MousePos.x, m_MousePos.y);
				m_pGuiAssetEditor->OnMouseButtonClick(m_MousePos.x, m_MousePos.y);
				m_pGuiView->OnMouseButtonClick(m_MousePos.x, m_MousePos.y);
				m_pGuiTimeline->OnMouseButtonClick(m_MousePos.x, m_MousePos.y);
			}
		}
	}
	else
	{
		if(m_MouseButton1 == 1)
		{
			m_MouseButton1 = 0;
			
			if(m_pGuiPopup)
			{
				m_pGuiPopup->OnMouseButtonRelease();
			}
			else
			{
				m_pGuiToolbar->OnMouseButtonRelease();
				m_pGuiAssetList->OnMouseButtonRelease();
				m_pGuiAssetEditor->OnMouseButtonRelease();
				m_pGuiView->OnMouseButtonRelease();
				m_pGuiTimeline->OnMouseButtonRelease();
			}
		}
	}
	
	//Rendering
	Render();

	Input()->Clear();
}

bool CModAPI_AssetsEditor::HasUnsavedData() const
{
	
}

void CModAPI_AssetsEditor::DisplayPopup(CModAPI_ClientGui_Popup* pWidget)
{
	if(m_pGuiPopup) delete m_pGuiPopup;
	
	m_pGuiPopup = pWidget;	
	m_pGuiPopup->SetRect(CModAPI_ClientGui_Rect(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight()));
	m_pGuiPopup->Update();
}

void CModAPI_AssetsEditor::ClosePopup()
{
	m_ClosePopup = true;
}

bool CModAPI_AssetsEditor::IsEditedAsset(int AssetType, CModAPI_AssetPath AssetPath)
{
	return (m_EditedAssetType == AssetType && m_EditedAssetPath == AssetPath);
}

bool CModAPI_AssetsEditor::IsDisplayedAsset(int AssetType, CModAPI_AssetPath AssetPath)
{
	return (m_ViewedAssetType == AssetType && m_ViewedAssetPath == AssetPath);
}

void CModAPI_AssetsEditor::EditAsset(int AssetType, CModAPI_AssetPath AssetPath)
{
	m_EditedAssetType = AssetType;
	m_EditedAssetPath = AssetPath;
	
	m_Time = 0.0f;
	
	m_pGuiAssetEditor->OnEditedAssetChange();
}

void CModAPI_AssetsEditor::EditAssetFrame(int FrameId)
{
	m_EditedAssetFrame = FrameId;
	
	if(m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_EditedAssetPath);
		if(pAnimation && m_EditedAssetFrame >= 0 && m_EditedAssetFrame < pAnimation->m_lKeyFrames.size())
		{
			m_Time = pAnimation->m_lKeyFrames[m_EditedAssetFrame].m_Time;
		}
		else
			m_Time = 0.0f;
	}
	else
		m_Time = 0.0f;
		
	m_Paused = true;
}

void CModAPI_AssetsEditor::EditAssetFirstFrame()
{
	if(m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_EditedAssetPath);
		if(pAnimation)
		{
			m_EditedAssetFrame = 0;
			m_Time = pAnimation->m_lKeyFrames[m_EditedAssetFrame].m_Time;
		}
		else
			m_Time = 0.0f;
	}
	else
		m_Time = 0.0f;
	
	m_Paused = true;
}

void CModAPI_AssetsEditor::EditAssetPrevFrame()
{
	if(m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_EditedAssetPath);
		if(pAnimation)
		{
			for(int i=pAnimation->m_lKeyFrames.size()-1; i>=0; i--)
			{
				if(pAnimation->m_lKeyFrames[i].m_Time < m_Time)
				{
					m_Time = pAnimation->m_lKeyFrames[i].m_Time;
					m_EditedAssetFrame = i;
					break;
				}
			}
		}
		else
			m_Time = 0.0f;
	}
	else
		m_Time = 0.0f;
	
	m_Paused = true;
}

void CModAPI_AssetsEditor::EditAssetNextFrame()
{
	if(m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_EditedAssetPath);
		if(pAnimation)
		{
			for(int i=0; i<pAnimation->m_lKeyFrames.size(); i++)
			{
				if(pAnimation->m_lKeyFrames[i].m_Time > m_Time)
				{
					m_Time = pAnimation->m_lKeyFrames[i].m_Time;
					m_EditedAssetFrame = i;
					break;
				}
			}
		}
		else
			m_Time = 0.0f;
	}
	else
		m_Time = 0.0f;
	
	m_Paused = true;
}

void CModAPI_AssetsEditor::EditAssetLastFrame()
{
	if(m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_EditedAssetPath);
		if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
		{
			m_EditedAssetFrame = pAnimation->m_lKeyFrames.size()-1;
			m_Time = pAnimation->m_lKeyFrames[m_EditedAssetFrame].m_Time;
		}
	}
		
	m_Paused = true;
}

void CModAPI_AssetsEditor::DisplayAsset(int AssetType, CModAPI_AssetPath AssetPath)
{
	m_ViewedAssetType = AssetType;
	m_ViewedAssetPath = AssetPath;
}

void CModAPI_AssetsEditor::NewAsset(int AssetType, CModAPI_AssetPath AssetPath)
{
	RefreshAssetList();
	DisplayAsset(AssetType, AssetPath);
	EditAsset(AssetType, AssetPath);
}

void CModAPI_AssetsEditor::DeleteAsset(int AssetType, CModAPI_AssetPath AssetPath)
{
	ModAPIGraphics()->DeleteAsset(AssetType, AssetPath);
	
	RefreshAssetList();
	
	m_EditedAssetPath = CModAPI_AssetPath::Null();
	m_ViewedAssetPath = CModAPI_AssetPath::Null();
}

void CModAPI_AssetsEditor::SetPause(bool Pause)
{
	m_Paused = Pause;
}

bool CModAPI_AssetsEditor::IsPaused()
{
	return m_Paused;
}

void CModAPI_AssetsEditor::SetTime(float Time)
{
	m_Time = Time;
	m_EditedAssetFrame = -1;
}

float CModAPI_AssetsEditor::GetTime()
{
	return m_Time;
}

void CModAPI_AssetsEditor::CloseEditor()
{
	g_Config.m_ClMode = MODAPI_CLIENTMODE_GAME;
}

void CModAPI_AssetsEditor::LoadAssetsFile()
{
	Client()->LoadAssetsFile("assets/test.assets");
	RefreshAssetList();
}

void CModAPI_AssetsEditor::SaveAssetsFile()
{
	ModAPIGraphics()->SaveInAssetsFile(Storage(), "assets/test.assets");
}
