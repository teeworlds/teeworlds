#include <base/tl/sorted_array.h>

#include <engine/shared/config.h>
#include <engine/shared/datafile.h>

#include <engine/client.h>
#include <engine/input.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>

#include <modapi/client/clientmode.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/label.h>
#include <modapi/client/gui/layout.h>
#include <modapi/client/gui/integer-edit.h>
#include <modapi/client/gui/text-edit.h>
#include <modapi/client/gui/expand.h>

#include <cstddef>

#include "assetseditor.h"
#include "popup.h"
#include "editor.h"
#include "view.h"
#include "timeline.h"

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
		m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_LoadAssets(
			m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_RIGHT
		));
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
		m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_SaveAssets(
			m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_RIGHT
		));
	}

public:
	CModAPI_AssetsEditorGui_Button_ToolbarSave(CModAPI_AssetsEditor* pAssetsEditor) :
		CModAPI_AssetsEditorGui_TextButton(pAssetsEditor, "Save", -1)
	{
		SetWidth(100);
	}
};

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
	int m_Source;

public:
	CModAPI_AssetsEditorGui_AssetListHeader(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, int Source) :
		CModAPI_ClientGui_HListLayout(pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetType(AssetType),
		m_Source(Source)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		#define ADD_ASSETTYPE_HEADER(TypeCode, TypeHeader) case TypeCode:\
			Add(new CModAPI_ClientGui_Label(m_pConfig, TypeHeader, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));\
			break;
		
		switch(m_AssetType)
		{
			//Search Tag: TAG_NEW_ASSET
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_IMAGE, "Images")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_SPRITE, "Sprites")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_SKELETON, "Skeletons")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_SKELETONSKIN, "Skeleton Skins")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_SKELETONANIMATION, "Skeleton Animations")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_CHARACTER, "Characters")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_CHARACTERPART, "Character Parts")
			ADD_ASSETTYPE_HEADER(CModAPI_AssetPath::TYPE_LIST, "Lists")
		}
		Add(new CAddButton(this));
		
		Update();
	}
	
	void AddAsset()
	{
		#define ON_NEW_ASSET(TypeName, AssetName) case TypeName::TypeId:\
		{\
			TypeName* pAsset = m_pAssetsEditor->AssetManager()->GetAssetCatalog<TypeName>()->NewAsset(&NewAssetPath, m_Source);\
			char aBuf[128];\
			str_format(aBuf, sizeof(aBuf), AssetName, NewAssetPath.GetId());\
			pAsset->SetName(aBuf);\
			m_pAssetsEditor->NewAsset(NewAssetPath);\
			break;\
		}
			
		CModAPI_AssetPath NewAssetPath;
		
		switch(m_AssetType)
		{
			case CModAPI_AssetPath::TYPE_IMAGE:
			{
				m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_AddImage(
					m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_RIGHT
				));
				break;
			}
			//Search Tag: TAG_NEW_ASSET
			ON_NEW_ASSET(CModAPI_Asset_Sprite, "sprite%d")
			ON_NEW_ASSET(CModAPI_Asset_Animation, "animation%d")
			ON_NEW_ASSET(CModAPI_Asset_TeeAnimation, "teeAnimation%d")
			ON_NEW_ASSET(CModAPI_Asset_Attach, "attach%d")
			ON_NEW_ASSET(CModAPI_Asset_LineStyle, "linestyle%d")
			ON_NEW_ASSET(CModAPI_Asset_Skeleton, "skeleton%d")
			ON_NEW_ASSET(CModAPI_Asset_SkeletonSkin, "skin%d")
			ON_NEW_ASSET(CModAPI_Asset_SkeletonAnimation, "animation%d")
			ON_NEW_ASSET(CModAPI_Asset_Character, "character%d")
			ON_NEW_ASSET(CModAPI_Asset_CharacterPart, "characterpart%d")
			ON_NEW_ASSET(CModAPI_Asset_List, "list%d")
		}
	}
};

class CModAPI_AssetsEditorGui_AssetListTitle : public CModAPI_ClientGui_ExternalTextButton
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	int m_AssetSubPath;
	int m_Tab;

protected:
	virtual void MouseClickAction()
	{
		if(m_AssetSubPath >= 0)
			m_pAssetsEditor->EditAssetSubItem(m_AssetPath, m_AssetSubPath, m_Tab);
		else
			m_pAssetsEditor->EditAsset(m_AssetPath);
		
		m_pAssetsEditor->DisplayAsset(m_AssetPath);
	}
	
public:
	CModAPI_AssetsEditorGui_AssetListTitle(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, char* pText, int IconId) :
		CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, pText, IconId),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_AssetSubPath(-1),
		m_Tab(0)
	{
		m_Centered = false;
	}
	
	void SetSubPath(int SubPath, int Tab)
	{
		m_AssetSubPath = SubPath;
		m_Tab = Tab;
	}
};

class CModAPI_AssetsEditorGui_AssetListItem : public CModAPI_ClientGui_HListLayout
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	CModAPI_AssetsEditorGui_AssetListTitle* m_Button;
	int m_Source;

public:
	CModAPI_AssetsEditorGui_AssetListItem(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Source = -1) :
		CModAPI_ClientGui_HListLayout(pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_Source(Source)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		int IconId = -1;
		switch(AssetPath.GetType())
		{
			case CModAPI_AssetPath::TYPE_IMAGE:
				IconId = MODAPI_ASSETSEDITOR_ICON_IMAGE;
				break;
			case CModAPI_AssetPath::TYPE_SPRITE:
				IconId = MODAPI_ASSETSEDITOR_ICON_SPRITE;
				break;
			case CModAPI_AssetPath::TYPE_SKELETON:
				IconId = MODAPI_ASSETSEDITOR_ICON_SKELETON;
				break;
			case CModAPI_AssetPath::TYPE_SKELETONSKIN:
				IconId = MODAPI_ASSETSEDITOR_ICON_SKELETONSKIN;
				break;
			case CModAPI_AssetPath::TYPE_SKELETONANIMATION:
				IconId = MODAPI_ASSETSEDITOR_ICON_SKELETONANIMATION;
				break;
			case CModAPI_AssetPath::TYPE_CHARACTER:
				IconId = MODAPI_ASSETSEDITOR_ICON_CHARACTER;
				break;
			case CModAPI_AssetPath::TYPE_CHARACTERPART:
				IconId = MODAPI_ASSETSEDITOR_ICON_CHARACTERPART;
				break;
		}
		
		char* pName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset::NAME, -1, 0);
		m_Button = new CModAPI_AssetsEditorGui_AssetListTitle(m_pAssetsEditor, m_AssetPath, pName, IconId);
		Add(m_Button);
		
		Update();
	}
	
	virtual void Render()
	{
		if(m_pAssetsEditor->IsEditedAsset(m_AssetPath))
			m_Button->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_HIGHLIGHT);
		else
			m_Button->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL);
		
		CModAPI_ClientGui_HListLayout::Render();
	}
};

class CModAPI_AssetsEditorGui_AssetListImage : public CModAPI_ClientGui_Expand
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	CModAPI_AssetsEditorGui_AssetListTitle* m_Button;
	int m_Source;
	bool m_SourceFound;

public:
	CModAPI_AssetsEditorGui_AssetListImage(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Source) :
		CModAPI_ClientGui_Expand(pAssetsEditor->m_pGuiConfig),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_Source(Source),
		m_SourceFound(false)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		char* pName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset::NAME, -1, 0);
		m_Button = new CModAPI_AssetsEditorGui_AssetListTitle(m_pAssetsEditor, m_AssetPath, pName, MODAPI_ASSETSEDITOR_ICON_IMAGE);
		SetTitle(m_Button);
		
		for(int i=0; i<m_pAssetsEditor->AssetManager()->GetNumAssets<CModAPI_Asset_Sprite>(m_Source); i++)
		{
			CModAPI_AssetPath SpritePath = CModAPI_AssetPath::Asset(CModAPI_Asset_Sprite::TypeId, m_Source, i);
			CModAPI_AssetPath ImagePath = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_AssetPath>(SpritePath, CModAPI_Asset_Sprite::IMAGEPATH, -1, 0);
			if(ImagePath == m_AssetPath)
			{
				CModAPI_AssetsEditorGui_AssetListItem* pItem = new CModAPI_AssetsEditorGui_AssetListItem(m_pAssetsEditor, SpritePath, m_Source);
				Add(pItem);
				m_SourceFound = true;
			}
		}
		
		Update();
	}
	
	bool SourceFound()
	{
		return m_SourceFound;
	}
};

class CModAPI_AssetsEditorGui_AssetListSkeleton : public CModAPI_ClientGui_Expand
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	CModAPI_AssetsEditorGui_AssetListTitle* m_Button;
	int m_Source;
	bool m_SourceFound;

public:
	CModAPI_AssetsEditorGui_AssetListSkeleton(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Source) :
		CModAPI_ClientGui_Expand(pAssetsEditor->m_pGuiConfig),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_Source(Source),
		m_SourceFound(false)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		char* pName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset::NAME, -1, 0);
		m_Button = new CModAPI_AssetsEditorGui_AssetListTitle(m_pAssetsEditor, m_AssetPath, pName, MODAPI_ASSETSEDITOR_ICON_SKELETON);
		SetTitle(m_Button);
		
		for(int i=0; i<m_pAssetsEditor->AssetManager()->GetNumAssets<CModAPI_Asset_SkeletonSkin>(m_Source); i++)
		{
			CModAPI_AssetPath SkeletonSkinPath = CModAPI_AssetPath::Asset(CModAPI_Asset_SkeletonSkin::TypeId, m_Source, i);
			CModAPI_AssetPath SkeletonPath = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_AssetPath>(SkeletonSkinPath, CModAPI_Asset_SkeletonSkin::SKELETONPATH, -1, 0);
			if(SkeletonPath == m_AssetPath)
			{
				Add(new CModAPI_AssetsEditorGui_AssetListItem(m_pAssetsEditor, SkeletonSkinPath));
				m_SourceFound = true;
			}
		}
		
		for(int i=0; i<m_pAssetsEditor->AssetManager()->GetNumAssets<CModAPI_Asset_SkeletonAnimation>(m_Source); i++)
		{
			CModAPI_AssetPath SkeletonAnimationPath = CModAPI_AssetPath::Asset(CModAPI_Asset_SkeletonAnimation::TypeId, m_Source, i);
			CModAPI_AssetPath SkeletonPath = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_AssetPath>(SkeletonAnimationPath, CModAPI_Asset_SkeletonAnimation::SKELETONPATH, -1, 0);
			if(SkeletonPath == m_AssetPath)
			{
				Add(new CModAPI_AssetsEditorGui_AssetListItem(m_pAssetsEditor, SkeletonAnimationPath));
				m_SourceFound = true;
			}
		}
		
		Update();
	}
	
	bool SourceFound()
	{
		return m_SourceFound;
	}
};

class CModAPI_AssetsEditorGui_AssetListCharacterPartType : public CModAPI_ClientGui_Expand
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	CModAPI_Asset_Character::CSubPath m_SubPath;
	CModAPI_AssetsEditorGui_AssetListTitle* m_Button;
	int m_Source;
	bool m_SourceFound;

public:
	CModAPI_AssetsEditorGui_AssetListCharacterPartType(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, CModAPI_Asset_Character::CSubPath SubPath, int Source) :
		CModAPI_ClientGui_Expand(pAssetsEditor->m_pGuiConfig),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_SubPath(SubPath),
		m_Source(Source),
		m_SourceFound(false)
	{
		SetHeight(m_pConfig->m_ButtonHeight);
		
		char* pName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset_Character::PART_NAME, SubPath.ConvertToInteger(), 0);
		m_Button = new CModAPI_AssetsEditorGui_AssetListTitle(m_pAssetsEditor, m_AssetPath, pName, -1);
		m_Button->SetSubPath(m_SubPath.ConvertToInteger(), CModAPI_AssetsEditorGui_Editor::TAB_CHARACTER_PARTS);
		SetTitle(m_Button);
		
		for(int i=0; i<m_pAssetsEditor->AssetManager()->GetNumAssets<CModAPI_Asset_CharacterPart>(m_Source); i++)
		{
			CModAPI_AssetPath CharacterPartPath = CModAPI_AssetPath::Asset(CModAPI_Asset_CharacterPart::TypeId, m_Source, i);
			CModAPI_AssetPath CharacterPath = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_AssetPath>(CharacterPartPath, CModAPI_Asset_CharacterPart::CHARACTERPATH, -1, CModAPI_AssetPath::Null());
			int CharacterPart = m_pAssetsEditor->AssetManager()->GetAssetValue<int>(CharacterPartPath, CModAPI_Asset_CharacterPart::CHARACTERPART, -1, 0);
			if(CharacterPath == m_AssetPath && CharacterPart == m_SubPath.ConvertToInteger())
			{
				Add(new CModAPI_AssetsEditorGui_AssetListItem(m_pAssetsEditor, CharacterPartPath));
				m_SourceFound = true;
			}
		}
		
		Update();
	}
	
	bool SourceFound()
	{
		return m_SourceFound;
	}
};

class CModAPI_AssetsEditorGui_AssetListCharacter : public CModAPI_ClientGui_Expand
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	CModAPI_AssetsEditorGui_AssetListTitle* m_Button;
	int m_Source;
	bool m_SourceFound;

public:
	CModAPI_AssetsEditorGui_AssetListCharacter(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Source) :
		CModAPI_ClientGui_Expand(pAssetsEditor->m_pGuiConfig),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_Source(Source),
		m_SourceFound(false)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		char* pName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset::NAME, -1, 0);
		m_Button = new CModAPI_AssetsEditorGui_AssetListTitle(m_pAssetsEditor, m_AssetPath, pName, MODAPI_ASSETSEDITOR_ICON_CHARACTER);
		SetTitle(m_Button);
		
		CModAPI_Asset_Character* pCharacter = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Character>(m_AssetPath);
		if(pCharacter)
		{
			for(int i=0; i<pCharacter->m_Parts.size(); i++)
			{
				CModAPI_AssetsEditorGui_AssetListCharacterPartType* pItem = new CModAPI_AssetsEditorGui_AssetListCharacterPartType(m_pAssetsEditor, m_AssetPath, CModAPI_Asset_Character::CSubPath::Part(i), m_Source);
				
				if(pItem->SourceFound())
				{
					Add(pItem);
					m_SourceFound = true;
				}
				else
					delete pItem;
			}
		}
		Update();
	}
	
	bool SourceFound()
	{
		return m_SourceFound;
	}
};

/* ASSETS EDITOR ******************************************************/

IModAPI_AssetsEditor *CreateAssetsEditor() { return new CModAPI_AssetsEditor; }

CModAPI_AssetsEditor::CModAPI_AssetsEditor()
{
	m_pGuiConfig = 0;
	m_pHintLabel = 0;
}

CModAPI_AssetsEditor::~CModAPI_AssetsEditor()
{
	delete m_pGuiAssetListTabs;
	delete m_pGuiAssetEditor;
	delete m_pGuiView;
	delete m_pGuiToolbar;
	for(int i=0; i<m_GuiPopups.size(); i++)
	{
		if(m_GuiPopups[i]) delete m_GuiPopups[i];
	}
	
	if(m_pGuiConfig) delete m_pGuiConfig;
}
	
void CModAPI_AssetsEditor::Init(CModAPI_AssetManager* pAssetManager, CModAPI_Client_Graphics* pModAPIGraphics)
{
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pAssetsFile = Kernel()->RequestInterface<IModAPI_AssetsFile>();
	m_pModAPIGraphics = pModAPIGraphics;
	m_pAssetManager = pAssetManager;
	
	m_RenderTools.m_pGraphics = m_pGraphics;
		
	m_CursorTexture = Graphics()->LoadTexture("editor/cursor.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_ModEditorTexture = Graphics()->LoadTexture("modapi/modeditor.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	
	m_pGuiConfig = new CModAPI_ClientGui_Config(Graphics(), RenderTools(), TextRender(), Input(), m_ModEditorTexture);
	m_pGuiConfig->m_fShowHint = CModAPI_AssetsEditor::ShowHint;
	m_pGuiConfig->m_pShowHintData = (void*) this;
	
	int Margin = 5;
	
	int MenuBarHeight = 30;
	int PanelWidth = 250;
	int PanelHeight = 250;
	CModAPI_ClientGui_Rect ToolbarRect(Margin, Margin, Graphics()->ScreenWidth()-2*Margin, MenuBarHeight);
	CModAPI_ClientGui_Rect AssetListRect(Margin, ToolbarRect.y+ToolbarRect.h+Margin, PanelWidth, Graphics()->ScreenHeight()-3*Margin-ToolbarRect.h);
	CModAPI_ClientGui_Rect ViewRect(AssetListRect.x + AssetListRect.w + Margin, AssetListRect.y, Graphics()->ScreenWidth() - 2*PanelWidth - 4*Margin, Graphics()->ScreenHeight() - 4*Margin - PanelHeight - MenuBarHeight);
	CModAPI_ClientGui_Rect AssetEditorRect(ViewRect.x + ViewRect.w + Margin, AssetListRect.y, PanelWidth, AssetListRect.h);
	CModAPI_ClientGui_Rect TimelineRect(AssetListRect.x + AssetListRect.w + Margin, ViewRect.y + ViewRect.h + Margin, ViewRect.w, PanelHeight);
	
	m_pHintLabel = new CModAPI_ClientGui_Label(m_pGuiConfig, "");
	
	m_pGuiToolbar = new CModAPI_ClientGui_HListLayout(m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME, MODAPI_CLIENTGUI_LAYOUTFILLING_LAST);
	m_pGuiToolbar->SetRect(ToolbarRect);
	m_pGuiToolbar->Add(new CModAPI_AssetsEditorGui_Button_ToolbarLoad(this));
	m_pGuiToolbar->Add(new CModAPI_AssetsEditorGui_Button_ToolbarSave(this));
	m_pGuiToolbar->Add(new CModAPI_AssetsEditorGui_Button_ToolbarExit(this));
	m_pGuiToolbar->Add(m_pHintLabel);
	m_pGuiToolbar->Update();
	
	m_pGuiAssetListTabs = new CModAPI_ClientGui_Tabs(m_pGuiConfig);
	m_pGuiAssetListTabs->SetRect(AssetListRect);
	
	m_pGuiAssetList[CModAPI_AssetPath::SRC_EXTERNAL] = new CModAPI_ClientGui_VListLayout(m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
	m_pGuiAssetListTabs->AddTab(m_pGuiAssetList[CModAPI_AssetPath::SRC_EXTERNAL], MODAPI_ASSETSEDITOR_ICON_EXTERNAL_ASSET, "External assets");
	
	m_pGuiAssetList[CModAPI_AssetPath::SRC_INTERNAL] = new CModAPI_ClientGui_VListLayout(m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
	m_pGuiAssetListTabs->AddTab(m_pGuiAssetList[CModAPI_AssetPath::SRC_INTERNAL], MODAPI_ASSETSEDITOR_ICON_INTERNAL_ASSET, "Internal assets");
	
	m_pGuiAssetList[CModAPI_AssetPath::SRC_MAP] = new CModAPI_ClientGui_VListLayout(m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
	m_pGuiAssetListTabs->AddTab(m_pGuiAssetList[CModAPI_AssetPath::SRC_MAP], MODAPI_ASSETSEDITOR_ICON_MAP_ASSET, "Map assets");
	
	m_pGuiAssetList[CModAPI_AssetPath::SRC_SKIN] = new CModAPI_ClientGui_VListLayout(m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
	m_pGuiAssetListTabs->AddTab(m_pGuiAssetList[CModAPI_AssetPath::SRC_SKIN], MODAPI_ASSETSEDITOR_ICON_SKIN_ASSET, "Skin assets");
	
	m_pGuiAssetListTabs->Update();
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
	m_EditorTab = -1;
	
	m_ShowCursor = true;
	
	m_LastTime = -1.0f;
	m_Time = 0.0f;
	m_Paused = true;
	
	m_EditedAssetSubPath = -1;
	m_AssetsListSource = CModAPI_AssetPath::SRC_EXTERNAL;
}

void CModAPI_AssetsEditor::ShowHint(const char* pText, void* pData)
{
	CModAPI_AssetsEditor* pThis = (CModAPI_AssetsEditor*) pData;
	if(pThis->m_pHintLabel)
	{
		pThis->m_pHintLabel->SetText(pText);
		pThis->m_Hint = true;
	}
}

void CModAPI_AssetsEditor::RefreshAssetList(int Source)
{
	m_pGuiAssetList[Source]->Clear();
	
	switch(Source)
	{
		case CModAPI_AssetPath::SRC_INTERNAL:
			m_pGuiAssetList[Source]->Add(new CModAPI_ClientGui_Label(m_pGuiConfig, "Internal assets", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			break;
		case CModAPI_AssetPath::SRC_EXTERNAL:
			m_pGuiAssetList[Source]->Add(new CModAPI_ClientGui_Label(m_pGuiConfig, "External assets", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			break;
		case CModAPI_AssetPath::SRC_MAP:
			m_pGuiAssetList[Source]->Add(new CModAPI_ClientGui_Label(m_pGuiConfig, "Map assets", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			break;
		case CModAPI_AssetPath::SRC_SKIN:
			m_pGuiAssetList[Source]->Add(new CModAPI_ClientGui_Label(m_pGuiConfig, "Skin assets", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));
			break;
	}
	
	//Images
	m_pGuiAssetList[Source]->AddSeparator();
	m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_Image::TypeId, Source));
	for(int s=0; s<CModAPI_AssetPath::NUM_SOURCES; s++)
	{
		for(int i=0; i<AssetManager()->GetNumAssets<CModAPI_Asset_Image>(s); i++)
		{
			CModAPI_AssetsEditorGui_AssetListImage* pItem = new CModAPI_AssetsEditorGui_AssetListImage(this, CModAPI_AssetPath::Asset(CModAPI_Asset_Image::TypeId, s, i), Source);
			if(pItem->SourceFound())
				m_pGuiAssetList[Source]->Add(pItem);
			else
				delete pItem;
		}
	}
	
	//Sprites
	{
		int nbAssets = AssetManager()->GetNumAssets<CModAPI_Asset_Sprite>(Source);
		if(nbAssets > 0)
		{
			m_pGuiAssetList[Source]->AddSeparator();
			m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_Sprite::TypeId, Source));
			for(int i=0; i<nbAssets; i++)
			{
				CModAPI_AssetPath SpritePath = CModAPI_AssetPath::Asset(CModAPI_Asset_Sprite::TypeId, Source, i);
				CModAPI_AssetPath ImagePath = AssetManager()->GetAssetValue<CModAPI_AssetPath>(SpritePath, CModAPI_Asset_Sprite::IMAGEPATH, -1, 0);
				if(!AssetManager()->GetAsset<CModAPI_Asset_Image>(ImagePath))
				{
					m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, SpritePath));
				}
			}
		}
	}
	
	//Skeletons
	m_pGuiAssetList[Source]->AddSeparator();
	m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_Skeleton::TypeId, Source));
	for(int s=0; s<CModAPI_AssetPath::NUM_SOURCES; s++)
	{
		for(int i=0; i<AssetManager()->GetNumAssets<CModAPI_Asset_Skeleton>(s); i++)
		{
			CModAPI_AssetsEditorGui_AssetListSkeleton* pItem = new CModAPI_AssetsEditorGui_AssetListSkeleton(this, CModAPI_AssetPath::Asset(CModAPI_Asset_Skeleton::TypeId, s, i), Source);
			if(pItem->SourceFound())
				m_pGuiAssetList[Source]->Add(pItem);
			else
				delete pItem;
		}
	}
	
	//SkeletonSkins
	{
		int nbAssets = AssetManager()->GetNumAssets<CModAPI_Asset_SkeletonSkin>(Source);
		if(nbAssets > 0)
		{
			m_pGuiAssetList[Source]->AddSeparator();
			m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_SkeletonSkin::TypeId, Source));
			for(int i=0; i<nbAssets; i++)
			{
				CModAPI_AssetPath SkeletonSkinPath = CModAPI_AssetPath::Asset(CModAPI_Asset_SkeletonSkin::TypeId, Source, i);
				CModAPI_AssetPath SkeletonPath = AssetManager()->GetAssetValue<CModAPI_AssetPath>(SkeletonSkinPath, CModAPI_Asset_SkeletonSkin::SKELETONPATH, -1, 0);
				if(!AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(SkeletonPath))
				{
					m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, SkeletonSkinPath));
				}
			}
		}
	}
	
	//SkeletonAnimations
	{
		int nbAssets = AssetManager()->GetNumAssets<CModAPI_Asset_SkeletonAnimation>(Source);
		if(nbAssets > 0)
		{
			m_pGuiAssetList[Source]->AddSeparator();
			m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_SkeletonAnimation::TypeId, Source));
			for(int i=0; i<nbAssets; i++)
			{
				CModAPI_AssetPath SkeletonAnimationPath = CModAPI_AssetPath::Asset(CModAPI_Asset_SkeletonAnimation::TypeId, Source, i);
				CModAPI_AssetPath SkeletonPath = AssetManager()->GetAssetValue<CModAPI_AssetPath>(SkeletonAnimationPath, CModAPI_Asset_SkeletonAnimation::SKELETONPATH, -1, 0);
				if(!AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(SkeletonPath))
				{
					m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, SkeletonAnimationPath));
				}
			}
		}
	}
	
	//Search Tag: TAG_NEW_ASSET
	
	//Character
	m_pGuiAssetList[Source]->AddSeparator();
	m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_Character::TypeId, Source));
	for(int s=0; s<CModAPI_AssetPath::NUM_SOURCES; s++)
	{
		for(int i=0; i<AssetManager()->GetNumAssets<CModAPI_Asset_Character>(s); i++)
		{
			CModAPI_AssetsEditorGui_AssetListCharacter* pItem = new CModAPI_AssetsEditorGui_AssetListCharacter(this, CModAPI_AssetPath::Asset(CModAPI_Asset_Character::TypeId, s, i), Source);
			if(pItem->SourceFound())
				m_pGuiAssetList[Source]->Add(pItem);
			else
				delete pItem;
		}
	}
	
	//CharacterPart
	{
		int nbAssets = AssetManager()->GetNumAssets<CModAPI_Asset_CharacterPart>(Source);
		if(nbAssets > 0)
		{
			m_pGuiAssetList[Source]->AddSeparator();
			m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, CModAPI_Asset_CharacterPart::TypeId, Source));
			for(int i=0; i<nbAssets; i++)
			{
				CModAPI_AssetPath CharacterPartPath = CModAPI_AssetPath::Asset(CModAPI_Asset_CharacterPart::TypeId, Source, i);
				CModAPI_AssetPath CharacterPath = AssetManager()->GetAssetValue<CModAPI_AssetPath>(CharacterPartPath, CModAPI_Asset_CharacterPart::CHARACTERPATH, -1, 0);
				CModAPI_Asset_Character* pCharacter = AssetManager()->GetAsset<CModAPI_Asset_Character>(CharacterPath);
				if(!pCharacter)
				{
					m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, CharacterPartPath));
				}
				else
				{
					int CharacterPart = AssetManager()->GetAssetValue<int>(CharacterPartPath, CModAPI_Asset_CharacterPart::CHARACTERPART, -1, 0);
					if(CharacterPart >= pCharacter->m_Parts.size() || CharacterPart < 0)
					{
						m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, CharacterPartPath));
					}
				}
			}
		}
	}
	
	#define REFRESH_ASSET_LIST(TypeName) \
	{\
		int nbAssets = AssetManager()->GetNumAssets<TypeName>(Source);\
		if(nbAssets > 0)\
		{\
			m_pGuiAssetList[Source]->AddSeparator();\
			m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListHeader(this, TypeName::TypeId, Source));\
			for(int i=0; i<nbAssets; i++)\
			{\
				m_pGuiAssetList[Source]->Add(new CModAPI_AssetsEditorGui_AssetListItem(this, CModAPI_AssetPath::Asset(TypeName::TypeId, Source, i)));\
			}\
		}\
	}
	
	REFRESH_ASSET_LIST(CModAPI_Asset_List)
	
	m_pGuiAssetList[Source]->Update();
}

void CModAPI_AssetsEditor::RefreshAssetList()
{
	RefreshAssetList(CModAPI_AssetPath::SRC_INTERNAL);
	RefreshAssetList(CModAPI_AssetPath::SRC_EXTERNAL);
	RefreshAssetList(CModAPI_AssetPath::SRC_MAP);
	RefreshAssetList(CModAPI_AssetPath::SRC_SKIN);
}

void CModAPI_AssetsEditor::RefreshAssetEditor(int Tab)
{
	m_EditorTab = Tab;
	m_RefreshAssetEditor = true;
}

void CModAPI_AssetsEditor::Render()
{
	Graphics()->MapScreen(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight());
	m_pGuiToolbar->Render();
	m_pGuiAssetListTabs->Render();
	m_pGuiAssetEditor->Render();
	m_pGuiView->Render();
	m_pGuiTimeline->Render();
	
	for(int i=0; i<m_GuiPopups.size(); i++)
	{
		if(m_GuiPopups[i])
			m_GuiPopups[i]->Render();
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
	switch(m_ViewedAssetPath.GetType())
	{
		case CModAPI_AssetPath::TYPE_ANIMATION:
		{
			CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(m_ViewedAssetPath);
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
		case CModAPI_AssetPath::TYPE_TEEANIMATION:
		{
			CModAPI_Asset_TeeAnimation* pTeeAnimation = AssetManager()->GetAsset<CModAPI_Asset_TeeAnimation>(m_ViewedAssetPath);
			if(pTeeAnimation)
			{
				float MaxTime = 0.0f;
				bool Loop = true;
				
				//Body
				{
					CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_BodyAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//BackFoot
				{
					CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_BackFootAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//FrontFoot
				{
					CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_FrontFootAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//BackHand
				{
					CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_BackHandAnimationPath);
					if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
					{
						MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
						Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
					}
				}
				//FrontHand
				{
					CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_FrontHandAnimationPath);
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
		case CModAPI_AssetPath::TYPE_ATTACH:
		{
			CModAPI_Asset_Attach* pAttach = AssetManager()->GetAsset<CModAPI_Asset_Attach>(m_ViewedAssetPath);
			if(pAttach)
			{
				float MaxTime = 0.0f;
				bool Loop = true;
				
				CModAPI_Asset_TeeAnimation* pTeeAnimation = AssetManager()->GetAsset<CModAPI_Asset_TeeAnimation>(pAttach->m_TeeAnimationPath);
				if(pTeeAnimation)
				{
					//Body
					{
						CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_BodyAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//BackFoot
					{
						CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_BackFootAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//FrontFoot
					{
						CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_FrontFootAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//BackHand
					{
						CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_BackHandAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
					//FrontHand
					{
						CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnimation->m_FrontHandAnimationPath);
						if(pAnimation && pAnimation->m_lKeyFrames.size() > 0)
						{
							MaxTime = max(MaxTime, pAnimation->m_lKeyFrames[pAnimation->m_lKeyFrames.size()-1].m_Time);
							Loop = Loop && (pAnimation->m_CycleType == MODAPI_ANIMCYCLETYPE_LOOP);
						}
					}
				}
				
				for(int i=0; i<pAttach->m_BackElements.size(); i++)
				{
					CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pAttach->m_BackElements[i].m_AnimationPath);
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
	if(Input()->KeyIsPressed(KEY_ESCAPE))
	{
		CloseEditor();
	}
	
	m_Hint = false;
	
	//Update time
	if(!m_Paused)
	{
		if(m_LastTime < 0.0f) m_Time = 0.0f;
		else m_Time += (Client()->LocalTime() - m_LastTime);
	}
	m_LastTime = Client()->LocalTime();
	TimeWrap();
	
	//Update popup state
	for(int i=0; i<m_GuiPopups.size(); i++)
	{
		if(m_GuiPopups[i]->IsClosed())
		{
			delete m_GuiPopups[i];
			m_GuiPopups.remove_index(i);
		}
	}
	
	if(m_RefreshAssetEditor)
	{
		m_pGuiAssetEditor->Refresh(m_EditorTab);
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
	
	if(m_GuiPopups.size() > 0)
	{
		for(int i=0; i<m_GuiPopups.size(); i++)
		{
			m_GuiPopups[i]->OnMouseOver(m_MousePos.x, m_MousePos.y, m_MouseDelta.x, m_MouseDelta.y, Keys);
			m_GuiPopups[i]->OnInputEvent();
		}
	}
	else
	{
		m_pGuiToolbar->OnMouseOver(m_MousePos.x, m_MousePos.y, m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiAssetListTabs->OnMouseOver(m_MousePos.x, m_MousePos.y, m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiAssetEditor->OnMouseOver(m_MousePos.x, m_MousePos.y, m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiView->OnMouseOver(m_MousePos.x, m_MousePos.y, m_MouseDelta.x, m_MouseDelta.y, Keys);
		m_pGuiTimeline->OnMouseOver(m_MousePos.x, m_MousePos.y, m_MouseDelta.x, m_MouseDelta.y, Keys);
		
		m_pGuiToolbar->OnInputEvent();
		m_pGuiAssetListTabs->OnInputEvent();
		m_pGuiAssetEditor->OnInputEvent();
		m_pGuiView->OnInputEvent();
		m_pGuiTimeline->OnInputEvent();
	}
	
	for(int i=0; i<3; i++)
	{
		int Button = KEY_MOUSE_1+i;
		int ModAPIButton;
		
		if(Input()->KeyIsPressed(Button))
		{
			if(m_MouseButton[i] == 0)
			{
				m_MouseButton[i] = 1;
				
				if(m_GuiPopups.size() > 0)
				{
					for(int i=0; i<m_GuiPopups.size(); i++)
					{
						m_GuiPopups[i]->OnButtonClick(m_MousePos.x, m_MousePos.y, Button);
					}
				}
				else
				{
					m_pGuiToolbar->OnButtonClick(m_MousePos.x, m_MousePos.y, Button);
					m_pGuiAssetListTabs->OnButtonClick(m_MousePos.x, m_MousePos.y, Button);
					m_pGuiAssetEditor->OnButtonClick(m_MousePos.x, m_MousePos.y, Button);
					m_pGuiView->OnButtonClick(m_MousePos.x, m_MousePos.y, Button);
					m_pGuiTimeline->OnButtonClick(m_MousePos.x, m_MousePos.y, Button);
				}
			}
		}
		else
		{
			if(m_MouseButton[i] == 1)
			{
				m_MouseButton[i] = 0;
				
				if(m_GuiPopups.size() > 0)
				{
					for(int i=0; i<m_GuiPopups.size(); i++)
					{
						m_GuiPopups[i]->OnButtonRelease(Button);
					}
				}
				else
				{
					m_pGuiToolbar->OnButtonRelease(Button);
					m_pGuiAssetListTabs->OnButtonRelease(Button);
					m_pGuiAssetEditor->OnButtonRelease(Button);
					m_pGuiView->OnButtonRelease(Button);
					m_pGuiTimeline->OnButtonRelease(Button);
				}
			}
		}
	}
	
	//Rendering
	Render();
	
	if(!m_Hint)
	{
		m_pHintLabel->SetText("");
	}

	Input()->Clear();
}

bool CModAPI_AssetsEditor::HasUnsavedData() const
{
	
}

void CModAPI_AssetsEditor::DisplayPopup(CModAPI_ClientGui_Popup* pWidget)
{
	m_GuiPopups.add(pWidget);
}

bool CModAPI_AssetsEditor::IsEditedAsset(CModAPI_AssetPath AssetPath)
{
	return (m_EditedAssetPath == AssetPath);
}

bool CModAPI_AssetsEditor::IsDisplayedAsset(CModAPI_AssetPath AssetPath)
{
	return (m_ViewedAssetPath == AssetPath);
}

void CModAPI_AssetsEditor::EditAsset(CModAPI_AssetPath AssetPath)
{
	m_EditedAssetPath = AssetPath;
	m_EditedAssetSubPath = -1;
	m_Paused = true;
	RefreshAssetEditor(-1);
}

void CModAPI_AssetsEditor::EditAssetSubItem(CModAPI_AssetPath AssetPath, int SubPath, int Tab)
{
	m_EditedAssetPath = AssetPath;
	m_EditedAssetSubPath = SubPath;
	m_Paused = true;
	RefreshAssetEditor(Tab);
}

void CModAPI_AssetsEditor::EditAssetFirstFrame()
{
	m_Time = 0.0f;
}

void CModAPI_AssetsEditor::EditAssetPrevFrame()
{
	if(m_EditedAssetPath.GetType() == CModAPI_AssetPath::TYPE_SKELETONANIMATION)
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = AssetManager()->GetAsset<CModAPI_Asset_SkeletonAnimation>(m_EditedAssetPath);
		if(pSkeletonAnimation)
		{
			float MaxTime = m_Time;
			m_Time = 0.0f;
			
			//Bones
			for(int i=0; i<pSkeletonAnimation->m_BoneAnimations.size(); i++)
			{
				for(int j=0; j<pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames.size(); j++)
				{
					float Time = pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames[j].m_Time / static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
					
					if(Time >= MaxTime)
						break;
						
					if(Time > m_Time)
						m_Time = Time;
				}
			}
			
			//Layers
			for(int i=0; i<pSkeletonAnimation->m_LayerAnimations.size(); i++)
			{
				for(int j=0; j<pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames.size(); j++)
				{
					float Time = pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames[j].m_Time / static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
					
					if(Time >= MaxTime)
						break;
						
					if(Time > m_Time)
						m_Time = Time;
				}
			}
		}
	}
}

void CModAPI_AssetsEditor::EditAssetNextFrame()
{
	if(m_EditedAssetPath.GetType() == CModAPI_AssetPath::TYPE_SKELETONANIMATION)
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = AssetManager()->GetAsset<CModAPI_Asset_SkeletonAnimation>(m_EditedAssetPath);
		if(pSkeletonAnimation)
		{
			float MinTime = m_Time;
			bool Initialized = false;
			
			//Bones
			for(int i=0; i<pSkeletonAnimation->m_BoneAnimations.size(); i++)
			{
				for(int j=pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames.size()-1; j>=0; j--)
				{
					float Time = pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames[j].m_Time / static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
					
					if(Time <= MinTime)
						break;
						
					if(Time < m_Time || !Initialized)
					{
						Initialized = true;
						m_Time = Time;
					}
				}
			}
			
			//Layers
			for(int i=0; i<pSkeletonAnimation->m_LayerAnimations.size(); i++)
			{
				for(int j=pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames.size()-1; j>=0; j--)
				{
					float Time = pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames[j].m_Time / static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
					
					if(Time <= MinTime)
						break;
						
					if(Time < m_Time || !Initialized)
					{
						Initialized = true;
						m_Time = Time;
					}
				}
			}
		}
	}
}

void CModAPI_AssetsEditor::EditAssetLastFrame()
{
	if(m_EditedAssetPath.GetType() == CModAPI_AssetPath::TYPE_SKELETONANIMATION)
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = AssetManager()->GetAsset<CModAPI_Asset_SkeletonAnimation>(m_EditedAssetPath);
		if(pSkeletonAnimation)
		{
			m_Time = 0.0f;
			
			//Bones
			for(int i=0; i<pSkeletonAnimation->m_BoneAnimations.size(); i++)
			{
				if(pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames.size() > 0)
				{
					int KeyId = pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames.size()-1;
					float Time = pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames[KeyId].m_Time / static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
					if(Time > m_Time)
						m_Time = Time;
				}
			}
			
			//Layers
			for(int i=0; i<pSkeletonAnimation->m_LayerAnimations.size(); i++)
			{
				if(pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames.size() > 0)
				{
					int KeyId = pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames.size()-1;
					float Time = pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames[KeyId].m_Time / static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
					if(Time > m_Time)
						m_Time = Time;
				}
			}
		}
	}
}

void CModAPI_AssetsEditor::DisplayAsset(CModAPI_AssetPath AssetPath)
{
	m_ViewedAssetPath = AssetPath;
}

void CModAPI_AssetsEditor::NewAsset(CModAPI_AssetPath AssetPath)
{
	RefreshAssetList();
	DisplayAsset(AssetPath);
	EditAsset(AssetPath);
}

void CModAPI_AssetsEditor::DeleteAsset(CModAPI_AssetPath AssetPath)
{
	AssetManager()->DeleteAsset(AssetPath);
	
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
}

float CModAPI_AssetsEditor::GetTime()
{
	return m_Time;
}

void CModAPI_AssetsEditor::CloseEditor()
{
	g_Config.m_ClMode = MODAPI_CLIENTMODE_GAME;
}

void CModAPI_AssetsEditor::LoadAssetsFile(const char* pFilename)
{
	Client()->LoadAssetsFile(pFilename);
	RefreshAssetList();
}
