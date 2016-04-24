#ifndef MODAPI_ASSETSEDITOR_POPUP_H
#define MODAPI_ASSETSEDITOR_POPUP_H

#include <modapi/client/gui/popup.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/tabs.h>
#include <modapi/client/gui/label.h>
#include <modapi/client/gui/float-edit.h>
#include <base/color.h>

#include "assetseditor.h"

/* POPUP ADD IMAGE ****************************************************/

class CModAPI_AssetsEditorGui_Popup_AddImage : public CModAPI_ClientGui_Popup
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_ClientGui_VListLayout* m_Layout;
	
protected:
	class CItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_AddImage* m_pPopup;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		char m_aFilename[128];
		int m_StorageType;
		int m_IsDirectory;
		
	protected:
		virtual void MouseClickAction()
		{
			if(!m_IsDirectory)
			{
				CModAPI_AssetPath Path = m_pAssetsEditor->AssetManager()->AddImage(m_StorageType, m_aFilename, CModAPI_AssetPath::SRC_EXTERNAL);
				if(!Path.IsNull())
				{
					m_pAssetsEditor->NewAsset(Path);
				}
				m_pPopup->Close();
			}
		}
		
	public:
		CItem(CModAPI_AssetsEditorGui_Popup_AddImage* pPopup, const char* pFilename, int StorageType, int IsDir) :
			CModAPI_ClientGui_TextButton(pPopup->m_pAssetsEditor->m_pGuiConfig, pFilename, MODAPI_ASSETSEDITOR_ICON_IMAGE),
			m_pPopup(pPopup),
			m_pAssetsEditor(pPopup->m_pAssetsEditor)
		{
			m_Centered = false;
			
			str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
			m_StorageType = StorageType;
			m_IsDirectory = IsDir;
		}
	};
	
public:
	CModAPI_AssetsEditorGui_Popup_AddImage(CModAPI_AssetsEditor* pAssetsEditor, const CModAPI_ClientGui_Rect& CreatorRect, int Alignment) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 250, 500, Alignment),
		m_pAssetsEditor(pAssetsEditor)
	{		
		m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		Add(m_Layout);
		
		Update();
	}
	
	static int FileListFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
	{
		CModAPI_AssetsEditorGui_Popup_AddImage* pAddImageView = static_cast<CModAPI_AssetsEditorGui_Popup_AddImage*>(pUser);
		
		int Length = str_length(pName);
		if(pName[0] == '.' && (pName[1] == 0))
			return 0;
		
		if(Length < 4 || str_comp(pName+Length-4, ".png"))
			return 0;

		CModAPI_AssetsEditorGui_Popup_AddImage::CItem* pItem = new CModAPI_AssetsEditorGui_Popup_AddImage::CItem(pAddImageView, pName, StorageType, IsDir);
		pAddImageView->m_Layout->Add(pItem);

		return 0;
	}
	
	virtual void Update()
	{
		m_pAssetsEditor->Storage()->ListDirectory(IStorage::TYPE_ALL, ".", FileListFetchCallback, this);
		
		CModAPI_ClientGui_Popup::Update();
	}
};

/* POPUP ASSET EDIT ***************************************************/

class CModAPI_AssetsEditorGui_Popup_AssetEdit : public CModAPI_ClientGui_Popup
{
public:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	
protected:
	class CItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_AssetEdit* m_pPopup;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		CModAPI_AssetPath m_ParentAssetPath;
		int m_ParentAssetMember;
		int m_ParentAssetSubId;
		CModAPI_AssetPath m_FieldAssetPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->AssetManager()->SetAssetValue<CModAPI_AssetPath>(m_ParentAssetPath, m_ParentAssetMember, m_ParentAssetSubId, m_FieldAssetPath);
			m_pAssetsEditor->RefreshAssetEditor();
		}
		
	public:
		CItem(CModAPI_AssetsEditorGui_Popup_AssetEdit* pPopup, CModAPI_AssetPath ParentAssetPath, int ParentAssetMember, int ParentAssetSubId, int FieldAssetType, CModAPI_AssetPath FieldAssetPath) :
			CModAPI_ClientGui_TextButton(pPopup->m_pAssetsEditor->m_pGuiConfig, ""),
			m_pPopup(pPopup),
			m_pAssetsEditor(pPopup->m_pAssetsEditor),
			m_ParentAssetPath(ParentAssetPath),
			m_ParentAssetMember(ParentAssetMember),
			m_ParentAssetSubId(ParentAssetSubId),
			m_FieldAssetPath(FieldAssetPath)
		{
			m_Centered = false;
			
			if(m_FieldAssetPath.IsNull())
			{
				SetText("None");
			}
			else
			{
				int IconId = -1;
				switch(FieldAssetPath.GetType())
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
				
				char* pName = m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(m_FieldAssetPath, CModAPI_Asset::NAME, -1, 0);
				SetText(pName);
				SetIcon(IconId);
			}
		}
	};

protected:
	CModAPI_ClientGui_VListLayout* m_Layout;
	
protected:
	CModAPI_AssetPath m_ParentAssetPath;
	int m_ParentAssetMember;
	int m_ParentAssetSubId;
	int m_FieldAssetType;
	
protected:
	void AddListElement(CModAPI_AssetPath Path)
	{	
		CItem* pItem = new CItem(
			this,
			m_ParentAssetPath,
			m_ParentAssetMember,
			m_ParentAssetSubId,
			m_FieldAssetType,
			Path
		);
		m_Layout->Add(pItem);
	}
	
public:
	CModAPI_AssetsEditorGui_Popup_AssetEdit(
		CModAPI_AssetsEditor* pAssetsEditor,
		const CModAPI_ClientGui_Rect& CreatorRect,
		int Alignment,
		CModAPI_AssetPath ParentAssetPath,
		int ParentAssetMember,
		int ParentAssetSubId,
		int FieldAssetType
	) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 250, 500, Alignment),
		m_pAssetsEditor(pAssetsEditor),
		m_ParentAssetPath(ParentAssetPath),
		m_ParentAssetMember(ParentAssetMember),
		m_ParentAssetSubId(ParentAssetSubId),
		m_FieldAssetType(FieldAssetType)
	{
		m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		Add(m_Layout);
		
		Update();
	}
	
	virtual void Update()
	{
		#define POPUP_ASSET_EDIT_LIST(TypeName, TypeHeader) case TypeName::TypeId:\
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Internal "TypeHeader, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));\
			for(int i=0; i<m_pAssetsEditor->AssetManager()->GetNumAssets<TypeName>(CModAPI_AssetPath::SRC_INTERNAL); i++)\
				AddListElement(CModAPI_AssetPath::Internal(TypeName::TypeId, i));\
			m_Layout->AddSeparator();\
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "External "TypeHeader, MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));\
			for(int i=0; i<m_pAssetsEditor->AssetManager()->GetNumAssets<TypeName>(CModAPI_AssetPath::SRC_EXTERNAL); i++)\
				AddListElement(CModAPI_AssetPath::External(TypeName::TypeId, i));\
			break;
		
		AddListElement(CModAPI_AssetPath::Null());
		switch(m_FieldAssetType)
		{
			//Search Tag: TAG_NEW_ASSET
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_Image, "Images")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_Sprite, "Sprites")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_Animation, "Animations")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_TeeAnimation, "Tee Animations")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_Attach, "Attaches")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_LineStyle, "Line Styles")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_Skeleton, "Skeletons")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_SkeletonSkin, "Skeleton Skins")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_SkeletonAnimation, "Skeleton Animations")
			POPUP_ASSET_EDIT_LIST(CModAPI_Asset_List, "Lists")
		}
		
		CModAPI_ClientGui_Popup::Update();
	}
};

/* POPUP BONE EDIT ****************************************************/

class CModAPI_AssetsEditorGui_Popup_BoneEdit : public CModAPI_ClientGui_Popup
{
public:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	int m_AssetMember;
	int m_AssetSubPath;
	CModAPI_AssetPath m_SkeletonPath;
	
protected:
	class CItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_BoneEdit* m_pPopup;
		CModAPI_Asset_Skeleton::CBonePath m_BonePath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pPopup->m_pAssetsEditor->AssetManager()->SetAssetValue<CModAPI_Asset_Skeleton::CBonePath>(
				m_pPopup->m_AssetPath,
				m_pPopup->m_AssetMember,
				m_pPopup->m_AssetSubPath,
				m_BonePath);
			m_pPopup->m_pAssetsEditor->RefreshAssetEditor();
		}
		
	public:
		CItem(CModAPI_AssetsEditorGui_Popup_BoneEdit* pPopup, CModAPI_Asset_Skeleton::CBonePath BonePath) :
			CModAPI_ClientGui_TextButton(pPopup->m_pAssetsEditor->m_pGuiConfig, ""),
			m_pPopup(pPopup),
			m_BonePath(BonePath)
		{
			m_Centered = false;
			
			if(m_BonePath.IsNull())
			{
				SetText("None");
			}
			else
			{
				if(m_BonePath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_LOCAL)
				{
					char* pName = pPopup->m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
						pPopup->m_SkeletonPath,
						CModAPI_Asset_Skeleton::BONE_NAME,
						CModAPI_Asset_Skeleton::CSubPath::Bone(m_BonePath.GetId()).ConvertToInteger(),
						0);
					SetText(pName);
					SetIcon(MODAPI_ASSETSEDITOR_ICON_BONE);
				}
				else
				{
					CModAPI_Asset_Skeleton* pSkeleton = pPopup->m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(pPopup->m_SkeletonPath);
					if(pSkeleton && !pSkeleton->m_ParentPath.IsNull())
					{
						char* pName = pPopup->m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
							pSkeleton->m_ParentPath,
							CModAPI_Asset_Skeleton::BONE_NAME,
							CModAPI_Asset_Skeleton::CSubPath::Bone(m_BonePath.GetId()).ConvertToInteger(),
							0);
						SetText(pName);
						SetIcon(MODAPI_ASSETSEDITOR_ICON_BONE);
					}					
				}
			}
		}
	};

protected:
	CModAPI_ClientGui_VListLayout* m_Layout;
	
public:
	CModAPI_AssetsEditorGui_Popup_BoneEdit(
		CModAPI_AssetsEditor* pAssetsEditor,
		const CModAPI_ClientGui_Rect& CreatorRect,
		int Alignment,
		CModAPI_AssetPath AssetPath,
		int AssetMember,
		int AssetSubPath,
		CModAPI_AssetPath SkeletonPath
	) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 250, 500, Alignment),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_AssetMember(AssetMember),
		m_AssetSubPath(AssetSubPath),
		m_SkeletonPath(SkeletonPath)
	{		
		m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		Add(m_Layout);
		
		Update();
	}
	
	virtual void Update()
	{
		m_Layout->Clear();
		
		CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(m_SkeletonPath);
		if(pSkeleton)
		{
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Local bones", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));\
			for(int i=0; i<pSkeleton->m_Bones.size(); i++)
			{
				CItem* pItem = new CItem(this, CModAPI_Asset_Skeleton::CBonePath::Local(i));
				m_Layout->Add(pItem);
			}
			
			if(!pSkeleton->m_ParentPath.IsNull())
			{
				CModAPI_Asset_Skeleton* pParentSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(pSkeleton->m_ParentPath);
				if(pParentSkeleton)
				{
					m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Parent bones", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));\
					for(int i=0; i<pParentSkeleton->m_Bones.size(); i++)
					{
						CItem* pItem = new CItem(this, CModAPI_Asset_Skeleton::CBonePath::Parent(i));
						m_Layout->Add(pItem);
					}
				}
			}
		}
		
		CModAPI_ClientGui_Popup::Update();
	}
};

/* POPUP LAYER EDIT ****************************************************/

class CModAPI_AssetsEditorGui_Popup_LayerEdit : public CModAPI_ClientGui_Popup
{
public:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	int m_AssetMember;
	int m_AssetSubPath;
	CModAPI_AssetPath m_SkeletonPath;
	
protected:
	class CItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_LayerEdit* m_pPopup;
		CModAPI_Asset_Skeleton::CBonePath m_LayerPath;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pPopup->m_pAssetsEditor->AssetManager()->SetAssetValue<CModAPI_Asset_Skeleton::CBonePath>(
				m_pPopup->m_AssetPath,
				m_pPopup->m_AssetMember,
				m_pPopup->m_AssetSubPath,
				m_LayerPath);
			m_pPopup->m_pAssetsEditor->RefreshAssetEditor();
		}
		
	public:
		CItem(CModAPI_AssetsEditorGui_Popup_LayerEdit* pPopup, CModAPI_Asset_Skeleton::CBonePath LayerPath) :
			CModAPI_ClientGui_TextButton(pPopup->m_pAssetsEditor->m_pGuiConfig, ""),
			m_pPopup(pPopup),
			m_LayerPath(LayerPath)
		{
			m_Centered = false;
			
			if(m_LayerPath.IsNull())
			{
				SetText("None");
			}
			else
			{
				if(m_LayerPath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_LOCAL)
				{
					char* pName = pPopup->m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
						pPopup->m_SkeletonPath,
						CModAPI_Asset_Skeleton::LAYER_NAME,
						CModAPI_Asset_Skeleton::CSubPath::Layer(m_LayerPath.GetId()).ConvertToInteger(),
						0);
					SetText(pName);
					SetIcon(MODAPI_ASSETSEDITOR_ICON_BONE);
				}
				else
				{
					CModAPI_Asset_Skeleton* pSkeleton = pPopup->m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(pPopup->m_SkeletonPath);
					if(pSkeleton && !pSkeleton->m_ParentPath.IsNull())
					{
						char* pName = pPopup->m_pAssetsEditor->AssetManager()->GetAssetValue<char*>(
							pSkeleton->m_ParentPath,
							CModAPI_Asset_Skeleton::LAYER_NAME,
							CModAPI_Asset_Skeleton::CSubPath::Layer(m_LayerPath.GetId()).ConvertToInteger(),
							0);
						SetText(pName);
						SetIcon(MODAPI_ASSETSEDITOR_ICON_BONE);
					}					
				}
			}
		}
	};

protected:
	CModAPI_ClientGui_VListLayout* m_Layout;
	
public:
	CModAPI_AssetsEditorGui_Popup_LayerEdit(
		CModAPI_AssetsEditor* pAssetsEditor,
		const CModAPI_ClientGui_Rect& CreatorRect,
		int Alignment,
		CModAPI_AssetPath AssetPath,
		int AssetMember,
		int AssetSubPath,
		CModAPI_AssetPath SkeletonPath
	) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 250, 500, Alignment),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_AssetMember(AssetMember),
		m_AssetSubPath(AssetSubPath),
		m_SkeletonPath(SkeletonPath)
	{		
		m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		Add(m_Layout);
		
		Update();
	}
	
	virtual void Update()
	{
		m_Layout->Clear();
		
		CModAPI_Asset_Skeleton* pSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(m_SkeletonPath);
		if(pSkeleton)
		{
			m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Local layers", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));\
			for(int i=0; i<pSkeleton->m_Layers.size(); i++)
			{
				CItem* pItem = new CItem(this, CModAPI_Asset_Skeleton::CBonePath::Local(i));
				m_Layout->Add(pItem);
			}
			
			if(!pSkeleton->m_ParentPath.IsNull())
			{
				CModAPI_Asset_Skeleton* pParentSkeleton = m_pAssetsEditor->AssetManager()->GetAsset<CModAPI_Asset_Skeleton>(pSkeleton->m_ParentPath);
				if(pParentSkeleton)
				{
					m_Layout->Add(new CModAPI_ClientGui_Label(m_pConfig, "Parent layers", MODAPI_CLIENTGUI_TEXTSTYLE_HEADER));\
					for(int i=0; i<pParentSkeleton->m_Layers.size(); i++)
					{
						CItem* pItem = new CItem(this, CModAPI_Asset_Skeleton::CBonePath::Parent(i));
						m_Layout->Add(pItem);
					}
				}
			}
		}
		
		CModAPI_ClientGui_Popup::Update();
	}
};

/* POPUP COLOR EDIT ***************************************************/

class CModAPI_AssetsEditorGui_Popup_ColorEdit : public CModAPI_ClientGui_Popup
{
public:
	class CColorSliderEdit : public CModAPI_ClientGui_Widget
	{	
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;		
		bool m_Clicked;
		
	public:
		CColorSliderEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_Widget(pPopup->m_pAssetsEditor->m_pGuiConfig),
			m_pPopup(pPopup),
			m_Clicked(false)
		{
			m_Rect.w = m_pConfig->m_ButtonHeight;
			m_Rect.h = m_pConfig->m_ButtonHeight;
		}
	public:
	
		virtual vec4 GetSliderValue(float Value) = 0;

		void RenderLines(float Cursor)
		{
			Graphics()->LinesBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			IGraphics::CLineItem Lines[4] = {
					IGraphics::CLineItem(m_Rect.x, m_Rect.y, m_Rect.x+m_Rect.w, m_Rect.y),
					IGraphics::CLineItem(m_Rect.x+m_Rect.w, m_Rect.y, m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h),
					IGraphics::CLineItem(m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h, m_Rect.x, m_Rect.y+m_Rect.h),
					IGraphics::CLineItem(m_Rect.x, m_Rect.y+m_Rect.h, m_Rect.x, m_Rect.y)
			};
			Graphics()->LinesDraw(Lines, 4);
			
			IGraphics::CLineItem CursorLine(m_Rect.x+m_Rect.w*Cursor, m_Rect.y, m_Rect.x+m_Rect.w*Cursor, m_Rect.y+m_Rect.h);
			Graphics()->LinesDraw(&CursorLine, 1);
			Graphics()->LinesEnd();
		}
		
		void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
		{
			if(m_Rect.IsInside(X, Y) && m_Clicked)
			{
				float Value = static_cast<float>(X - m_Rect.x)/static_cast<float>(m_Rect.w);
				m_pPopup->SetColor(GetSliderValue(Value));
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
			
			m_Clicked = false;
		}
	};
	
	template<int C>
	class CRGBSliderEdit : public CColorSliderEdit
	{
	public:
		CRGBSliderEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CColorSliderEdit(pPopup)
		{
			
		}
		
		virtual void Render()
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			
			vec4 Color = m_pPopup->GetColor();
			vec4 minColor = Color;
			*(static_cast<float*>(&minColor.r)+C) = 0.0f;
			vec4 maxColor = Color;
			*(static_cast<float*>(&maxColor.r)+C) = 1.0f;
						
			IGraphics::CColorVertex ColorArray[4];
			ColorArray[0] = IGraphics::CColorVertex(0, minColor.r, minColor.g, minColor.b, 1.0f);
			ColorArray[1] = IGraphics::CColorVertex(1, maxColor.r, maxColor.g, maxColor.b, 1.0f);
			ColorArray[2] = IGraphics::CColorVertex(2, maxColor.r, maxColor.g, maxColor.b, 1.0f);
			ColorArray[3] = IGraphics::CColorVertex(3, minColor.r, minColor.g, minColor.b, 1.0f);
			Graphics()->SetColorVertex(ColorArray, 4);
		
			IGraphics::CQuadItem QuadItem(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			
			float Cursor = *(static_cast<float*>(&Color.r)+C);
			RenderLines(Cursor);
		}

		virtual vec4 GetSliderValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			*(static_cast<float*>(&Color.r)+C) = Value;
			return Color;
		}
	};
	
	class CAlphaSliderEdit : public CRGBSliderEdit<3>
	{		
	public:
		CAlphaSliderEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CRGBSliderEdit<3>(pPopup)
		{
			
		}
		
	public:
		virtual void Render()
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			
			float SquareSize = m_Rect.h/3.0f;
			int nbColumns = m_Rect.w / SquareSize;
			
			for(int j=0; j<3; j++)
			{
				for(int i=0; i<nbColumns; i++)
				{
					if(i%2==j%2)
						Graphics()->SetColor(0.4f, 0.4f, 0.4f, 1.0f);
					else
						Graphics()->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
					IGraphics::CQuadItem QuadItem(m_Rect.x + i*SquareSize, m_Rect.y + j*SquareSize, SquareSize, SquareSize);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
				}
			}
			
			vec4 Color = m_pPopup->GetColor();
			
			IGraphics::CColorVertex ColorArray[4];
			ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
			ColorArray[1] = IGraphics::CColorVertex(1, Color.r, Color.g, Color.b, 1.0f);
			ColorArray[2] = IGraphics::CColorVertex(2, Color.r, Color.g, Color.b, 1.0f);
			ColorArray[3] = IGraphics::CColorVertex(3, 0.0f, 0.0f, 0.0f, 0.0f);
			Graphics()->SetColorVertex(ColorArray, 4);
		
			IGraphics::CQuadItem QuadItem(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			
			Graphics()->QuadsEnd();
			
			float Cursor = Color.a;
			RenderLines(Cursor);
		}
	};
	
	template<int C>
	class CHSVSliderEdit : public CColorSliderEdit
	{
	public:
		CHSVSliderEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CColorSliderEdit(pPopup)
		{
			
		}
		
		virtual void Render()
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();

			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			
			int nbCells = 6;
			float Offset = m_Rect.w/static_cast<float>(nbCells);
			for(int i=0; i<nbCells; i++)
			{
				float Value0 = i*1.0f/nbCells;
				float Value1 = (i+1)*1.0f/nbCells;
				vec3 ColorHSV0 = ColorHSV;
				*(static_cast<float*>(&ColorHSV0.r)+C) = Value0;
				vec3 ColorHSV1 = ColorHSV;		
				*(static_cast<float*>(&ColorHSV1.r)+C) = Value1;
				vec3 ColorRGB0 = HsvToRgb(vec3(ColorHSV0.r, ColorHSV0.g, ColorHSV0.b));
				vec3 ColorRGB1 = HsvToRgb(vec3(ColorHSV1.r, ColorHSV1.g, ColorHSV1.b));

				IGraphics::CColorVertex ColorArray[4];
				ColorArray[0] = IGraphics::CColorVertex(0, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				ColorArray[1] = IGraphics::CColorVertex(1, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				ColorArray[2] = IGraphics::CColorVertex(2, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				ColorArray[3] = IGraphics::CColorVertex(3, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				Graphics()->SetColorVertex(ColorArray, 4);
				IGraphics::CQuadItem QuadItem(m_Rect.x+i*Offset, m_Rect.y, Offset, m_Rect.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
		
			Graphics()->QuadsEnd();

			float Cursor = *(static_cast<float*>(&ColorHSV.r)+C);
			RenderLines(Cursor);
		}

		virtual vec4 GetSliderValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			vec3 HSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			*(static_cast<float*>(&HSV.r)+C) = Value;
			vec3 RGB = HsvToRgb(vec3(HSV.r, HSV.g, HSV.b));
			return vec4(RGB.r, RGB.g, RGB.b, Color.a);
		}
	};
	
	class CHueVSliderEdit : public CModAPI_ClientGui_Widget
	{	
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;		
		bool m_Clicked;
		
	public:
		CHueVSliderEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_Widget(pPopup->m_pAssetsEditor->m_pGuiConfig),
			m_pPopup(pPopup),
			m_Clicked(false)
		{
			m_Rect.w = m_pConfig->m_ButtonHeight;
			m_Rect.h = 250;
		}
	public:

		void RenderLines(float Cursor)
		{
			Graphics()->LinesBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			IGraphics::CLineItem Lines[4] = {
					IGraphics::CLineItem(m_Rect.x, m_Rect.y, m_Rect.x+m_Rect.w, m_Rect.y),
					IGraphics::CLineItem(m_Rect.x+m_Rect.w, m_Rect.y, m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h),
					IGraphics::CLineItem(m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h, m_Rect.x, m_Rect.y+m_Rect.h),
					IGraphics::CLineItem(m_Rect.x, m_Rect.y+m_Rect.h, m_Rect.x, m_Rect.y)
			};
			Graphics()->LinesDraw(Lines, 4);
			
			IGraphics::CLineItem CursorLine(m_Rect.x, m_Rect.y+m_Rect.h*Cursor, m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h*Cursor);
			Graphics()->LinesDraw(&CursorLine, 1);
			Graphics()->LinesEnd();
		}
		
		virtual void Render()
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			
			int nbCells = 6;
			float Offset = m_Rect.h/static_cast<float>(nbCells);
			for(int i=0; i<nbCells; i++)
			{
				float Value0 = i*1.0f/nbCells;
				float Value1 = (i+1)*1.0f/nbCells;
				vec3 ColorRGB0 = HsvToRgb(vec3(Value0, 1.0f, 1.0f));
				vec3 ColorRGB1 = HsvToRgb(vec3(Value1, 1.0f, 1.0f));

				IGraphics::CColorVertex ColorArray[4];
				ColorArray[0] = IGraphics::CColorVertex(0, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				ColorArray[1] = IGraphics::CColorVertex(1, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				ColorArray[2] = IGraphics::CColorVertex(2, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				ColorArray[3] = IGraphics::CColorVertex(3, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				Graphics()->SetColorVertex(ColorArray, 4);
				IGraphics::CQuadItem QuadItem(m_Rect.x, m_Rect.y+i*Offset, m_Rect.w, Offset);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
		
			Graphics()->QuadsEnd();

			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			RenderLines(ColorHSV.r);
		}

		virtual vec4 GetSliderValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			vec3 HSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			HSV.r = Value;
			vec3 RGB = HsvToRgb(vec3(HSV.r, HSV.g, HSV.b));
			return vec4(RGB.r, RGB.g, RGB.b, Color.a);
		}
		
		void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
		{
			if(m_Rect.IsInside(X, Y) && m_Clicked)
			{
				float Value = static_cast<float>(Y - m_Rect.y)/static_cast<float>(m_Rect.h);
				m_pPopup->SetColor(GetSliderValue(Value));
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
			
			m_Clicked = false;
		}
	};
	
	class CSquarePicker : public CModAPI_ClientGui_Widget
	{	
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;		
		bool m_Clicked;
		
	public:
		CSquarePicker(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_Widget(pPopup->m_pAssetsEditor->m_pGuiConfig),
			m_pPopup(pPopup),
			m_Clicked(false)
		{
			m_Rect.w = 250;
			m_Rect.h = 250;
		}
	public:		
		virtual void Render()
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();

			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			vec3 ColorRGB0 = HsvToRgb(vec3(ColorHSV.x, 0.0f, 1.0f));
			vec3 ColorRGB1 = HsvToRgb(vec3(ColorHSV.x, 1.0f, 1.0f));

			{
				IGraphics::CColorVertex ColorArray[4];
				ColorArray[0] = IGraphics::CColorVertex(0, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				ColorArray[1] = IGraphics::CColorVertex(1, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				ColorArray[2] = IGraphics::CColorVertex(2, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				ColorArray[3] = IGraphics::CColorVertex(3, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				Graphics()->SetColorVertex(ColorArray, 4);
				IGraphics::CQuadItem QuadItem(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
			
			{
				IGraphics::CColorVertex ColorArray[4];
				ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
				ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 0.0f);
				ColorArray[2] = IGraphics::CColorVertex(2, 0.0f, 0.0f, 0.0f, 1.0f);
				ColorArray[3] = IGraphics::CColorVertex(3, 0.0f, 0.0f, 0.0f, 1.0f);
				Graphics()->SetColorVertex(ColorArray, 4);
				IGraphics::CQuadItem QuadItem(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
		
			Graphics()->QuadsEnd();

			Graphics()->LinesBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			IGraphics::CLineItem Lines[4] = {
					IGraphics::CLineItem(m_Rect.x, m_Rect.y, m_Rect.x+m_Rect.w, m_Rect.y),
					IGraphics::CLineItem(m_Rect.x+m_Rect.w, m_Rect.y, m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h),
					IGraphics::CLineItem(m_Rect.x+m_Rect.w, m_Rect.y+m_Rect.h, m_Rect.x, m_Rect.y+m_Rect.h),
					IGraphics::CLineItem(m_Rect.x, m_Rect.y+m_Rect.h, m_Rect.x, m_Rect.y)
			};
			
			vec2 CursorPos = vec2(m_Rect.x + m_Rect.w*ColorHSV.g, m_Rect.y + m_Rect.h*(1.0f-ColorHSV.b));
			IGraphics::CLineItem CursorLines[4] = {
					IGraphics::CLineItem(CursorPos.x - 2.0f, CursorPos.y - 2.0f, CursorPos.x + 2.0f, CursorPos.y - 2.0f),
					IGraphics::CLineItem(CursorPos.x + 2.0f, CursorPos.y - 2.0f, CursorPos.x + 2.0f, CursorPos.y + 2.0f),
					IGraphics::CLineItem(CursorPos.x + 2.0f, CursorPos.y + 2.0f, CursorPos.x - 2.0f, CursorPos.y + 2.0f),
					IGraphics::CLineItem(CursorPos.x - 2.0f, CursorPos.y + 2.0f, CursorPos.x - 2.0f, CursorPos.y - 2.0f),
			};
			
			Graphics()->LinesDraw(CursorLines, 4);
			Graphics()->LinesEnd();
		}
		
		void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
		{
			if(m_Rect.IsInside(X, Y) && m_Clicked)
			{
				float Value0 = static_cast<float>(X - m_Rect.x)/static_cast<float>(m_Rect.w);
				float Value1 = 1.0f - static_cast<float>(Y - m_Rect.y)/static_cast<float>(m_Rect.h);
				
				vec4 Color = m_pPopup->GetColor();
				vec3 HSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
				HSV.g = Value0;
				HSV.b = Value1;
				vec3 RGB = HsvToRgb(vec3(HSV.r, HSV.g, HSV.b));

				m_pPopup->SetColor(vec4(RGB.r, RGB.g, RGB.b, Color.a));
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
			
			m_Clicked = false;
		}
	};
	
	class CWheelPicker : public CModAPI_ClientGui_Widget
	{	
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;		
		int m_Clicked;
		
	public:
		CWheelPicker(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_Widget(pPopup->m_pAssetsEditor->m_pGuiConfig),
			m_pPopup(pPopup),
			m_Clicked(0)
		{
			m_Rect.w = 250;
			m_Rect.h = 250;
		}
	public:		
		virtual void Render()
		{
			Graphics()->TextureClear();

			vec2 Center(m_Rect.x + m_Rect.w/2, m_Rect.y + m_Rect.h/2);
			float Radius1 = min(m_Rect.w, m_Rect.h)/2.0f - 1.0f;
			float Radius0 = Radius1*0.8f;

			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			vec3 ColorRGB0 = HsvToRgb(vec3(ColorHSV.x, 0.0f, 1.0f));
			vec3 ColorRGB1 = HsvToRgb(vec3(ColorHSV.x, 1.0f, 1.0f));

			float CursorAngle = ColorHSV.r*2.0f*pi;
			
			const int nbSegments = 64;
			const float deltaAngle = pi*2.0f/nbSegments;
			
			Graphics()->QuadsBegin();
			
			for(int i=0; i<nbSegments; i++)
			{
				float Hue0 = i*1.0f/nbSegments;
				float Hue1 = (i+1)*1.0f/nbSegments;
				vec3 ColorRGB0 = HsvToRgb(vec3(Hue0, 1.0f, 1.0f));
				vec3 ColorRGB1 = HsvToRgb(vec3(Hue1, 1.0f, 1.0f));

				IGraphics::CColorVertex ColorArray[4];
				ColorArray[0] = IGraphics::CColorVertex(0, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				ColorArray[1] = IGraphics::CColorVertex(1, ColorRGB0.r, ColorRGB0.g, ColorRGB0.b, 1.0f);
				ColorArray[2] = IGraphics::CColorVertex(2, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				ColorArray[3] = IGraphics::CColorVertex(3, ColorRGB1.r, ColorRGB1.g, ColorRGB1.b, 1.0f);
				Graphics()->SetColorVertex(ColorArray, 4);
				
				IGraphics::CFreeformItem Freeform(
					Center.x + Radius1*cos(i*deltaAngle), Center.y + Radius1*sin(i*deltaAngle),
					Center.x + Radius0*cos(i*deltaAngle), Center.y + Radius0*sin(i*deltaAngle),
					Center.x + Radius1*cos((i+1)*deltaAngle), Center.y + Radius1*sin((i+1)*deltaAngle),
					Center.x + Radius0*cos((i+1)*deltaAngle), Center.y + Radius0*sin((i+1)*deltaAngle));
				Graphics()->QuadsDrawFreeform(&Freeform, 1);
			}
			
			{
				IGraphics::CColorVertex ColorArray[4];
				vec3 Hue = HsvToRgb(vec3(ColorHSV.r, 1.0f, 1.0f));
				vec3 Saturation = HsvToRgb(vec3(ColorHSV.r, 1.0f, 0.0f));
				vec3 Value = HsvToRgb(vec3(ColorHSV.r, 0.0f, 1.0f));
				
				ColorArray[0] = IGraphics::CColorVertex(0, Saturation.r, Saturation.g, Saturation.b, 1.0f);
				ColorArray[1] = IGraphics::CColorVertex(1, Saturation.r, Saturation.g, Saturation.b, 1.0f);
				ColorArray[2] = IGraphics::CColorVertex(2, Value.r, Value.g, Value.b, 1.0f);
				ColorArray[3] = IGraphics::CColorVertex(3, Hue.r, Hue.g, Hue.b, 1.0f);
				Graphics()->SetColorVertex(ColorArray, 4);
				
				IGraphics::CFreeformItem Freeform(
					Center.x + Radius0*cos(CursorAngle-2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle-2.0f*pi/3.0f),
					Center.x + Radius0*cos(CursorAngle-2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle-2.0f*pi/3.0f),
					Center.x + Radius0*cos(CursorAngle+2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle+2.0f*pi/3.0f),
					Center.x + Radius0*cos(CursorAngle), Center.y + Radius0*sin(CursorAngle));
				Graphics()->QuadsDrawFreeform(&Freeform, 1);
			}
			
			Graphics()->QuadsEnd();
			
			Graphics()->LinesBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			for(int i=0; i<nbSegments; i++)
			{
				IGraphics::CLineItem Line1(Center.x + Radius1*cos(i*deltaAngle), Center.y + Radius1*sin(i*deltaAngle), Center.x + Radius1*cos((i+1)*deltaAngle), Center.y + Radius1*sin((i+1)*deltaAngle));
				Graphics()->LinesDraw(&Line1, 1);
				IGraphics::CLineItem Line2(Center.x + Radius0*cos(i*deltaAngle), Center.y + Radius0*sin(i*deltaAngle), Center.x + Radius0*cos((i+1)*deltaAngle), Center.y + Radius0*sin((i+1)*deltaAngle));
				Graphics()->LinesDraw(&Line2, 1);
			}
			{
				vec2 HuePos(Center.x + Radius0*cos(CursorAngle), Center.y + Radius0*sin(CursorAngle));
				vec2 SaturationPos(Center.x + Radius0*cos(CursorAngle+2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle+2.0f*pi/3.0f));
				vec2 ValuePos(Center.x + Radius0*cos(CursorAngle-2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle-2.0f*pi/3.0f));
				vec2 Origin = (SaturationPos + HuePos)/2.0f;
				
				IGraphics::CLineItem TriangleLines[3] = {
					IGraphics::CLineItem(HuePos.x, HuePos.y, SaturationPos.x, SaturationPos.y),
					IGraphics::CLineItem(SaturationPos.x, SaturationPos.y, ValuePos.x, ValuePos.y),
					IGraphics::CLineItem(ValuePos.x, ValuePos.y, HuePos.x, HuePos.y),
				};
				Graphics()->LinesDraw(TriangleLines, 3);
				
				vec2 CursorPos = Origin + (HuePos - Origin)*2.0f*(ColorHSV.g-0.5f)*ColorHSV.b + (ValuePos - Origin)*(1.0f-ColorHSV.b);
				IGraphics::CLineItem CursorLines[4] = {
						IGraphics::CLineItem(CursorPos.x - 2.0f, CursorPos.y - 2.0f, CursorPos.x + 2.0f, CursorPos.y - 2.0f),
						IGraphics::CLineItem(CursorPos.x + 2.0f, CursorPos.y - 2.0f, CursorPos.x + 2.0f, CursorPos.y + 2.0f),
						IGraphics::CLineItem(CursorPos.x + 2.0f, CursorPos.y + 2.0f, CursorPos.x - 2.0f, CursorPos.y + 2.0f),
						IGraphics::CLineItem(CursorPos.x - 2.0f, CursorPos.y + 2.0f, CursorPos.x - 2.0f, CursorPos.y - 2.0f),
				};
				Graphics()->LinesDraw(CursorLines, 4);
			}
			
			Graphics()->LinesEnd();
		}
		
		void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
		{
			if(m_Clicked == 1)
			{
				vec2 Center(m_Rect.x + m_Rect.w/2, m_Rect.y + m_Rect.h/2);
				vec2 CursorPos(X, Y);
				float Hue = angle(CursorPos-Center)/(2.0f*pi);
				if(Hue < 0.0f) Hue += 1.0f;
				
				vec4 Color = m_pPopup->GetColor();
				vec3 HSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
				vec3 RGB = HsvToRgb(vec3(Hue, HSV.g, HSV.b));

				m_pPopup->SetColor(vec4(RGB.r, RGB.g, RGB.b, Color.a));
			}
			else if(m_Clicked == 2)
			{
				vec4 Color = m_pPopup->GetColor();
				vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
				float CursorAngle = ColorHSV.r*2.0f*pi;
				
				float Radius1 = min(m_Rect.w, m_Rect.h)/2.0f - 1.0f;
				float Radius0 = Radius1*0.8f;
				
				vec2 Center(m_Rect.x + m_Rect.w/2, m_Rect.y + m_Rect.h/2);
				vec2 HuePos(Center.x + Radius0*cos(CursorAngle), Center.y + Radius0*sin(CursorAngle));
				vec2 SaturationPos(Center.x + Radius0*cos(CursorAngle+2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle+2.0f*pi/3.0f));
				vec2 ValuePos(Center.x + Radius0*cos(CursorAngle-2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle-2.0f*pi/3.0f));
				vec2 Origin = (SaturationPos + HuePos)/2.0f;
				float Size = length(SaturationPos - HuePos);
				float Size2 = length(ValuePos - Origin);
				vec2 CursorPos(X, Y);
				
				float Value = clamp(1.0f-dot(normalize(ValuePos - Origin), CursorPos - Origin)/Size2, 0.0f, 1.0f);
				vec2 ValueOrigin = Origin + (ValuePos - Origin)*(1.0f-Value) - (HuePos - SaturationPos)*Value/2.0f;
				float Saturation = clamp(length(CursorPos - ValueOrigin)/(Size*Value), 0.0f, 1.0f);
				
				vec3 RGB = HsvToRgb(vec3(ColorHSV.r, Saturation, Value));

				m_pPopup->SetColor(vec4(RGB.r, RGB.g, RGB.b, Color.a));
			}
		}

		void OnButtonClick(int X, int Y, int Button)
		{
			if(Button != KEY_MOUSE_1)
				return;
			
			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			float CursorAngle = ColorHSV.r*2.0f*pi;
			
			float Radius1 = min(m_Rect.w, m_Rect.h)/2.0f - 1.0f;
			float Radius0 = Radius1*0.8f;
			
			vec2 Center(m_Rect.x + m_Rect.w/2, m_Rect.y + m_Rect.h/2);
			vec2 HuePos(Center.x + Radius0*cos(CursorAngle), Center.y + Radius0*sin(CursorAngle));
			vec2 SaturationPos(Center.x + Radius0*cos(CursorAngle+2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle+2.0f*pi/3.0f));
			vec2 ValuePos(Center.x + Radius0*cos(CursorAngle-2.0f*pi/3.0f), Center.y + Radius0*sin(CursorAngle-2.0f*pi/3.0f));
			vec2 Origin = (SaturationPos + HuePos)/2.0f;
				
			vec2 CursorPos(X, Y);
			float d = distance(Center, CursorPos);
			if(Radius0 < d && d < Radius1)
			{
				m_Clicked = 1;
			}
			else
			{
				vec2 Vertices[3];
				Vertices[0] = HuePos - CursorPos;
				Vertices[1] = SaturationPos - CursorPos;
				Vertices[2] = ValuePos - CursorPos;
				
				bool isInside = true;
				for(int i=0; i<3; i++)
				{
					vec2 Edge = Vertices[(i+1)%3] - Vertices[i];
					if(Edge.x * Vertices[i].y - Edge.y * Vertices[i].x > 0.0f)
					{
						isInside = false;
						break;
					}
				}
				
				if(isInside)
				{
					m_Clicked = 2;
				}
				else m_Clicked = 0;
			}
		}

		void OnButtonRelease(int Button)
		{
			if(Button != KEY_MOUSE_1)
				return;
			
			m_Clicked = 0;
		}
	};

	template<int C>
	class CRGBIntegerEdit : public CModAPI_ClientGui_AbstractFloatEdit
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;
		
		virtual void SetValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			*(static_cast<float*>(&Color.r)+C) = Value/255.0f;
			m_pPopup->SetColor(Color);
		}
		
		virtual float GetValue()
		{
			vec4 Color = m_pPopup->GetColor();
			float Value = *(static_cast<float*>(&Color.r)+C);
			return Value*255.0f;
		}
		
		virtual void ApplyFormat()
		{
			str_format(m_aFloatText, sizeof(m_aFloatText), "%.00f", GetValue());
		}
		
	public:
		CRGBIntegerEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_AbstractFloatEdit(pPopup->m_pConfig),
			m_pPopup(pPopup)
		{
			
		}
	};

	template<int C>
	class CRGBFloatEdit : public CModAPI_ClientGui_AbstractFloatEdit
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;
		
		virtual void SetValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			*(static_cast<float*>(&Color.r)+C) = Value;
			m_pPopup->SetColor(Color);
		}
		
		virtual float GetValue()
		{
			vec4 Color = m_pPopup->GetColor();
			float Value = *(static_cast<float*>(&Color.r)+C);
			return Value;
		}
		
		virtual void ApplyFormat()
		{
			str_format(m_aFloatText, sizeof(m_aFloatText), "%.03f", GetValue());
		}
		
	public:
		CRGBFloatEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_AbstractFloatEdit(pPopup->m_pConfig),
			m_pPopup(pPopup)
		{
			
		}
	};

	template<int C>
	class CHSVIntegerEdit : public CModAPI_ClientGui_AbstractFloatEdit
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;
		
		virtual void SetValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			*(static_cast<float*>(&ColorHSV.r)+C) = Value/255.0f;
			vec3 ColorRGB = HsvToRgb(vec3(ColorHSV.r, ColorHSV.g, ColorHSV.b));
			m_pPopup->SetColor(vec4(ColorRGB.r, ColorRGB.g, ColorRGB.b, Color.a));
		}
		
		virtual float GetValue()
		{
			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			float Value = *(static_cast<float*>(&ColorHSV.r)+C);
			return Value*255.0f;
		}
		
		virtual void ApplyFormat()
		{
			str_format(m_aFloatText, sizeof(m_aFloatText), "%.00f", GetValue());
		}
		
	public:
		CHSVIntegerEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_AbstractFloatEdit(pPopup->m_pConfig),
			m_pPopup(pPopup)
		{
			
		}
	};

	template<int C>
	class CHSVFloatEdit : public CModAPI_ClientGui_AbstractFloatEdit
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_ColorEdit* m_pPopup;
		
		virtual void SetValue(float Value)
		{
			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			*(static_cast<float*>(&ColorHSV.r)+C) = Value;
			vec3 ColorRGB = HsvToRgb(vec3(ColorHSV.r, ColorHSV.g, ColorHSV.b));
			m_pPopup->SetColor(vec4(ColorRGB.r, ColorRGB.g, ColorRGB.b, Color.a));
		}
		
		virtual float GetValue()
		{
			vec4 Color = m_pPopup->GetColor();
			vec3 ColorHSV = RgbToHsv(vec3(Color.r, Color.g, Color.b));
			float Value = *(static_cast<float*>(&ColorHSV.r)+C);
			return Value;
		}
		
		virtual void ApplyFormat()
		{
			str_format(m_aFloatText, sizeof(m_aFloatText), "%.03f", GetValue());
		}
		
	public:
		CHSVFloatEdit(CModAPI_AssetsEditorGui_Popup_ColorEdit* pPopup) :
			CModAPI_ClientGui_AbstractFloatEdit(pPopup->m_pConfig),
			m_pPopup(pPopup)
		{
			
		}
	};
	
public:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_AssetPath m_AssetPath;
	int m_AssetMember;
	int m_AssetSubPath;
	
protected:
	CModAPI_ClientGui_Tabs* m_Tabs;
	CModAPI_ClientGui_VListLayout* m_TabRGB;
	CModAPI_ClientGui_VListLayout* m_TabHSV;
	CModAPI_ClientGui_VListLayout* m_TabSquare;
	CModAPI_ClientGui_VListLayout* m_TabWheel;
	
public:
	CModAPI_AssetsEditorGui_Popup_ColorEdit(
		CModAPI_AssetsEditor* pAssetsEditor,
		const CModAPI_ClientGui_Rect& CreatorRect,
		int Alignment,
		CModAPI_AssetPath AssetPath,
		int AssetMember,
		int AssetSubPath
	) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 300, 380, Alignment),
		m_pAssetsEditor(pAssetsEditor),
		m_AssetPath(AssetPath),
		m_AssetMember(AssetMember),
		m_AssetSubPath(AssetSubPath)
	{		
		m_Tabs = new CModAPI_ClientGui_Tabs(pAssetsEditor->m_pGuiConfig);
		Add(m_Tabs);
		
		Update();
		
		m_TabRGB = new CModAPI_ClientGui_VListLayout(pAssetsEditor->m_pGuiConfig);
		m_Tabs->AddTab(m_TabRGB, MODAPI_ASSETSEDITOR_ICON_COLORPICKER_RGB, "RGB Color Chooser");
		
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabRGB->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Red:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<0>(this));
			pLayout->Add(new CRGBIntegerEdit<0>(this));
			
			m_TabRGB->Add(new CRGBSliderEdit<0>(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabRGB->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Green:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<1>(this));
			pLayout->Add(new CRGBIntegerEdit<1>(this));
			
			m_TabRGB->Add(new CRGBSliderEdit<1>(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabRGB->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Blue:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<2>(this));
			pLayout->Add(new CRGBIntegerEdit<2>(this));
			
			m_TabRGB->Add(new CRGBSliderEdit<2>(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabRGB->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Alpha:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<3>(this));
			pLayout->Add(new CRGBIntegerEdit<3>(this));
			
			m_TabRGB->Add(new CAlphaSliderEdit(this));
		}
		
		m_TabHSV = new CModAPI_ClientGui_VListLayout(pAssetsEditor->m_pGuiConfig);
		m_Tabs->AddTab(m_TabHSV, MODAPI_ASSETSEDITOR_ICON_COLORPICKER_HSV, "HSV Color Chooser");
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabHSV->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Hue:");
			pLayout->Add(pLabel);
			pLayout->Add(new CHSVFloatEdit<0>(this));
			pLayout->Add(new CHSVIntegerEdit<0>(this));
			
			m_TabHSV->Add(new CHSVSliderEdit<0>(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabHSV->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Saturation:");
			pLayout->Add(pLabel);
			pLayout->Add(new CHSVFloatEdit<1>(this));
			pLayout->Add(new CHSVIntegerEdit<1>(this));
			
			m_TabHSV->Add(new CHSVSliderEdit<1>(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabHSV->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Value:");
			pLayout->Add(pLabel);
			pLayout->Add(new CHSVFloatEdit<2>(this));
			pLayout->Add(new CHSVIntegerEdit<2>(this));
			
			m_TabHSV->Add(new CHSVSliderEdit<2>(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabHSV->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Alpha:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<3>(this));
			pLayout->Add(new CRGBIntegerEdit<3>(this));
			
			m_TabHSV->Add(new CAlphaSliderEdit(this));
		}
		
		m_TabSquare = new CModAPI_ClientGui_VListLayout(pAssetsEditor->m_pGuiConfig);
		m_Tabs->AddTab(m_TabSquare, MODAPI_ASSETSEDITOR_ICON_COLORPICKER_SQUARE, "HSV Square Color Chooser");
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_LAST);
			pLayout->SetHeight(250);
			m_TabSquare->Add(pLayout);
			
			pLayout->Add(new CSquarePicker(this));
			pLayout->Add(new CHueVSliderEdit(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabSquare->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Alpha:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<3>(this));
			pLayout->Add(new CRGBIntegerEdit<3>(this));
			
			m_TabSquare->Add(new CAlphaSliderEdit(this));
		}
		
		m_TabWheel = new CModAPI_ClientGui_VListLayout(pAssetsEditor->m_pGuiConfig);
		m_Tabs->AddTab(m_TabWheel, MODAPI_ASSETSEDITOR_ICON_COLORPICKER_WHEEL, "HSV Wheel Color Chooser");
		{
			m_TabWheel->Add(new CWheelPicker(this));
		}
		{
			CModAPI_ClientGui_HListLayout* pLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_ALL);
			pLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_TabWheel->Add(pLayout);
			
			CModAPI_ClientGui_Label* pLabel = new CModAPI_ClientGui_Label(m_pAssetsEditor->m_pGuiConfig, "Alpha:");
			pLayout->Add(pLabel);
			pLayout->Add(new CRGBFloatEdit<3>(this));
			pLayout->Add(new CRGBIntegerEdit<3>(this));
			
			m_TabWheel->Add(new CAlphaSliderEdit(this));
		}
		
		Update();
	}
	
	void SetColor(vec4 Color)
	{
		m_pAssetsEditor->AssetManager()->SetAssetValue<vec4>(
			m_AssetPath,
			m_AssetMember,
			m_AssetSubPath,
			Color);
	}
	
	vec4 GetColor()
	{
		return m_pAssetsEditor->AssetManager()->GetAssetValue<vec4>(
			m_AssetPath,
			m_AssetMember,
			m_AssetSubPath,
			vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}
};

/* SAVE ASSETS ********************************************************/

class CModAPI_AssetsEditorGui_Popup_SaveAssets : public CModAPI_ClientGui_Popup
{	
protected:
	class CSave : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_SaveAssets* m_pPopup;
		
	public:
		CSave(CModAPI_AssetsEditorGui_Popup_SaveAssets* pPopup) :
			CModAPI_ClientGui_TextButton(pPopup->m_pConfig, "Save"),
			m_pPopup(pPopup)
		{ SetWidth(120); }

		virtual void MouseClickAction() { m_pPopup->Save(); }
	};
	
	class CItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_SaveAssets* m_pPopup;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		char m_aFilename[128];
		int m_StorageType;
		int m_IsDirectory;
		
	protected:
		virtual void MouseClickAction()
		{
			if(!m_IsDirectory)
			{
				m_pPopup->SetFilename(m_aFilename);
			}
		}
		
	public:
		CItem(CModAPI_AssetsEditorGui_Popup_SaveAssets* pPopup, const char* pFilename, int StorageType, int IsDir) :
			CModAPI_ClientGui_TextButton(pPopup->m_pAssetsEditor->m_pGuiConfig, pFilename, MODAPI_ASSETSEDITOR_ICON_ASSET),
			m_pPopup(pPopup),
			m_pAssetsEditor(pPopup->m_pAssetsEditor)
		{
			m_Centered = false;
			
			str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
			m_StorageType = StorageType;
			m_IsDirectory = IsDir;
		}
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_ClientGui_VListLayout* m_Layout;
	CModAPI_ClientGui_VListLayout* m_Filelist;
	char m_aFilename[256];
	
public:
	CModAPI_AssetsEditorGui_Popup_SaveAssets(CModAPI_AssetsEditor* pAssetsEditor, const CModAPI_ClientGui_Rect& CreatorRect, int Alignment) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 500, 500, Alignment),
		m_pAssetsEditor(pAssetsEditor)
	{
		str_copy(m_aFilename, "myassets.assets", sizeof(m_aFilename));
		
		m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		Add(m_Layout);
		
		m_Filelist = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		m_Layout->Add(m_Filelist);
		
		{
			CModAPI_ClientGui_HListLayout* pHLayout = new CModAPI_ClientGui_HListLayout(m_pAssetsEditor->m_pGuiConfig, MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE, MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST);
			pHLayout->SetHeight(m_pConfig->m_ButtonHeight);
			m_Layout->Add(pHLayout);
			
			CModAPI_ClientGui_ExternalTextEdit* pTextEdit = new CModAPI_ClientGui_ExternalTextEdit(m_pAssetsEditor->m_pGuiConfig, m_aFilename, sizeof(m_aFilename));
			pHLayout->Add(pTextEdit);
			
			pHLayout->Add(new CSave(this));
		}
			
		
		CModAPI_ClientGui_Popup::Update();
		m_Filelist->SetHeight(m_Layout->m_Rect.h - 2*m_pConfig->m_LayoutSpacing - m_pConfig->m_ButtonHeight);
		Update();
	}
	
	static int FileListFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
	{
		CModAPI_AssetsEditorGui_Popup_SaveAssets* pPopup = static_cast<CModAPI_AssetsEditorGui_Popup_SaveAssets*>(pUser);
		
		int Length = str_length(pName);
		if(pName[0] == '.' && (pName[1] == 0))
			return 0;
		
		if(Length < 7 || str_comp(pName+Length-7, ".assets"))
			return 0;

		CItem* pItem = new CItem(pPopup, pName, StorageType, IsDir);
		pPopup->m_Filelist->Add(pItem);

		return 0;
	}
	
	virtual void Update()
	{
		m_pAssetsEditor->Storage()->ListDirectory(IStorage::TYPE_ALL, "assets", FileListFetchCallback, this);
		
		CModAPI_ClientGui_Popup::Update();
	}
	
	void SetFilename(const char* pFilename)
	{
		str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
	}
	
	void Save()
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "assets/%s", m_aFilename);
		m_pAssetsEditor->AssetManager()->SaveInAssetsFile(aBuf, CModAPI_AssetPath::SRC_EXTERNAL);
		Close();
	}
};

/* LOAD ASSETS ********************************************************/

class CModAPI_AssetsEditorGui_Popup_LoadAssets : public CModAPI_ClientGui_Popup
{	
protected:	
	class CItem : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_LoadAssets* m_pPopup;
		CModAPI_AssetsEditor* m_pAssetsEditor;
		char m_aFilename[128];
		int m_StorageType;
		int m_IsDirectory;
		
	protected:
		virtual void MouseClickAction()
		{
			if(!m_IsDirectory)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "assets/%s", m_aFilename);
				m_pAssetsEditor->LoadAssetsFile(aBuf);
				m_pPopup->Close();
			}
		}
		
	public:
		CItem(CModAPI_AssetsEditorGui_Popup_LoadAssets* pPopup, const char* pFilename, int StorageType, int IsDir) :
			CModAPI_ClientGui_TextButton(pPopup->m_pAssetsEditor->m_pGuiConfig, pFilename, MODAPI_ASSETSEDITOR_ICON_ASSET),
			m_pPopup(pPopup),
			m_pAssetsEditor(pPopup->m_pAssetsEditor)
		{
			m_Centered = false;
			
			str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
			m_StorageType = StorageType;
			m_IsDirectory = IsDir;
		}
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_ClientGui_VListLayout* m_Layout;
	CModAPI_ClientGui_VListLayout* m_Filelist;
	
public:
	CModAPI_AssetsEditorGui_Popup_LoadAssets(CModAPI_AssetsEditor* pAssetsEditor, const CModAPI_ClientGui_Rect& CreatorRect, int Alignment) :
		CModAPI_ClientGui_Popup(pAssetsEditor->m_pGuiConfig, CreatorRect, 500, 500, Alignment),
		m_pAssetsEditor(pAssetsEditor)
	{		
		m_Layout = new CModAPI_ClientGui_VListLayout(m_pAssetsEditor->m_pGuiConfig);
		Add(m_Layout);
		
		Update();
	}
	
	static int FileListFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
	{
		CModAPI_AssetsEditorGui_Popup_LoadAssets* pPopup = static_cast<CModAPI_AssetsEditorGui_Popup_LoadAssets*>(pUser);
		
		int Length = str_length(pName);
		if(pName[0] == '.' && (pName[1] == 0))
			return 0;
		
		if(Length < 7 || str_comp(pName+Length-7, ".assets"))
			return 0;

		CItem* pItem = new CItem(pPopup, pName, StorageType, IsDir);
		pPopup->m_Layout->Add(pItem);

		return 0;
	}
	
	virtual void Update()
	{
		m_pAssetsEditor->Storage()->ListDirectory(IStorage::TYPE_ALL, "assets", FileListFetchCallback, this);
		
		CModAPI_ClientGui_Popup::Update();
	}
};

#endif
