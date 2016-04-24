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

#include "editor.h"

const char* CModAPI_AssetsEditorGui_Editor::CAssetEdit::m_aNoneText = "None";
const char* CModAPI_AssetsEditorGui_Editor::CBoneEdit::m_aNoneText = "None";
const char* CModAPI_AssetsEditorGui_Editor::CLayerEdit::m_aNoneText = "None";

const char* g_aCycleTypeText[] = {
    "Clamp",
    "Loop",
};
const char* g_aSpriteAlignText[] = {
    "World",
    "Bone",
};
const char* g_aLayerStateText[] = {
    "Visible",
    "Hidden",
};
const char* g_aBoneAlignText[] = {
    "Parent",
    "World",
    "Aim",
    "Motion",
    "Hook",
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
	CModAPI_AssetPath m_AssetPath;
	
	CEditButton* m_pEditButton;
	CDisplayButton* m_pDisplayButton;

public:
	CModAPI_AssetsEditorGui_AssetListItem(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath) :
		CModAPI_ClientGui_HListLayout(pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath)
	{		
		SetHeight(m_pConfig->m_ButtonHeight);
		
		char* aName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset::NAME, -1, 0);
		Add(new CItemListButton(this, aName));
		
		m_pDisplayButton = new CDisplayButton(this);
		Add(m_pDisplayButton);
		
		m_pEditButton = new CEditButton(this);
		Add(m_pEditButton);
		
		Update();
	}
	
	void EditAsset()
	{
		m_pAssetsEditor->EditAsset(m_AssetPath);
	}
	
	void DisplayAsset()
	{
		m_pAssetsEditor->DisplayAsset(m_AssetPath);
	}
	
	virtual void Render()
	{
		if(m_pAssetsEditor->IsEditedAsset(m_AssetPath))
		{
			m_pEditButton->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_HIGHLIGHT);
		}
		else
		{
			m_pEditButton->SetButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL);
		}
		
		if(m_pAssetsEditor->IsDisplayedAsset(m_AssetPath))
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

CModAPI_AssetsEditorGui_Editor::CModAPI_AssetsEditorGui_Editor(CModAPI_AssetsEditor* pAssetsEditor) :
	CModAPI_ClientGui_Tabs(pAssetsEditor->m_pGuiConfig),
	m_LastEditedAssetType(-1),
	m_pAssetsEditor(pAssetsEditor)
{
	for(int i=0; i<NUM_TABS; i++)
	{
		m_pTabs[i] = 0;
	}
}

void CModAPI_AssetsEditorGui_Editor::AddAssetTabCommons(CModAPI_ClientGui_VListLayout* pList)
{
	pList->Clear();
	AddTextField(pList, CModAPI_Asset::NAME, -1, sizeof(CModAPI_Asset::m_aName), "Asset name:");
	
	pList->Add(new CModAPI_AssetsEditorGui_Editor::CDeleteAsset(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath));
	
	pList->AddSeparator();
}

void CModAPI_AssetsEditorGui_Editor::AddSubTitle(CModAPI_ClientGui_VListLayout* pList, const char* pText)
{
	pList->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, pText, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
}

void CModAPI_AssetsEditorGui_Editor::AddField(CModAPI_ClientGui_VListLayout* pList, CModAPI_ClientGui_Widget* pWidget, const char* pLabelText)
{
	if(pLabelText)
	{
		CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
		pLayout->SetHeight(m_pConfig->m_ButtonHeight);
		pList->Add(pLayout);
		
		CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, pLabelText);
		pLayout->Add(pLabel);
		
		pLayout->Add(pWidget);
	}
	else
	{
		pList->Add(pWidget);
	}
}

void CModAPI_AssetsEditorGui_Editor::AddFloatField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CFloatAssetMemberEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CFloatAssetMemberEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddAngleField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CAnglerEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CAnglerEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddFloat2Field(CModAPI_ClientGui_VListLayout* pList, int Member1, int Member2, int SubId, const char* pLabelText)
{
	CModAPI_ClientGui_HListLayout* pWidget = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
	pWidget->SetHeight(m_pConfig->m_ButtonHeight);
	
	CModAPI_AssetsEditorGui_Editor::CFloatAssetMemberEdit* pEdit1 = new CModAPI_AssetsEditorGui_Editor::CFloatAssetMemberEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member1, SubId);
	pWidget->Add(pEdit1);
	
	CModAPI_AssetsEditorGui_Editor::CFloatAssetMemberEdit* pEdit2 = new CModAPI_AssetsEditorGui_Editor::CFloatAssetMemberEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member2, SubId);
	pWidget->Add(pEdit2);
			
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddIntegerField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CIntegerAssetMemberEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CIntegerAssetMemberEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddCycleField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CEnumEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CEnumEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, g_aCycleTypeText, sizeof(g_aCycleTypeText)/sizeof(const char*));
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddSpriteAlignField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CEnumEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CEnumEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, g_aSpriteAlignText, sizeof(g_aSpriteAlignText)/sizeof(const char*));
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddLayerStateField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CEnumEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CEnumEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, g_aLayerStateText, sizeof(g_aLayerStateText)/sizeof(const char*));
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddBoneAlignField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CEnumEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CEnumEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, g_aBoneAlignText, sizeof(g_aBoneAlignText)/sizeof(const char*));
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddColorField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CColorEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CColorEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddTextField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, int TextSize, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CTextAssetMemberEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CTextAssetMemberEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, TextSize);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddAssetField(CModAPI_ClientGui_VListLayout* pList, int Member, int AssetType, int SubId, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CAssetEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CAssetEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, AssetType);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddBoneField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, CModAPI_AssetPath SkeletonPath, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CBoneEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CBoneEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, SkeletonPath);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::AddLayerField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, CModAPI_AssetPath SkeletonPath, const char* pLabelText)
{
	CModAPI_AssetsEditorGui_Editor::CLayerEdit* pWidget = new CModAPI_AssetsEditorGui_Editor::CLayerEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, SkeletonPath);
	
	AddField(pList, pWidget, pLabelText);
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_Image_Asset(bool KeepStatus)
{
	CModAPI_Asset_Image* pImage = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Image>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pImage)
		return;
	
	//Size
	{
		CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
		pLayout->SetHeight(m_pConfig->m_ButtonHeight);
		m_pTabs[TAB_ASSET]->Add(pLayout);
		
		CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Size:");
		pLayout->Add(pLabel);
		
		int Width = m_pAssetsEditor->AssetManager()->GetAssetValue<int>(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_Image::WIDTH, -1, 0);
		int Height = m_pAssetsEditor->AssetManager()->GetAssetValue<int>(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_Image::HEIGHT, -1, 0);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d x %d", Width, Height);
		
		CModAPI_ClientGui_Label* pValue = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, aBuf);
		pLayout->Add(pValue);
	}
	
	AddIntegerField(m_pTabs[TAB_ASSET], CModAPI_Asset_Image::GRIDWIDTH, -1, "Grid width:");
	AddIntegerField(m_pTabs[TAB_ASSET], CModAPI_Asset_Image::GRIDHEIGHT, -1, "Grid height:");
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_Sprite_Asset(bool KeepStatus)
{	
	AddAssetField(m_pTabs[TAB_ASSET], CModAPI_Asset_Sprite::IMAGEPATH, CModAPI_AssetPath::TYPE_IMAGE, -1, "Image:");
	AddIntegerField(m_pTabs[TAB_ASSET], CModAPI_Asset_Sprite::X, -1, "X:");
	AddIntegerField(m_pTabs[TAB_ASSET], CModAPI_Asset_Sprite::Y, -1, "Y:");
	AddIntegerField(m_pTabs[TAB_ASSET], CModAPI_Asset_Sprite::WIDTH, -1, "Width:");
	AddIntegerField(m_pTabs[TAB_ASSET], CModAPI_Asset_Sprite::HEIGHT, -1, "Height:");
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_Skeleton_Asset(bool KeepStatus)
{	
	AddAssetField(m_pTabs[TAB_ASSET], CModAPI_Asset_Skeleton::PARENTPATH, CModAPI_AssetPath::TYPE_SKELETON, -1, "Parent:");
	AddAssetField(m_pTabs[TAB_ASSET], CModAPI_Asset_Skeleton::SKINPATH, CModAPI_AssetPath::TYPE_SKELETONSKIN, -1, "Default Skin:");
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_Skeleton_Bones(bool KeepStatus)
{
	float Scroll = 0;
	if(KeepStatus && m_pLists[LIST_SKELETON_BONES])
	{
		Scroll = m_pLists[LIST_SKELETON_BONES]->GetScrollPos();
	}
	m_pTabs[TAB_SKELETON_BONES]->Clear();
	m_pLists[LIST_SKELETON_BONES] = 0;
	
	CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pSkeleton)
		return;
		
	m_pLists[LIST_SKELETON_BONES] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
	m_pLists[LIST_SKELETON_BONES]->SetHeight(m_Rect.h/2);
	m_pTabs[TAB_SKELETON_BONES]->Add(m_pLists[LIST_SKELETON_BONES]);
	m_pLists[LIST_SKELETON_BONES]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Bones", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	
	for(int i=0; i<pSkeleton->m_Bones.size(); i++)
	{
		CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_BONE);
		pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, pSkeleton->GetBonePath(i));
		pItem->SetText(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_Skeleton::BONE_NAME, pSkeleton->GetBonePath(i));
		m_pLists[LIST_SKELETON_BONES]->Add(pItem);
	}
	if(KeepStatus)
	{
		m_pLists[LIST_SKELETON_BONES]->SetScrollPos(Scroll);
	}
	
	m_pTabs[TAB_SKELETON_BONES]->Add(new CAddSubItem(
		m_pAssetsEditor,
		m_pAssetsEditor->m_EditedAssetPath,
		CModAPI_Asset_Skeleton::CSubPath::TYPE_BONE,
		"Add bone",
		MODAPI_ASSETSEDITOR_ICON_INCREASE,
		TAB_SKELETON_BONES
	));

	m_pTabs[TAB_SKELETON_BONES]->AddSeparator();	
		
	if(m_pAssetsEditor->m_EditedAssetSubPath >= 0 && CModAPI_Asset_Skeleton::CSubPath(m_pAssetsEditor->m_EditedAssetSubPath).GetType() == CModAPI_Asset_Skeleton::CSubPath::TYPE_BONE)
	{
		AddTextField(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_NAME, m_pAssetsEditor->m_EditedAssetSubPath, sizeof(CModAPI_Asset_Skeleton::CBone::m_aName), "Name:");
		AddColorField(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_COLOR, m_pAssetsEditor->m_EditedAssetSubPath, "Color:");
		AddBoneField(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_PARENT, m_pAssetsEditor->m_EditedAssetSubPath, m_pAssetsEditor->m_EditedAssetPath, "Parent:");
		AddFloatField(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_LENGTH, m_pAssetsEditor->m_EditedAssetSubPath, "Length:");
		AddFloatField(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_ANCHOR, m_pAssetsEditor->m_EditedAssetSubPath, "Anchor:");
		AddFloat2Field(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_TRANSLATION_X, CModAPI_Asset_Skeleton::BONE_TRANSLATION_Y, m_pAssetsEditor->m_EditedAssetSubPath, "Translation:");
		AddFloat2Field(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_SCALE_X, CModAPI_Asset_Skeleton::BONE_SCALE_Y, m_pAssetsEditor->m_EditedAssetSubPath, "Scale:");
		AddAngleField(m_pTabs[TAB_SKELETON_BONES], CModAPI_Asset_Skeleton::BONE_ANGLE, m_pAssetsEditor->m_EditedAssetSubPath, "Angle:");	
	}
}
	
void CModAPI_AssetsEditorGui_Editor::RefreshTab_Skeleton_Layers(bool KeepStatus)
{
	float Scroll = 0;
	if(KeepStatus && m_pLists[LIST_SKELETON_LAYERS])
	{
		Scroll = m_pLists[LIST_SKELETON_LAYERS]->GetScrollPos();
	}
	m_pTabs[TAB_SKELETON_LAYERS]->Clear();
			
	CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pSkeleton)
		return;
			
	m_pLists[LIST_SKELETON_LAYERS] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
	m_pLists[LIST_SKELETON_LAYERS]->SetHeight(m_Rect.h/2);
	m_pTabs[TAB_SKELETON_LAYERS]->Add(m_pLists[LIST_SKELETON_LAYERS]);
	m_pLists[LIST_SKELETON_LAYERS]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Layers", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	
	for(int i=0; i<pSkeleton->m_Layers.size(); i++)
	{
		CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_LAYERS);
		pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, pSkeleton->GetLayerPath(i));
		pItem->SetText(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_Skeleton::LAYER_NAME, pSkeleton->GetLayerPath(i));
		m_pLists[LIST_SKELETON_LAYERS]->Add(pItem);
	}
	if(KeepStatus)
	{
		m_pLists[LIST_SKELETON_LAYERS]->SetScrollPos(Scroll);
	}

	m_pTabs[TAB_SKELETON_LAYERS]->Add(new CAddSubItem(
		m_pAssetsEditor,
		m_pAssetsEditor->m_EditedAssetPath,
		CModAPI_Asset_Skeleton::CSubPath::TYPE_LAYER,
		"Add layer",
		MODAPI_ASSETSEDITOR_ICON_INCREASE,
		TAB_SKELETON_LAYERS
	));
	
	m_pTabs[TAB_SKELETON_LAYERS]->AddSeparator();
		
	if(m_pAssetsEditor->m_EditedAssetSubPath >= 0 && CModAPI_Asset_Skeleton::CSubPath(m_pAssetsEditor->m_EditedAssetSubPath).GetType() == CModAPI_Asset_Skeleton::CSubPath::TYPE_LAYER)
	{
		AddTextField(m_pTabs[TAB_SKELETON_LAYERS], CModAPI_Asset_Skeleton::LAYER_NAME, m_pAssetsEditor->m_EditedAssetSubPath, sizeof(CModAPI_Asset_Skeleton::CLayer::m_aName), "Name:");
	}
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_SkeletonSkin_Asset(bool KeepStatus)
{
	AddAssetField(m_pTabs[TAB_ASSET], CModAPI_Asset_SkeletonSkin::SKELETONPATH, CModAPI_AssetPath::TYPE_SKELETON, -1, "Skeleton:");
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_SkeletonSkin_Sprites(bool KeepStatus)
{
	float Scroll = 0;
	if(KeepStatus && m_pLists[LIST_SKELETONSKIN_SPRITES])
	{
		Scroll = m_pLists[LIST_SKELETONSKIN_SPRITES]->GetScrollPos();
	}
	m_pTabs[TAB_SKELETONSKIN_SPRITES]->Clear();
			
	CModAPI_Asset_SkeletonSkin* pSkeletonSkin = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_SkeletonSkin>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pSkeletonSkin)
		return;
		
	m_pLists[LIST_SKELETONSKIN_SPRITES] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
	m_pLists[LIST_SKELETONSKIN_SPRITES]->SetHeight(m_Rect.h/2);
	m_pTabs[TAB_SKELETONSKIN_SPRITES]->Add(m_pLists[LIST_SKELETONSKIN_SPRITES]);
	m_pLists[LIST_SKELETONSKIN_SPRITES]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Sprites", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	
	for(int i=0; i<pSkeletonSkin->m_Sprites.size(); i++)
	{
		CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_SPRITE);
		pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, pSkeletonSkin->GetSpritePath(i));
		pItem->SetText(pSkeletonSkin->m_Sprites[i].m_SpritePath, CModAPI_Asset::NAME, -1);
		m_pLists[LIST_SKELETONSKIN_SPRITES]->Add(pItem);
	}
	if(KeepStatus)
	{
		m_pLists[LIST_SKELETONSKIN_SPRITES]->SetScrollPos(Scroll);
	}

	m_pTabs[TAB_SKELETONSKIN_SPRITES]->Add(new CAddSubItem(
		m_pAssetsEditor,
		m_pAssetsEditor->m_EditedAssetPath,
		CModAPI_Asset_SkeletonSkin::CSubPath::TYPE_SPRITE,
		"Add sprite",
		MODAPI_ASSETSEDITOR_ICON_INCREASE,
		TAB_SKELETONSKIN_SPRITES
	));
	
	m_pTabs[TAB_SKELETONSKIN_SPRITES]->AddSeparator();	
		
	if(m_pAssetsEditor->m_EditedAssetSubPath >= 0)
	{
		AddAssetField(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_PATH, CModAPI_AssetPath::TYPE_SPRITE, m_pAssetsEditor->m_EditedAssetSubPath, "Sprite:");
		AddBoneField(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_BONE, m_pAssetsEditor->m_EditedAssetSubPath, pSkeletonSkin->m_SkeletonPath, "Bone:");
		AddLayerField(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_LAYER, m_pAssetsEditor->m_EditedAssetSubPath, pSkeletonSkin->m_SkeletonPath, "Layer:");
		AddSpriteAlignField(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_ALIGNMENT, m_pAssetsEditor->m_EditedAssetSubPath, "Alignment:");
		AddFloat2Field(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_TRANSLATION_X, CModAPI_Asset_SkeletonSkin::SPRITE_TRANSLATION_Y, m_pAssetsEditor->m_EditedAssetSubPath, "Translation:");
		AddFloat2Field(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_SCALE_X, CModAPI_Asset_SkeletonSkin::SPRITE_SCALE_Y, m_pAssetsEditor->m_EditedAssetSubPath, "Scale:");
		AddAngleField(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_ANGLE, m_pAssetsEditor->m_EditedAssetSubPath, "Angle:");
		AddFloatField(m_pTabs[TAB_SKELETONSKIN_SPRITES], CModAPI_Asset_SkeletonSkin::SPRITE_ANCHOR, m_pAssetsEditor->m_EditedAssetSubPath, "Anchor:");
	}
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_SkeletonAnimation_Asset(bool KeepStatus)
{
	AddAssetField(m_pTabs[TAB_ASSET], CModAPI_Asset_SkeletonAnimation::SKELETONPATH, CModAPI_AssetPath::TYPE_SKELETON, -1, "Skeleton:");
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_SkeletonAnimation_Animations(bool KeepStatus)
{
	float Scroll = 0;
	if(KeepStatus && m_pLists[LIST_SKELETONANIMATION_ANIMATIONS])
	{
		Scroll = m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->GetScrollPos();
	}
	m_pTabs[TAB_SKELETONANIMATION_ANIMATIONS]->Clear();
			
	CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_SkeletonAnimation>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pSkeletonAnimation)
		return;
			
	CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(pSkeletonAnimation->m_SkeletonPath);
	if(!pSkeleton)
		return;
		
	m_pLists[LIST_SKELETONANIMATION_ANIMATIONS] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
	m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->SetHeight(m_Rect.h/2);
	m_pTabs[TAB_SKELETONANIMATION_ANIMATIONS]->Add(m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]);
	
	m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Bone Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	for(int i=0; i<pSkeletonAnimation->m_BoneAnimations.size(); i++)
	{
		CModAPI_Asset_Skeleton::CSubPath BonePath = CModAPI_Asset_Skeleton::CSubPath::Bone(pSkeletonAnimation->m_BoneAnimations[i].m_BonePath.GetId());
		
		CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_SKELETONANIMATION);
		pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_SkeletonAnimation::CSubPath::BoneAnimation(i).ConvertToInteger());
		
		if(pSkeletonAnimation->m_BoneAnimations[i].m_BonePath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_PARENT)
			pItem->SetText(pSkeleton->m_ParentPath, CModAPI_Asset_Skeleton::BONE_NAME, BonePath.ConvertToInteger());
		else
			pItem->SetText(pSkeletonAnimation->m_SkeletonPath, CModAPI_Asset_Skeleton::BONE_NAME, BonePath.ConvertToInteger());
		
		m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->Add(pItem);
	}
	
	m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Layer Animations", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	for(int i=0; i<pSkeletonAnimation->m_LayerAnimations.size(); i++)
	{
		CModAPI_Asset_Skeleton::CSubPath LayerPath = CModAPI_Asset_Skeleton::CSubPath::Layer(pSkeletonAnimation->m_LayerAnimations[i].m_LayerPath.GetId());
		
		CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_LAYERANIMATION);
		pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_SkeletonAnimation::CSubPath::LayerAnimation(i).ConvertToInteger());
		
		if(pSkeletonAnimation->m_LayerAnimations[i].m_LayerPath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_PARENT)
			pItem->SetText(pSkeleton->m_ParentPath, CModAPI_Asset_Skeleton::LAYER_NAME, LayerPath.ConvertToInteger());
		else
			pItem->SetText(pSkeletonAnimation->m_SkeletonPath, CModAPI_Asset_Skeleton::LAYER_NAME, LayerPath.ConvertToInteger());
		
		m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->Add(pItem);
	}
	if(KeepStatus)
	{
		m_pLists[LIST_SKELETONANIMATION_ANIMATIONS]->SetScrollPos(Scroll);
	}

	m_pTabs[TAB_SKELETONANIMATION_ANIMATIONS]->AddSeparator();	
		
	CModAPI_Asset_Skeleton::CSubPath EditedSubPath(m_pAssetsEditor->m_EditedAssetSubPath);
	if(!EditedSubPath.IsNull() && EditedSubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEANIMATION)
	{
		AddCycleField(m_pTabs[TAB_SKELETONANIMATION_ANIMATIONS], CModAPI_Asset_SkeletonAnimation::BONEANIMATION_CYCLETYPE, m_pAssetsEditor->m_EditedAssetSubPath, "Cycle type:");
	}
}

void CModAPI_AssetsEditorGui_Editor::RefreshTab_SkeletonAnimation_KeyFrames(bool KeepStatus)
{
	float Scroll = 0;
	if(KeepStatus && m_pLists[LIST_SKELETONANIMATION_KEYFRAMES])
	{
		Scroll = m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->GetScrollPos();
	}
	m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES]->Clear();
	
	CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_SkeletonAnimation>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pSkeletonAnimation)
		return;
			
	CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(pSkeletonAnimation->m_SkeletonPath);
	if(!pSkeleton)
		return;
		
	m_pLists[LIST_SKELETONANIMATION_KEYFRAMES] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
	m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->SetHeight(m_Rect.h/2);
	m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES]->Add(m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]);
	
	m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Bone Key Frames", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	for(int i=0; i<pSkeletonAnimation->m_BoneAnimations.size(); i++)
	{
		CModAPI_Asset_Skeleton::CSubPath BonePath = CModAPI_Asset_Skeleton::CSubPath::Bone(pSkeletonAnimation->m_BoneAnimations[i].m_BonePath.GetId());
		const char* pBoneName;
		
		if(pSkeletonAnimation->m_BoneAnimations[i].m_BonePath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_PARENT)
			pBoneName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(pSkeleton->m_ParentPath, CModAPI_Asset_Skeleton::BONE_NAME, BonePath.ConvertToInteger(), 0);
		else
			pBoneName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(pSkeletonAnimation->m_SkeletonPath, CModAPI_Asset_Skeleton::BONE_NAME, BonePath.ConvertToInteger(), 0);
				
		for(int j=0; j<pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames.size(); j++)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "%s, frame #%d", pBoneName, pSkeletonAnimation->m_BoneAnimations[i].m_KeyFrames[j].m_Time);
			
			CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_KEYFRAME_BONE);
			pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_SkeletonAnimation::CSubPath::BoneKeyFrame(i, j).ConvertToInteger());
			pItem->SetText(aBuf);
			m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->Add(pItem);
		}
	}

	m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Layer Key Frames", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
	for(int i=0; i<pSkeletonAnimation->m_LayerAnimations.size(); i++)
	{
		CModAPI_Asset_Skeleton::CSubPath LayerPath = CModAPI_Asset_Skeleton::CSubPath::Layer(pSkeletonAnimation->m_LayerAnimations[i].m_LayerPath.GetId());
		const char* pLayerName;
		
		if(pSkeletonAnimation->m_LayerAnimations[i].m_LayerPath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_PARENT)
			pLayerName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(pSkeleton->m_ParentPath, CModAPI_Asset_Skeleton::LAYER_NAME, LayerPath.ConvertToInteger(), 0);
		else
			pLayerName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(pSkeletonAnimation->m_SkeletonPath, CModAPI_Asset_Skeleton::LAYER_NAME, LayerPath.ConvertToInteger(), 0);
		
		for(int j=0; j<pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames.size(); j++)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "%s, frame #%d", pLayerName, pSkeletonAnimation->m_LayerAnimations[i].m_KeyFrames[j].m_Time);
			
			CModAPI_AssetsEditorGui_Editor::CSubItemListItem* pItem = new CModAPI_AssetsEditorGui_Editor::CSubItemListItem(m_pAssetsEditor, MODAPI_ASSETSEDITOR_ICON_KEYFRAME_LAYER);
			pItem->SetTarget(m_pAssetsEditor->m_EditedAssetPath, CModAPI_Asset_SkeletonAnimation::CSubPath::LayerKeyFrame(i, j).ConvertToInteger());
			pItem->SetText(aBuf);
			m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->Add(pItem);
		}
	}
	if(KeepStatus)
	{
		m_pLists[LIST_SKELETONANIMATION_KEYFRAMES]->SetScrollPos(Scroll);
	}
	
	m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES]->AddSeparator();	
		
	CModAPI_Asset_Skeleton::CSubPath EditedSubPath(m_pAssetsEditor->m_EditedAssetSubPath);
	if(!EditedSubPath.IsNull())
	{
		if(EditedSubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEKEYFRAME)
		{
			AddFloat2Field(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], CModAPI_Asset_SkeletonAnimation::BONEKEYFRAME_TRANSLATION_X, CModAPI_Asset_SkeletonAnimation::BONEKEYFRAME_TRANSLATION_Y, m_pAssetsEditor->m_EditedAssetSubPath, "Translation:");
			AddFloat2Field(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], CModAPI_Asset_SkeletonAnimation::BONEKEYFRAME_SCALE_X, CModAPI_Asset_SkeletonAnimation::BONEKEYFRAME_SCALE_Y, m_pAssetsEditor->m_EditedAssetSubPath, "Scale:");
			AddAngleField(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], CModAPI_Asset_SkeletonAnimation::BONEKEYFRAME_ANGLE, m_pAssetsEditor->m_EditedAssetSubPath, "Angle:");
			AddBoneAlignField(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], CModAPI_Asset_SkeletonAnimation::BONEKEYFRAME_ALIGNMENT, m_pAssetsEditor->m_EditedAssetSubPath, "Alignment:");
		}
		else if(EditedSubPath.GetType() == CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERKEYFRAME)
		{
			AddColorField(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], CModAPI_Asset_SkeletonAnimation::LAYERKEYFRAME_COLOR, m_pAssetsEditor->m_EditedAssetSubPath, "Color:");
			AddLayerStateField(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], CModAPI_Asset_SkeletonAnimation::LAYERKEYFRAME_STATE, m_pAssetsEditor->m_EditedAssetSubPath, "State:");
		}
	}
}

void CModAPI_AssetsEditorGui_Editor::Refresh(int Tab)
{
	bool KeepStatus = false;
	if(m_pAssetsEditor->m_EditedAssetPath.GetType() != m_LastEditedAssetType)
	{
		Clear();
		
		for(int i=0; i<NUM_TABS; i++)
		{
			m_pTabs[i] = 0;
		}
		for(int i=0; i<NUM_LISTS; i++)
		{
			m_pLists[i] = 0;
		}
		
		//Add main tab
		m_pTabs[TAB_ASSET] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
		AddTab(m_pTabs[TAB_ASSET], MODAPI_ASSETSEDITOR_ICON_ASSET, "Asset properties");
		
		//Add asset-specific tabs
		switch(m_pAssetsEditor->m_EditedAssetPath.GetType())
		{
			case CModAPI_AssetPath::TYPE_SKELETON:
				m_pTabs[TAB_SKELETON_BONES] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
				AddTab(m_pTabs[TAB_SKELETON_BONES], MODAPI_ASSETSEDITOR_ICON_BONE, "Bones: create, edit and remove bones from the skeleton");
				m_pTabs[TAB_SKELETON_LAYERS] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
				AddTab(m_pTabs[TAB_SKELETON_LAYERS], MODAPI_ASSETSEDITOR_ICON_LAYERS, "Layers: create, edit and remove layers from the skeleton");
				break;
			case CModAPI_AssetPath::TYPE_SKELETONSKIN:
				m_pTabs[TAB_SKELETONSKIN_SPRITES] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
				AddTab(m_pTabs[TAB_SKELETONSKIN_SPRITES], MODAPI_ASSETSEDITOR_ICON_SPRITE, "Sprites: create, edit and remove sprites from the skin");
				break;
			case CModAPI_AssetPath::TYPE_SKELETONANIMATION:
				m_pTabs[TAB_SKELETONANIMATION_ANIMATIONS] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
				AddTab(m_pTabs[TAB_SKELETONANIMATION_ANIMATIONS], MODAPI_ASSETSEDITOR_ICON_SKELETONANIMATION, "Animations: set propeties for bone and layer animations");
				m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES] = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE);
				AddTab(m_pTabs[TAB_SKELETONANIMATION_KEYFRAMES], MODAPI_ASSETSEDITOR_ICON_FRAMES, "Key Frames: edit properties of key frames");
				break;
		}
		
		m_LastEditedAssetType = m_pAssetsEditor->m_EditedAssetPath.GetType();
	}
	else
	{
		KeepStatus = true;
	}
	AddAssetTabCommons(m_pTabs[TAB_ASSET]);
	
	if(Tab >= 0)
	{
		m_SelectedTab = Tab;
	}
	
	//Search Tag: TAG_NEW_ASSET
	switch(m_pAssetsEditor->m_EditedAssetPath.GetType())
	{
		case CModAPI_AssetPath::TYPE_IMAGE:
			RefreshTab_Image_Asset(KeepStatus);
			break;
		case CModAPI_AssetPath::TYPE_SPRITE:
			RefreshTab_Sprite_Asset(KeepStatus);
			break;
		case CModAPI_AssetPath::TYPE_SKELETON:
			RefreshTab_Skeleton_Asset(KeepStatus);
			RefreshTab_Skeleton_Bones(KeepStatus);
			RefreshTab_Skeleton_Layers(KeepStatus);
			break;
		case CModAPI_AssetPath::TYPE_SKELETONSKIN:
			RefreshTab_SkeletonSkin_Asset(KeepStatus);
			RefreshTab_SkeletonSkin_Sprites(KeepStatus);
			break;
		case CModAPI_AssetPath::TYPE_SKELETONANIMATION:
			RefreshTab_SkeletonAnimation_Asset(KeepStatus);
			RefreshTab_SkeletonAnimation_Animations(KeepStatus);
			RefreshTab_SkeletonAnimation_KeyFrames(KeepStatus);
			break;
	}
	
	
	Update();
}

void CModAPI_AssetsEditorGui_Editor::OnEditedAssetChange()
{
	Refresh();
}
