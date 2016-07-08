#ifndef MODAPI_ASSETSEDITOR_EDITOR_H
#define MODAPI_ASSETSEDITOR_EDITOR_H

#include <base/vmath.h>
#include <engine/kernel.h>
#include <game/client/render.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/layout.h>
#include <modapi/client/gui/integer-edit.h>
#include <modapi/client/gui/float-edit.h>

#include "popup.h"
#include "assetseditor.h"

class CModAPI_AssetsEditorGui_Editor : public CModAPI_ClientGui_Tabs
{	
public:
	class CAssetNameEdit : public CModAPI_ClientGui_AbstractTextEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		
		virtual char* GetTextPtr()
		{
			return m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, CModAPI_Asset::NAME, -1, 0);
		}
		
	public:
		CAssetNameEdit(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath) :
			CModAPI_ClientGui_AbstractTextEdit(pAssetsEditor->m_pGuiConfig, sizeof(CModAPI_Asset::m_aName), MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath)
		{
			
		}
	};
			
	class CTextAssetMemberEdit : public CModAPI_ClientGui_AbstractTextEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		
		virtual char* GetTextPtr()
		{
			return m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_AssetPath, m_Member, m_SubId, 0);
		}
		
	public:
		CTextAssetMemberEdit(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Member, int SubId, int TextSize) :
			CModAPI_ClientGui_AbstractTextEdit(pAssetsEditor->m_pGuiConfig, TextSize, MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
	};
			
	class CFloatAssetMemberEdit : public CModAPI_ClientGui_AbstractFloatEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		
		virtual void SetValue(float v)
		{
			m_pAssetsEditor->AssetManager()->SetAssetValue<float>(m_AssetPath, m_Member, m_SubId, v);
		}
		
		virtual float GetValue()
		{
			return m_pAssetsEditor->AssetManager()->GetAssetValue<float>(m_AssetPath, m_Member, m_SubId, -2);
		}
		
	public:
		CFloatAssetMemberEdit(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_AbstractFloatEdit(pAssetsEditor->m_pGuiConfig),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
	};
			
	class CAnglerEdit : public CModAPI_ClientGui_AbstractFloatEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		
		virtual void SetValue(float v)
		{
			m_pAssetsEditor->AssetManager()->SetAssetValue<float>(m_AssetPath, m_Member, m_SubId, pi*v/180.0f);
		}
		
		virtual float GetValue()
		{
			return 180.0f*m_pAssetsEditor->AssetManager()->GetAssetValue<float>(m_AssetPath, m_Member, m_SubId, -2)/pi;
		}
		
	public:
		CAnglerEdit(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_AbstractFloatEdit(pAssetsEditor->m_pGuiConfig),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
	};

	class CIntegerAssetMemberEdit : public CModAPI_ClientGui_AbstractIntegerEdit
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		
		virtual void SetValue(int v)
		{
			m_pAssetsEditor->AssetManager()->SetAssetValue<int>(m_AssetPath, m_Member, m_SubId, v);
		}
		
		virtual int GetValue()
		{
			return m_pAssetsEditor->AssetManager()->GetAssetValue<int>(m_AssetPath, m_Member, m_SubId, -2);
		}
		
	public:
		CIntegerAssetMemberEdit(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Member, int SubId = -1) :
			CModAPI_ClientGui_AbstractIntegerEdit(pAssetsEditor->m_pGuiConfig),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId)
		{
			
		}
	};

	class CAssetEdit : public CModAPI_ClientGui_ExternalTextButton
	{
	public:
		static const char* m_aNoneText;
		
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_ParentAssetPath;
		int m_ParentAssetMember;
		int m_ParentAssetSubId;
		int m_FieldAssetType;
		CModAPI_AssetPath m_FieldAssetPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_AssetEdit(
				m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_LEFT,
				m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, m_FieldAssetType
			));
		}
		
	public:
		CAssetEdit(
			CModAPI_AssetsEditor* pAssetsEditor,
			CModAPI_AssetPath ParentAssetPath,
			int ParentAssetMember,
			int ParentAssetSubId,
			int FieldAssetType
		) :
			CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, 0),
			m_ParentAssetPath(ParentAssetPath),
			m_ParentAssetMember(ParentAssetMember),
			m_ParentAssetSubId(ParentAssetSubId),
			m_FieldAssetType(FieldAssetType),
			m_pAssetsEditor(pAssetsEditor)
		{
			m_Centered = false;
			
			Update();
		}
		
		virtual void Update()
		{
			CModAPI_AssetPath Path = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_AssetPath>(m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, CModAPI_AssetPath::Null());
			if(Path.IsNull())
			{
				m_pText = m_aNoneText;
			}
			else
			{
				m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(Path, CModAPI_Asset::NAME, -1, 0);
			}
			
			int IconId = -1;
			switch(Path.GetType())
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
			}
			SetIcon(IconId);
			
			CModAPI_ClientGui_ExternalTextButton::Update();
		}
	};

	class CBoneEdit : public CModAPI_ClientGui_ExternalTextButton
	{
	public:
		static const char* m_aNoneText;
		
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_ParentAssetPath;
		int m_ParentAssetMember;
		int m_ParentAssetSubId;
		CModAPI_AssetPath m_SkeletonPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_BoneEdit(
				m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_LEFT,
				m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, m_SkeletonPath
			));
		}
		
	public:
		CBoneEdit(
			CModAPI_AssetsEditor* pAssetsEditor,
			CModAPI_AssetPath ParentAssetPath,
			int ParentAssetMember,
			int ParentAssetSubId,
			CModAPI_AssetPath SkeletonPath
		) :
			CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, 0),
			m_ParentAssetPath(ParentAssetPath),
			m_ParentAssetMember(ParentAssetMember),
			m_ParentAssetSubId(ParentAssetSubId),
			m_SkeletonPath(SkeletonPath),
			m_pAssetsEditor(pAssetsEditor)
		{
			m_Centered = false;
			
			Update();
		}
		
		virtual void Update()
		{
			CModAPI_Asset_Skeleton::CBonePath BonePath = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_Asset_Skeleton::CBonePath>(m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, CModAPI_Asset_Skeleton::CBonePath::Null());
			if(BonePath.IsNull())
			{
				m_pText = m_aNoneText;
			}
			else if(BonePath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_PARENT)
			{
				CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(m_SkeletonPath);
				if(pSkeleton)
				{
					m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
						pSkeleton->m_ParentPath,
						CModAPI_Asset_Skeleton::BONE_NAME,
						CModAPI_Asset_Skeleton::CSubPath::Bone(BonePath.GetId()).ConvertToInteger(),
						0);
					SetIcon(MODAPI_ASSETSEDITOR_ICON_BONE);
				}
			}
			else
			{
				m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
					m_SkeletonPath,
					CModAPI_Asset_Skeleton::BONE_NAME,
					CModAPI_Asset_Skeleton::CSubPath::Bone(BonePath.GetId()).ConvertToInteger(),
					0);
				SetIcon(MODAPI_ASSETSEDITOR_ICON_BONE);
			}
			
			CModAPI_ClientGui_ExternalTextButton::Update();
		}
	};

	class CLayerEdit : public CModAPI_ClientGui_ExternalTextButton
	{
	public:
		static const char* m_aNoneText;
		
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_ParentAssetPath;
		int m_ParentAssetMember;
		int m_ParentAssetSubId;
		CModAPI_AssetPath m_SkeletonPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_LayerEdit(
				m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_LEFT,
				m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, m_SkeletonPath
			));
		}
		
	public:
		CLayerEdit(
			CModAPI_AssetsEditor* pAssetsEditor,
			CModAPI_AssetPath ParentAssetPath,
			int ParentAssetMember,
			int ParentAssetSubId,
			CModAPI_AssetPath SkeletonPath
		) :
			CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, 0),
			m_ParentAssetPath(ParentAssetPath),
			m_ParentAssetMember(ParentAssetMember),
			m_ParentAssetSubId(ParentAssetSubId),
			m_SkeletonPath(SkeletonPath),
			m_pAssetsEditor(pAssetsEditor)
		{
			m_Centered = false;
			
			Update();
		}
		
		virtual void Update()
		{
			CModAPI_Asset_Skeleton::CBonePath LayerPath = m_pAssetsEditor->AssetManager()->GetAssetValue<CModAPI_Asset_Skeleton::CBonePath>(m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, CModAPI_Asset_Skeleton::CBonePath::Null());
			if(LayerPath.IsNull())
			{
				m_pText = m_aNoneText;
			}
			else if(LayerPath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_PARENT)
			{
				CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(m_SkeletonPath);
				if(pSkeleton)
				{
					m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
						pSkeleton->m_ParentPath,
						CModAPI_Asset_Skeleton::LAYER_NAME,
						CModAPI_Asset_Skeleton::CSubPath::Layer(LayerPath.GetId()).ConvertToInteger(),
						0);
					SetIcon(MODAPI_ASSETSEDITOR_ICON_LAYERS);
				}
			}
			else
			{
				m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
					m_SkeletonPath,
					CModAPI_Asset_Skeleton::LAYER_NAME,
					CModAPI_Asset_Skeleton::CSubPath::Layer(LayerPath.GetId()).ConvertToInteger(),
					0);
				SetIcon(MODAPI_ASSETSEDITOR_ICON_LAYERS);
			}
			
			CModAPI_ClientGui_ExternalTextButton::Update();
		}
	};

	class CCharacterPartEdit : public CModAPI_ClientGui_ExternalTextButton
	{
	public:
		static const char* m_aNoneText;
		
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_ParentAssetPath;
		int m_ParentAssetMember;
		int m_ParentAssetSubId;
		CModAPI_AssetPath m_CharacterPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_CharacterPartEdit(
				m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_LEFT,
				m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, m_CharacterPath
			));
		}
		
	public:
		CCharacterPartEdit(
			CModAPI_AssetsEditor* pAssetsEditor,
			CModAPI_AssetPath ParentAssetPath,
			int ParentAssetMember,
			int ParentAssetSubId,
			CModAPI_AssetPath CharacterPath
		) :
			CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, 0),
			m_ParentAssetPath(ParentAssetPath),
			m_ParentAssetMember(ParentAssetMember),
			m_ParentAssetSubId(ParentAssetSubId),
			m_CharacterPath(CharacterPath),
			m_pAssetsEditor(pAssetsEditor)
		{
			m_Centered = false;
			
			Update();
		}
		
		virtual void Update()
		{
			CModAPI_Asset_Character::CSubPath SubPath = m_pAssetsEditor->AssetManager()->GetAssetValue<int>(
				m_ParentAssetPath,
				m_ParentAssetMember,
				m_ParentAssetSubId,
				CModAPI_Asset_Character::CSubPath::Null().ConvertToInteger()
			);
			
			if(SubPath.IsNull())
			{
				m_pText = m_aNoneText;
			}
			else
			{
				m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
					m_CharacterPath,
					CModAPI_Asset_Character::PART_NAME,
					CModAPI_Asset_Character::CSubPath::Part(SubPath.GetId()).ConvertToInteger(),
					0);
				SetIcon(MODAPI_ASSETSEDITOR_ICON_CHARACTERPART);
			}
			
			CModAPI_ClientGui_ExternalTextButton::Update();
		}
	};

	class CColorEdit : public CModAPI_ClientGui_Widget
	{		
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		int m_AssetMember;
		int m_AssetSubId;
		
		bool m_Clicked;
		bool m_UnderMouse;
		
	public:
		CColorEdit(
			CModAPI_AssetsEditor* pAssetsEditor,
			CModAPI_AssetPath AssetPath,
			int AssetMember,
			int AssetSubId
		) :
			CModAPI_ClientGui_Widget(pAssetsEditor->m_pGuiConfig),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_AssetMember(AssetMember),
			m_AssetSubId(AssetSubId),
			m_Clicked(false),
			m_UnderMouse(false)
		{
			m_Rect.w = m_pConfig->m_ButtonHeight;
			m_Rect.h = m_pConfig->m_ButtonHeight;
		}
		
		virtual void Render()
		{
			{
				CUIRect rect;
				rect.x = m_Rect.x;
				rect.y = m_Rect.y;
				rect.w = m_Rect.w;
				rect.h = m_Rect.h;
				
				RenderTools()->DrawRoundRect(&rect, m_pConfig->m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL], s_ButtonCornerRadius);
			}
			{
				vec4 Color = m_pAssetsEditor->AssetManager()->GetAssetValue<vec4>(
					m_AssetPath,
					m_AssetMember,
					m_AssetSubId,
					vec4(1.0f, 1.0f, 1.0f, 1.0f));
					
				CUIRect rect;
				rect.x = m_Rect.x+1;
				rect.y = m_Rect.y+1;
				rect.w = (m_Rect.w-2)/2;
				rect.h = m_Rect.h-2;
				
				vec4 SquareColor = Color;
				SquareColor.a = 1.0f;
				RenderTools()->DrawRoundRect(&rect, SquareColor, s_ButtonCornerRadius-1);
			}
			{
				vec4 Color = m_pAssetsEditor->AssetManager()->GetAssetValue<vec4>(
					m_AssetPath,
					m_AssetMember,
					m_AssetSubId,
					vec4(1.0f, 1.0f, 1.0f, 1.0f));
				
				int alphaSquareX = m_Rect.x+2+(m_Rect.w-2)/2;
				int alphaSquareW = (m_Rect.x + m_Rect.w - 1) - alphaSquareX;
				float SquareHeight = (m_Rect.h-2)/3.0f;
				int nbColumns = alphaSquareW / SquareHeight;
				float SquareWidth = alphaSquareW/nbColumns;
				
				Graphics()->TextureClear();
				Graphics()->QuadsBegin();
				for(int j=0; j<3; j++)
				{
					for(int i=0; i<nbColumns; i++)
					{
						if(i%2==j%2)
							Graphics()->SetColor(0.4f, 0.4f, 0.4f, 1.0f);
						else
							Graphics()->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
						IGraphics::CQuadItem QuadItem(alphaSquareX + i*SquareWidth, m_Rect.y+1 + j*SquareHeight, SquareWidth, SquareHeight);
						Graphics()->QuadsDrawTL(&QuadItem, 1);
					}
				}
				Graphics()->QuadsEnd();
				
				CUIRect rect;
				rect.x = alphaSquareX;
				rect.y = m_Rect.y+1;
				rect.w = alphaSquareW;
				rect.h = m_Rect.h-2;
				
				vec4 SquareColor = vec4(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);
				RenderTools()->DrawRoundRect(&rect, SquareColor, s_ButtonCornerRadius-1);
			}
		}

		void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
		{
			if(m_Rect.IsInside(X, Y))
			{
				m_UnderMouse = true;
			}
			else
			{
				m_UnderMouse = false;
			}
		}

		void OnButtonClick(int X, int Y, int Button)
		{
			if(Button != KEY_MOUSE_1)
				return;
			
			if(m_Rect.IsInside(X, Y))
			{
				m_Clicked = true;
			}
		}

		void OnButtonRelease(int Button)
		{
			if(Button != KEY_MOUSE_1)
				return;
			
			if(m_UnderMouse && m_Clicked)
			{
				m_Clicked = false;
				m_pAssetsEditor->DisplayPopup(new CModAPI_AssetsEditorGui_Popup_ColorEdit(
					m_pAssetsEditor, m_Rect, CModAPI_ClientGui_Popup::ALIGNMENT_LEFT,
					m_AssetPath, m_AssetMember, m_AssetSubId
				));
			}
		}
	};

	class CEnumEdit : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetPath m_AssetPath;
		int m_Member;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		int m_NbElements;
		const char** m_aTexts;
		
	protected:
		virtual void MouseClickAction()
		{
			int OldValue = m_pAssetsEditor->AssetManager()->GetAssetValue<int>(m_AssetPath, m_Member, m_SubId, 0);
			int NewValue = (OldValue + 1)%m_NbElements;
			m_pAssetsEditor->AssetManager()->SetAssetValue<int>(m_AssetPath, m_Member, m_SubId, NewValue);
			UpdateButtonLabel(NewValue);
		}
		
		void UpdateButtonLabel(int Value)
		{
			SetText(m_aTexts[Value%m_NbElements]);
		}
		
	public:
		CEnumEdit(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int Member, int SubId, const char** aTexts, int NbElements) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "", -1),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_Member(Member),
			m_SubId(SubId),
			m_aTexts(aTexts),
			m_NbElements(NbElements)
		{
			
		}
		
		virtual void Update()
		{
			int Value = m_pAssetsEditor->AssetManager()->GetAssetValue<int>(m_AssetPath, m_Member, m_SubId, 0);
			UpdateButtonLabel(Value);
			
			CModAPI_ClientGui_TextButton::Update();
		}
	};

	class CDeleteAsset : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_AssetPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->DeleteAsset(m_AssetPath);
		}

	public:
		CDeleteAsset(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, "Delete", MODAPI_ASSETSEDITOR_ICON_DELETE),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath)
		{
			
		}
	};

	class CDeleteSubItem : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetPath m_AssetPath;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			switch(m_AssetPath.GetType())
			{
				case CModAPI_AssetPath::TYPE_ANIMATION:
				{
					CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Animation>(m_AssetPath);
					if(!pAnimation)
						return;
					pAnimation->DeleteKeyFrame(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
			}
		}

	public:
		CDeleteSubItem(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int SubId) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_DELETE),
			m_AssetPath(AssetPath),
			m_SubId(SubId),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};

	class CMoveDownSubItem : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetPath m_AssetPath;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			switch(m_AssetPath.GetType())
			{
				case CModAPI_AssetPath::TYPE_ANIMATION:
				{
					CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Animation>(m_AssetPath);
					if(!pAnimation)
						return;
					pAnimation->MoveDownKeyFrame(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
			}
		}

	public:
		CMoveDownSubItem(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int SubId) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_UP),
			m_AssetPath(AssetPath),
			m_SubId(SubId),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};

	class CMoveUpSubItem : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetPath m_AssetPath;
		int m_SubId;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
	protected:
		virtual void MouseClickAction()
		{
			switch(m_AssetPath.GetType())
			{
				case CModAPI_AssetPath::TYPE_ANIMATION:
				{
					CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Animation>(m_AssetPath);
					if(!pAnimation)
						return;
					pAnimation->MoveUpKeyFrame(m_SubId);
					m_pAssetsEditor->RefreshAssetEditor();
					break;
				}
			}
		}

	public:
		CMoveUpSubItem(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int SubId) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_DOWN),
			m_AssetPath(AssetPath),
			m_SubId(SubId),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};

	class CSubItemListItem : public CModAPI_ClientGui_ExternalTextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		CModAPI_AssetPath m_AssetPath;
		int m_AssetSubPath;
		char m_aText[128];
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->EditAssetSubItem(m_AssetPath, m_AssetSubPath);
		}
	
	public:
		CSubItemListItem(CModAPI_AssetsEditor* pAssetsEditor, int Icon) :
			CModAPI_ClientGui_ExternalTextButton(pAssetsEditor->m_pGuiConfig, 0, Icon),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(CModAPI_AssetPath::Null()),
			m_AssetSubPath(-1)
		{
			m_Centered = false;
		}
		
		void SetTarget(CModAPI_AssetPath AssetPath, int AssetSubPath)
		{
			m_AssetPath = AssetPath;
			m_AssetSubPath = AssetSubPath;
		}
		
		void SetText(CModAPI_AssetPath AssetPath, int AssetSubType, int AssetSubPath)
		{
			m_pText = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(AssetPath, AssetSubType, AssetSubPath, 0);
		}
		
		void SetText(const char* pText)
		{
			str_copy(m_aText, pText, sizeof(m_aText));
			m_pText = m_aText;
		}
	};

	class CAddSubItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		CModAPI_AssetPath m_AssetPath;
		int m_SubItemType;
		int m_TabId;
		
	protected:
		virtual void MouseClickAction()
		{
			int SubPath = m_pAssetsEditor->AssetManager()->AddSubItem(m_AssetPath, m_SubItemType);
			m_pAssetsEditor->EditAssetSubItem(m_AssetPath, SubPath, m_TabId);
		}
	
	public:
		CAddSubItem(CModAPI_AssetsEditor* pAssetsEditor, CModAPI_AssetPath AssetPath, int SubType, const char* pText, int IconId, int Tab=-1) :
			CModAPI_ClientGui_TextButton(pAssetsEditor->m_pGuiConfig, pText, IconId),
			m_pAssetsEditor(pAssetsEditor),
			m_AssetPath(AssetPath),
			m_SubItemType(SubType),
			m_TabId(Tab)
		{
			m_Centered = false;
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
			CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Animation>(m_AssetPath);
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
	
public:
	//Search Tag: TAG_NEW_ASSET
	enum
	{
		TAB_ASSET=0,
		
		TAB_SKELETON_BONES=1,
		TAB_SKELETON_LAYERS,
		
		TAB_SKELETONSKIN_SPRITES=1,
		
		TAB_SKELETONANIMATION_ANIMATIONS=1,
		TAB_SKELETONANIMATION_KEYFRAMES,
		
		TAB_CHARACTER_PARTS=1,
		
		NUM_TABS=3
	};
	enum
	{
		LIST_SKELETON_BONES=0,
		LIST_SKELETON_LAYERS,
		
		LIST_SKELETONSKIN_SPRITES=0,
		
		LIST_SKELETONANIMATION_ANIMATIONS=0,
		LIST_SKELETONANIMATION_KEYFRAMES,
		
		LIST_CHARACTER_PARTS=0,
		
		NUM_LISTS=2
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	
	int m_LastEditedAssetType;
	CModAPI_ClientGui_VListLayout* m_pTabs[NUM_TABS];
	CModAPI_ClientGui_VListLayout* m_pLists[NUM_LISTS];

protected:
	void AddAssetTabCommons(CModAPI_ClientGui_VListLayout* pList);
	void AddSubTitle(CModAPI_ClientGui_VListLayout* pList, const char* pText);
	
	void AddField(CModAPI_ClientGui_VListLayout* pList, CModAPI_ClientGui_Widget* pWidget, const char* pLabelText);
	
	void AddTextField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, int TextSize, const char* pLabelText = 0);
	void AddFloatField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddFloat2Field(CModAPI_ClientGui_VListLayout* pList, int Member1, int Member2, int SubId, const char* pLabelText = 0);
	void AddAngleField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddIntegerField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddAssetField(CModAPI_ClientGui_VListLayout* pList, int Member, int AssetType, int SubId, const char* pLabelText = 0);
	void AddBoneField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, CModAPI_AssetPath SkeletonPath, const char* pLabelText = 0);
	void AddLayerField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, CModAPI_AssetPath SkeletonPath, const char* pLabelText = 0);
	void AddCharacterPartField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, CModAPI_AssetPath CharacterPath, const char* pLabelText = 0);
	void AddCycleField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddSpriteAlignField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddLayerStateField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddBoneAlignField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	void AddColorField(CModAPI_ClientGui_VListLayout* pList, int Member, int SubId, const char* pLabelText = 0);
	
	//Search Tag: TAG_NEW_ASSET
	void RefreshTab_Image_Asset(bool KeepStatus);
	void RefreshTab_Sprite_Asset(bool KeepStatus);
	void RefreshTab_Skeleton_Asset(bool KeepStatus);
	void RefreshTab_Skeleton_Bones(bool KeepStatus);
	void RefreshTab_Skeleton_Layers(bool KeepStatus);
	void RefreshTab_SkeletonSkin_Asset(bool KeepStatus);
	void RefreshTab_SkeletonSkin_Sprites(bool KeepStatus);
	void RefreshTab_SkeletonAnimation_Asset(bool KeepStatus);
	void RefreshTab_SkeletonAnimation_Animations(bool KeepStatus);
	void RefreshTab_SkeletonAnimation_KeyFrames(bool KeepStatus);
	void RefreshTab_Character_Asset(bool KeepStatus);
	void RefreshTab_Character_Parts(bool KeepStatus);
	void RefreshTab_CharacterPart_Asset(bool KeepStatus);
	void RefreshTab_Attach_Asset(bool KeepStatus);
	
public:
	CModAPI_AssetsEditorGui_Editor(CModAPI_AssetsEditor* pAssetsEditor);
	void OnEditedAssetChange();
	void Refresh(int Tab=-1);
};

#endif
