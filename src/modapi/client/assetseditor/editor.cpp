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

CModAPI_AssetsEditorGui_Editor::CModAPI_AssetsEditorGui_Editor(CModAPI_AssetsEditor* pAssetsEditor) :
	CModAPI_ClientGui_VListLayout(pAssetsEditor->m_pGuiConfig),
	m_pAssetsEditor(pAssetsEditor)
{
	AddTitle();
}

void CModAPI_AssetsEditorGui_Editor::AddTitle()
{
	CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST);
	pLayout->SetHeight(m_pConfig->m_ButtonHeight);
	Add(pLayout);
	
	pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CAssetNameEdit(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath));
}

void CModAPI_AssetsEditorGui_Editor::AddLabeledIntegerField(const char* pLabelText, int Member, int SubId)
{
	CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
	pLayout->SetHeight(m_pConfig->m_ButtonHeight);
	Add(pLayout);
	
	CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, pLabelText);
	pLayout->Add(pLabel);
	
	CModAPI_AssetsEditorGui_Editor::CIntegerAssetMemberEdit* pEdit = new CModAPI_AssetsEditorGui_Editor::CIntegerAssetMemberEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, Member, SubId);
	pLayout->Add(pEdit);
}

void CModAPI_AssetsEditorGui_Editor::AddAssetField(int Member, int AssetType, int SubId)
{
	CModAPI_AssetsEditorGui_Editor::CAssetEdit* pEdit = new CModAPI_AssetsEditorGui_Editor::CAssetEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, AssetType);
	Add(pEdit);
}

void CModAPI_AssetsEditorGui_Editor::AddLabeledAssetField(const char* pLabelText, int Member, int AssetType, int SubId)
{
	CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
	pLayout->SetHeight(m_pConfig->m_ButtonHeight);
	Add(pLayout);
	
	CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, pLabelText);
	pLayout->Add(pLabel);
	
	CModAPI_AssetsEditorGui_Editor::CAssetEdit* pEdit = new CModAPI_AssetsEditorGui_Editor::CAssetEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, Member, SubId, AssetType);
	pLayout->Add(pEdit);
}

void CModAPI_AssetsEditorGui_Editor::AddLabeledTeeAlignField(const char* pLabelText, int Member, int SubId)
{
	CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
	pLayout->SetHeight(m_pConfig->m_ButtonHeight);
	Add(pLayout);
	
	CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, pLabelText);
	pLayout->Add(pLabel);
	
	CModAPI_AssetsEditorGui_Editor::CTeeAlignEdit* pEdit = new CModAPI_AssetsEditorGui_Editor::CTeeAlignEdit(
		m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, Member, SubId);
	pLayout->Add(pEdit);
}

void CModAPI_AssetsEditorGui_Editor::AddSubTitle(const char* pText)
{
	Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, pText, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
}

void CModAPI_AssetsEditorGui_Editor::AddImageEditor()
{
	CModAPI_Asset_Image* pImage = m_pAssetsEditor->ModAPIGraphics()->GetAsset<CModAPI_Asset_Image>(m_pAssetsEditor->m_EditedAssetPath);
	if(!pImage)
		return;
	
	//Size
	{
		CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
		pLayout->SetHeight(m_pConfig->m_ButtonHeight);
		Add(pLayout);
		
		CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Size:");
		pLayout->Add(pLabel);
		
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d x %d", pImage->m_Width, pImage->m_Height);
		
		CModAPI_ClientGui_Label* pValue = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, aBuf);
		pLayout->Add(pValue);
	}
	
	AddLabeledIntegerField("Grid width:", MODAPI_ASSETSEDITOR_MEMBER_IMAGE_GRIDWIDTH);
	AddLabeledIntegerField("Grid height:", MODAPI_ASSETSEDITOR_MEMBER_IMAGE_GRIDHEIGHT);
}

void CModAPI_AssetsEditorGui_Editor::AddSpriteEditor()
{
	CModAPI_Asset_Sprite* pSprite = m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
	if(!pSprite)
		return;
		
	AddLabeledAssetField("Image:", MODAPI_ASSETSEDITOR_MEMBER_SPRITE_IMAGEPATH, MODAPI_ASSETTYPE_IMAGE);
	AddLabeledIntegerField("X:", MODAPI_ASSETSEDITOR_MEMBER_SPRITE_X);
	AddLabeledIntegerField("Y:", MODAPI_ASSETSEDITOR_MEMBER_SPRITE_Y);
	AddLabeledIntegerField("Width:", MODAPI_ASSETSEDITOR_MEMBER_SPRITE_WIDTH);
	AddLabeledIntegerField("Height:", MODAPI_ASSETSEDITOR_MEMBER_SPRITE_HEIGHT);
}

void CModAPI_AssetsEditorGui_Editor::AddAnimationEditor()
{
	CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
	if(!pAnimation)
		return;
	
	{	
		CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
		pLayout->SetHeight(m_pConfig->m_ButtonHeight);
		Add(pLayout);
		
		CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Cycle type:");
		pLayout->Add(pLabel);
		
		CModAPI_AssetsEditorGui_Editor::CCycleTypeEdit* pEdit = new CModAPI_AssetsEditorGui_Editor::CCycleTypeEdit(
			m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, MODAPI_ASSETSEDITOR_MEMBER_ANIMATION_CYCLETYPE, -1);
		pLayout->Add(pEdit);
	}
	
	for(int i=0; i<pAnimation->m_lKeyFrames.size(); i++)
	{
		AddSeparator();
		
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Frame #%d", i+1);
		
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			Add(pLayout);
			
			pLayout->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, aBuf, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
			pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CMoveUpSubItem(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, i));
			pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CMoveDownSubItem(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, i));
			pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CDeleteSubItem(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, i));
		}
		
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			Add(pLayout);
			
			pLayout->Add(new CSelectFrameButton(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, i));
			pLayout->Add(new CDuplicateFrameButton(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetPath, i));
		}
		
		AddLabeledIntegerField("Index", MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_INDEX, i);
	
		//Angle
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Angle");
			pLayout->Add(pLabel);
			
			CModAPI_AssetsEditorGui_Editor::CAngleAssetMemberLabel* pValue = new CModAPI_AssetsEditorGui_Editor::CAngleAssetMemberLabel(
				m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_ANGLE, i);
			pLayout->Add(pValue);
		}
	}
}

void CModAPI_AssetsEditorGui_Editor::AddTeeAnimationEditor()
{
	CModAPI_Asset_TeeAnimation* pTeeAnimation = m_pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
	if(!pTeeAnimation)
		return;

	AddSubTitle("Body");
	AddAssetField(MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BODYANIMPATH, MODAPI_ASSETTYPE_ANIMATION);
	
	AddSubTitle("Back foot");
	AddAssetField(MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKFOOTANIMPATH, MODAPI_ASSETTYPE_ANIMATION);
	
	AddSubTitle("Front foot");
	AddAssetField(MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTFOOTANIMPATH, MODAPI_ASSETTYPE_ANIMATION);
	
	AddSubTitle("Back hand");
	AddAssetField(MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDANIMPATH, MODAPI_ASSETTYPE_ANIMATION);
	AddLabeledTeeAlignField("Alignment", MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDALIGNMENT);
	AddLabeledIntegerField("Offset X", MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDOFFSETX);
	AddLabeledIntegerField("Offset Y", MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDOFFSETY);
	
	AddSubTitle("Front hand");
	AddAssetField(MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDANIMPATH, MODAPI_ASSETTYPE_ANIMATION);
	AddLabeledTeeAlignField("Alignment", MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDALIGNMENT);
	AddLabeledIntegerField("Offset X", MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDOFFSETX);
	AddLabeledIntegerField("Offset Y", MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDOFFSETY);
}

void CModAPI_AssetsEditorGui_Editor::AddAttachEditor()
{
	CModAPI_Asset_Attach* pAttach = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
	if(!pAttach)
		return;
	
	AddLabeledAssetField("Animation", MODAPI_ASSETSEDITOR_MEMBER_ATTACH_TEEANIMATIONPATH, MODAPI_ASSETTYPE_TEEANIMATION);
	AddLabeledAssetField("Cursor", MODAPI_ASSETSEDITOR_MEMBER_ATTACH_CURSORPATH, MODAPI_ASSETTYPE_SPRITE);
	
	for(int i=0; i<pAttach->m_BackElements.size(); i++)
	{
		AddSeparator();
		
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Back element #%d", i+1);
		
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			Add(pLayout);
			
			pLayout->Add(new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, aBuf, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2));
			pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CMoveUpSubItem(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, i));
			pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CMoveDownSubItem(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, i));
			pLayout->Add(new CModAPI_AssetsEditorGui_Editor::CDeleteSubItem(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath, i));
		}
		
		
		AddLabeledAssetField("Sprite", MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_SPRITEPATH, MODAPI_ASSETTYPE_SPRITE, i);
		AddLabeledIntegerField("Size", MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_SIZE, i);
		AddLabeledAssetField("Animation", MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_ANIMATIONPATH, MODAPI_ASSETTYPE_ANIMATION, i);
		AddLabeledTeeAlignField("Alignment", MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_ALIGNMENT, i);
		AddLabeledIntegerField("Offset X", MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_OFFSETX, i);
		AddLabeledIntegerField("Offset Y", MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_OFFSETY, i);
	}
	
	AddSeparator();
	Add(new CModAPI_AssetsEditorGui_Editor::CAddAttachElement(m_pAssetsEditor, m_pAssetsEditor->m_EditedAssetType, m_pAssetsEditor->m_EditedAssetPath));
}

void CModAPI_AssetsEditorGui_Editor::Refresh()
{
	Clear();
	
	AddTitle();
	
	switch(m_pAssetsEditor->m_EditedAssetType)
	{
		case MODAPI_ASSETTYPE_IMAGE:
			AddImageEditor();
			break;
		case MODAPI_ASSETTYPE_SPRITE:
			AddSpriteEditor();
			break;
		case MODAPI_ASSETTYPE_ANIMATION:
			AddAnimationEditor();
			break;
		case MODAPI_ASSETTYPE_TEEANIMATION:
			AddTeeAnimationEditor();
			break;
		case MODAPI_ASSETTYPE_ATTACH:
			AddAttachEditor();
			break;
	}
	
	Update();
}

void CModAPI_AssetsEditorGui_Editor::OnEditedAssetChange()
{
	Refresh();
}
