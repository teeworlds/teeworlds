#ifndef MODAPI_ASSETSEDITOR_EDITOR_H
#define MODAPI_ASSETSEDITOR_EDITOR_H

#include <base/vmath.h>
#include <engine/kernel.h>
#include <game/client/render.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/layout.h>

#include "assetseditor.h"

class CModAPI_AssetsEditorGui_Editor : public CModAPI_ClientGui_VListLayout
{
protected:
	class CAssetNameEdit : public CModAPI_ClientGui_AbstractTextEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		
		virtual char* GetTextPtr()
		{
			return GetAssetMemberPointer<char>(m_pAssetsEditor, m_AssetType, m_AssetPath, MODAPI_ASSETSEDITOR_MEMBER_NAME, -1);
		}
		
	public:
		CAssetNameEdit(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath) :
			CModAPI_ClientGui_AbstractTextEdit(pAssetsEditor->m_pGuiConfig, sizeof(CModAPI_Asset::m_aName), MODAPI_CLIENTGUI_TEXTSTYLE_HEADER),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath)
		{
			
		}
	};

	class CAngleAssetMemberLabel : public CModAPI_ClientGui_Label
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		
		virtual float GetValue()
		{
			float* ptr = GetAssetMemberPointer<float>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
				return *ptr;
			else
				return 0;
		}
		
	public:
		CAngleAssetMemberLabel(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_Label(pAssetsEditor->m_pGuiConfig, "0.0 deg"),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
		
		virtual void Render()
		{
			float value = 180.0f * GetValue() / pi;
			if(value < 0.0f) value += 360.0f;
			if(value > 360.0f) value -= 360.0f;
			
			str_format(m_aText, sizeof(m_aText), "%.02f deg", value);
			CModAPI_ClientGui_Label::Render();
		}
	};

	class CIntegerAssetMemberEdit : public CModAPI_ClientGui_AbstractIntegerEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		
		virtual void SetValue(int v)
		{
			int* ptr = GetAssetMemberPointer<int>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
				*ptr = v;
		}
		
		virtual int GetValue()
		{
			int* ptr = GetAssetMemberPointer<int>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
				return *ptr;
			else
				return 0;
		}
		
	public:
		CIntegerAssetMemberEdit(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_AbstractIntegerEdit(pAssetsEditor->m_pGuiConfig),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
	};

	class CAssetEdit : public CModAPI_ClientGui_ExternalTextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		int m_ParentAssetType;
		CModAPI_AssetPath m_ParentAssetPath;
		int m_ParentAssetMember;
		int m_ParentAssetSubId;
		int m_FieldAssetType;
		CModAPI_AssetPath m_FieldAssetPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_AssetEditList(m_pAssetsEditor, m_ParentAssetType, m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, m_FieldAssetType));
		}
		
	public:
		CAssetEdit(
			CModAPI_AssetsEditor* pAssetsEditor,
			int ParentAssetType,
			CModAPI_AssetPath ParentAssetPath,
			int ParentAssetMember,
			int ParentAssetSubId,
			int FieldAssetType
		) :
			CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, 0),
			m_ParentAssetType(ParentAssetType),
			m_ParentAssetPath(ParentAssetPath),
			m_ParentAssetMember(ParentAssetMember),
			m_ParentAssetSubId(ParentAssetSubId),
			m_FieldAssetType(FieldAssetType),
			m_pAssetsEditor(pAssetsEditor)
		{
			CModAPI_AssetPath* ptr = GetAssetMemberPointer<CModAPI_AssetPath>(m_pAssetsEditor, m_ParentAssetType, m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId);
			if(ptr)
			{
				m_pText = GetAssetMemberPointer<char>(m_pAssetsEditor, m_FieldAssetType, *ptr, MODAPI_ASSETSEDITOR_MEMBER_NAME, -1);
			}
		}
		
		virtual void Update()
		{
			CModAPI_AssetPath* ptr = GetAssetMemberPointer<CModAPI_AssetPath>(m_pAssetsEditor, m_ParentAssetType, m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId);
			if(ptr)
			{
				m_pText = GetAssetMemberPointer<char>(m_pAssetsEditor, m_FieldAssetType, *ptr, MODAPI_ASSETSEDITOR_MEMBER_NAME, -1);
			}
			
			CModAPI_ClientGui_ExternalTextButton::Update();
		}
	};

	class CTeeAlignEdit : public CModAPI_ClientGui_TextButton
	{
	protected:
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			int* ptr = GetAssetMemberPointer<int>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
				*ptr = (*ptr + 1)%MODAPI_NUM_TEEALIGN;
			
			UpdateButtonLabel(*ptr);
		}
		
		void UpdateButtonLabel(int Value)
		{
			switch(Value)
			{
				case MODAPI_TEEALIGN_AIM:
					SetText("Aim");
					break;
				case MODAPI_TEEALIGN_HORIZONTAL:
					SetText("Horizontal");
					break;
				case MODAPI_TEEALIGN_DIRECTION:
					SetText("Motion");
					break;
				case MODAPI_TEEALIGN_NONE:
				default:
					SetText("None");
			}
		}
		
	public:
		CTeeAlignEdit(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "", -1),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
		
		virtual void Update()
		{
			int* ptr = GetAssetMemberPointer<int>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
			{
				UpdateButtonLabel(*ptr);
			}
			
			CModAPI_ClientGui_TextButton::Update();
		}
	};

	class CCycleTypeEdit : public CModAPI_ClientGui_TextButton
	{
	protected:
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			int* ptr = GetAssetMemberPointer<int>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
				*ptr = (*ptr + 1)%MODAPI_NUM_ANIMCYCLETYPE;
			
			UpdateButtonLabel(*ptr);
		}
		
		void UpdateButtonLabel(int Value)
		{
			switch(Value)
			{
				case MODAPI_ANIMCYCLETYPE_CLAMP:
					SetText("Clamp");
					break;
				case MODAPI_ANIMCYCLETYPE_LOOP:
					SetText("Loop");
					break;
			}
		}
		
	public:
		CCycleTypeEdit(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "", -1),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
		
		virtual void Update()
		{
			int* ptr = GetAssetMemberPointer<int>(m_pAssetsEditor, m_AssetType, m_AssetPath, m_Member, m_SubId);
			if(ptr)
			{
				UpdateButtonLabel(*ptr);
			}
			
			CModAPI_ClientGui_TextButton::Update();
		}
	};

	class CDeleteSubItem : public CModAPI_ClientGui_IconButton
	{
	protected:
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			switch(m_AssetType)
			{
				case MODAPI_ASSETTYPE_ANIMATION:
				{
					CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_AssetPath);
					if(!pAnimation)
						return;
					pAnimation->DeleteKeyFrame(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
				case MODAPI_ASSETTYPE_ATTACH:
				{
					CModAPI_Asset_Attach* pAttach = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_AssetPath);
					if(!pAttach)
						return;
					pAttach->DeleteBackElement(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
			}
		}

	public:
		CDeleteSubItem(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int SubId) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_DELETE),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_SubId(SubId),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};

	class CMoveDownSubItem : public CModAPI_ClientGui_IconButton
	{
	protected:
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			switch(m_AssetType)
			{
				case MODAPI_ASSETTYPE_ANIMATION:
				{
					CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_AssetPath);
					if(!pAnimation)
						return;
					pAnimation->MoveDownKeyFrame(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
				case MODAPI_ASSETTYPE_ATTACH:
				{
					CModAPI_Asset_Attach* pAttach = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_AssetPath);
					if(!pAttach)
						return;
					pAttach->MoveDownBackElement(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
			}
		}

	public:
		CMoveDownSubItem(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int SubId) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_UP),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_SubId(SubId),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};

	class CMoveUpSubItem : public CModAPI_ClientGui_IconButton
	{
	protected:
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			switch(m_AssetType)
			{
				case MODAPI_ASSETTYPE_ANIMATION:
				{
					CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_AssetPath);
					if(!pAnimation)
						return;
					pAnimation->MoveUpKeyFrame(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
				case MODAPI_ASSETTYPE_ATTACH:
				{
					CModAPI_Asset_Attach* pAttach = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_AssetPath);
					if(!pAttach)
						return;
					pAttach->MoveUpBackElement(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
			}
		}

	public:
		CMoveUpSubItem(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int SubId) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_DOWN),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath),
			m_SubId(SubId),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};

	class CAddAttachElement : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		int m_AssetType;
		CModAPI_AssetPath m_AssetPath;
		
	protected:
		virtual void MouseClickAction()
		{
			if(m_AssetType != MODAPI_ASSETTYPE_ATTACH)
				return;
			
			CModAPI_Asset_Attach* pAttach = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_AssetPath);
			if(!pAttach)
				return;
			
			pAttach->AddBackElement(CModAPI_Asset_Attach::CElement());
			
			m_pAssetsEditor->RefreshAssetEditor();
		}

	public:
		CAddAttachElement(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "Add element", MODAPI_ASSETSEDITOR_ICON_INCREASE),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetType(AssetType),
			m_AssetPath(AssetPath)
		{
			
		}
	};
	
	class CSelectFrameButton : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_FrameId;
		
	protected:
		virtual void MouseClickAction()
		{
			CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_AssetPath);
			if(!pAnimation)
				return;
			
			if(m_FrameId < 0 || m_FrameId >= pAnimation->m_lKeyFrames.size())
				return;
			
			float Time = pAnimation->m_lKeyFrames[m_FrameId].m_Time;
			
			m_pAssetsEditor->EditAssetFrame(m_FrameId);
		}
		
	public:
		CSelectFrameButton(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int FrameId) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "Select", MODAPI_ASSETSEDITOR_ICON_NEXT_FRAME),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_FrameId(FrameId)
		{
			
		}
	};
	
	class CDuplicateFrameButton : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_FrameId;
		
	protected:
		virtual void MouseClickAction()
		{
			CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_AssetPath);
			if(!pAnimation)
				return;
			
			pAnimation->DuplicateKeyFrame(m_FrameId);
			m_pAssetsEditor->RefreshAssetEditor();
		}
		
	public:
		CDuplicateFrameButton(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int FrameId) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "Duplicate", MODAPI_ASSETSEDITOR_ICON_DUPLICATE),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_FrameId(FrameId)
		{
			
		}
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;

protected:
	void AddTitle();
	void AddLabeledIntegerField(const char* pLabelText, int Member, int SubId = -1);
	void AddLabeledAssetField(const char* pLabelText, int Member, int AssetType, int SubId = -1);
	void AddLabeledTeeAlignField(const char* pLabelText, int Member, int SubId = -1);
	void AddAssetField(int Member, int AssetType, int SubId = -1);
	void AddSubTitle(const char* pText);
	void AddImageEditor();
	void AddSpriteEditor();
	void AddAnimationEditor();
	void AddTeeAnimationEditor();
	void AddAttachEditor();
	
public:
	CModAPI_AssetsEditorGui_Editor(CModAPI_AssetsEditor* pAssetsEditor);
	void OnEditedAssetChange();
	void Refresh();
};

#endif
