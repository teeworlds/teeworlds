#ifndef MODAPI_ASSETSEDITOR_H
#define MODAPI_ASSETSEDITOR_H

#include <base/vmath.h>
#include <engine/kernel.h>
#include <game/client/render.h>
#include <modapi/client/gui/popup.h>
#include <modapi/client/gui/button.h>

enum
{
	MODAPI_ASSETSEDITOR_ICON_DECREASE=2,
	MODAPI_ASSETSEDITOR_ICON_INCREASE,
	MODAPI_ASSETSEDITOR_ICON_DELETE,
	MODAPI_ASSETSEDITOR_ICON_ROTATION,
	MODAPI_ASSETSEDITOR_ICON_OPACITY,
	MODAPI_ASSETSEDITOR_ICON_TRANSLATE_X,
	MODAPI_ASSETSEDITOR_ICON_TRANSLATE_Y,
	MODAPI_ASSETSEDITOR_ICON_EDIT,
	MODAPI_ASSETSEDITOR_ICON_VIEW,
	MODAPI_ASSETSEDITOR_ICON_UP,
	MODAPI_ASSETSEDITOR_ICON_DOWN,
	MODAPI_ASSETSEDITOR_ICON_DUPLICATE,
	
	MODAPI_ASSETSEDITOR_ICON_FIRST_FRAME=16,
	MODAPI_ASSETSEDITOR_ICON_PREV_FRAME,
	MODAPI_ASSETSEDITOR_ICON_PLAY,
	MODAPI_ASSETSEDITOR_ICON_PAUSE,
	MODAPI_ASSETSEDITOR_ICON_NEXT_FRAME,
	MODAPI_ASSETSEDITOR_ICON_LAST_FRAME,
	
	
	MODAPI_ASSETSEDITOR_ICON_MAGNET_CIRCLE = 32,
	MODAPI_ASSETSEDITOR_ICON_MAGNET_TRIANGLE,
	MODAPI_ASSETSEDITOR_ICON_MAGNET_ROTATION,
};

enum
{
	MODAPI_ASSETSEDITOR_MEMBER_NAME,
	MODAPI_ASSETSEDITOR_MEMBER_IMAGE_GRIDWIDTH,
	MODAPI_ASSETSEDITOR_MEMBER_IMAGE_GRIDHEIGHT,
	MODAPI_ASSETSEDITOR_MEMBER_SPRITE_IMAGEPATH,
	MODAPI_ASSETSEDITOR_MEMBER_SPRITE_X,
	MODAPI_ASSETSEDITOR_MEMBER_SPRITE_Y,
	MODAPI_ASSETSEDITOR_MEMBER_SPRITE_WIDTH,
	MODAPI_ASSETSEDITOR_MEMBER_SPRITE_HEIGHT,
	MODAPI_ASSETSEDITOR_MEMBER_ANIMATION_CYCLETYPE,
	MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_INDEX,
	MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_ANGLE,
	MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_POSX,
	MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_POSY,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BODYANIMPATH,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKFOOTANIMPATH,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTFOOTANIMPATH,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDANIMPATH,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDALIGNMENT,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDOFFSETX,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDOFFSETY,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDANIMPATH,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDALIGNMENT,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDOFFSETX,
	MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDOFFSETY,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACH_TEEANIMATIONPATH,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACH_CURSORPATH,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_SPRITEPATH,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_SIZE,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_ANIMATIONPATH,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_ALIGNMENT,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_OFFSETX,
	MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_OFFSETY,
};

class IModAPI_AssetsEditor : public IInterface
{
	MACRO_INTERFACE("assetseditor", 0)
public:

	virtual ~IModAPI_AssetsEditor() {}
	virtual void Init(class CModAPI_Client_Graphics* pModAPIGraphics) = 0;
	virtual void UpdateAndRender() = 0;
	virtual bool HasUnsavedData() const = 0;
};

extern IModAPI_AssetsEditor *CreateAssetsEditor();

class CModAPI_AssetsEditor : public IModAPI_AssetsEditor
{
private:
	class IClient *m_pClient;
	class IInput *m_pInput;
	class IGraphics *m_pGraphics;
	class CModAPI_Client_Graphics *m_pModAPIGraphics;
	class ITextRender *m_pTextRender;
	class IStorage *m_pStorage;
	class IModAPI_AssetsFile *m_pAssetsFile;
	
	CRenderTools m_RenderTools;
	
	bool m_ShowCursor;
	vec2 m_MouseDelta;
	vec2 m_MousePos;
	int m_MouseButton1;
	
	float m_Time;
	float m_LastTime;
	bool m_Paused;
	
	IGraphics::CTextureHandle m_CursorTexture;
	
	bool m_ClosePopup;
	bool m_RefreshAssetEditor;

public:
	class CModAPI_ClientGui_Config *m_pGuiConfig;
	IGraphics::CTextureHandle m_ModEditorTexture;
	IGraphics::CTextureHandle m_SkinTexture[6];
	
	class CModAPI_ClientGui_HListLayout* m_pGuiToolbar;
	class CModAPI_ClientGui_VListLayout* m_pGuiAssetList;
	class CModAPI_AssetsEditorGui_Editor* m_pGuiAssetEditor;
	class CModAPI_AssetsEditorGui_View* m_pGuiView;
	class CModAPI_AssetsEditorGui_Timeline* m_pGuiTimeline;
	class CModAPI_ClientGui_Popup* m_pGuiPopup;
	
	int m_EditedAssetType;
	CModAPI_AssetPath m_EditedAssetPath;
	int m_EditedAssetFrame;
	
	int m_ViewedAssetType;
	CModAPI_AssetPath m_ViewedAssetPath;
	
private:
	void Render();

public:
	CModAPI_AssetsEditor();
	virtual ~CModAPI_AssetsEditor();
	
	virtual void Init(class CModAPI_Client_Graphics* pModAPIGraphics);
	virtual void UpdateAndRender();
	virtual bool HasUnsavedData() const;
	
	class IClient *Client() { return m_pClient; };
	class IInput *Input() { return m_pInput; };
	class IGraphics *Graphics() { return m_pGraphics; };
	class CModAPI_Client_Graphics *ModAPIGraphics() { return m_pModAPIGraphics; };
	class ITextRender *TextRender() { return m_pTextRender; };
	class CRenderTools *RenderTools() { return &m_RenderTools; }
	class IStorage *Storage() { return m_pStorage; };
	class IModAPI_AssetsFile *AssetsFile() { return m_pAssetsFile; };
	
	void SetPause(bool Pause);
	bool IsPaused();
	void SetTime(float Time);
	float GetTime();
	
	void LoadAssetsFile();
	void SaveAssetsFile();
	void CloseEditor();
	
	void RefreshAssetList();
	void RefreshAssetEditor();
	void DisplayPopup();
	void ClosePopup();
	void ShowCursor();
	void HideCursor();
	void DisplayPopup(CModAPI_ClientGui_Popup* pWidget);
	void EditAsset(int AssetType, CModAPI_AssetPath AssetPath);
	void EditAssetFrame(int FrameId);
	void DisplayAsset(int AssetType, CModAPI_AssetPath AssetPath);
	void DeleteAsset(int AssetType, CModAPI_AssetPath AssetPath);
	void NewAsset(int AssetType, CModAPI_AssetPath AssetPath);
	
	bool IsEditedAsset(int AssetType, CModAPI_AssetPath AssetPath);
	bool IsDisplayedAsset(int AssetType, CModAPI_AssetPath AssetPath);
};

class CModAPI_AssetsEditorGui_Popup : public CModAPI_ClientGui_Popup
{
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	CModAPI_ClientGui_VListLayout* m_Layout;
	
public:
	CModAPI_AssetsEditorGui_Popup(CModAPI_AssetsEditor* pAssetsEditor);
};

class CModAPI_AssetsEditorGui_Popup_AddImage : public CModAPI_AssetsEditorGui_Popup
{
protected:
	class CCancel : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_AddImage* m_pPopup;
		
	public:
		CCancel(CModAPI_AssetsEditorGui_Popup_AddImage* pPopup);
		void Cancel();
		virtual void MouseClickAction();
	};
	
public:
	CModAPI_AssetsEditorGui_Popup_AddImage(CModAPI_AssetsEditor* pAssetsEditor);
	static int FileListFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);
	virtual void Update();
	void Cancel();
};

class CModAPI_AssetsEditorGui_Popup_AssetEditList : public CModAPI_AssetsEditorGui_Popup
{
protected:
	class CCancel : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Popup_AssetEditList* m_pPopup;
		
	public:
		CCancel(CModAPI_AssetsEditorGui_Popup_AssetEditList* pPopup);
		void Cancel();
		virtual void MouseClickAction();
	};
	
protected:
	int m_ParentAssetType;
	CModAPI_AssetPath m_ParentAssetPath;
	int m_ParentAssetMember;
	int m_ParentAssetSubId;
	int m_FieldAssetType;
	
protected:
	void AddListElement(CModAPI_AssetPath Path);
	
public:
	CModAPI_AssetsEditorGui_Popup_AssetEditList(CModAPI_AssetsEditor* pAssetsEditor, int ParentAssetType, CModAPI_AssetPath ParentAssetPath, int ParentAssetMember, int ParentAssetSubId, int FieldAssetType);
	virtual void Update();
	void Cancel();
};

template<class T>
T* GetAssetMemberPointer(CModAPI_AssetsEditor* pAssetsEditor, int AssetType, CModAPI_AssetPath AssetPath, int Member, int SubId = -1)
{
	CModAPI_Asset* pBaseAsset = 0;
	char* pAsset = 0;
	switch(AssetType)
	{
		case MODAPI_ASSETTYPE_IMAGE:
		{
			CModAPI_Asset_Image* pTypedAsset = pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.GetAsset(AssetPath);
			pBaseAsset = pTypedAsset;
			pAsset = (char*) pTypedAsset;
			break;
		}
		case MODAPI_ASSETTYPE_SPRITE:
		case MODAPI_ASSETTYPE_SPRITE_LIST:
		{
			if(AssetPath.IsList())
			{
				CModAPI_Asset_List* pTypedAsset = pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetList(AssetPath);
				pBaseAsset = pTypedAsset;
				pAsset = (char*) pTypedAsset;
			}
			else
			{
				CModAPI_Asset_Sprite* pTypedAsset = pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetAsset(AssetPath);
				pBaseAsset = pTypedAsset;
				pAsset = (char*) pTypedAsset;
			}
			break;
		}
		case MODAPI_ASSETTYPE_ANIMATION:
		{
			CModAPI_Asset_Animation* pTypedAsset = pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(AssetPath);
			pBaseAsset = pTypedAsset;
			pAsset = (char*) pTypedAsset;
			break;
		}
		case MODAPI_ASSETTYPE_TEEANIMATION:
		{
			CModAPI_Asset_TeeAnimation* pTypedAsset = pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(AssetPath);
			pBaseAsset = pTypedAsset;
			pAsset = (char*) pTypedAsset;
			break;
		}
		case MODAPI_ASSETTYPE_ATTACH:
		{
			CModAPI_Asset_Attach* pTypedAsset = pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(AssetPath);
			pBaseAsset = pTypedAsset;
			pAsset = (char*) pTypedAsset;
			break;
		}
	}
	
	if(!pAsset)
		return 0;
	
	switch(Member)
	{
		case MODAPI_ASSETSEDITOR_MEMBER_NAME:
			return (T*)&pBaseAsset->m_aName;
			
		case MODAPI_ASSETSEDITOR_MEMBER_IMAGE_GRIDWIDTH:
			return (T*)&((CModAPI_Asset_Image*) pAsset)->m_GridWidth;
		case MODAPI_ASSETSEDITOR_MEMBER_IMAGE_GRIDHEIGHT:
			return (T*)&((CModAPI_Asset_Image*) pAsset)->m_GridHeight;
			
		case MODAPI_ASSETSEDITOR_MEMBER_SPRITE_IMAGEPATH:
			return (T*)&((CModAPI_Asset_Sprite*) pAsset)->m_ImagePath;
		case MODAPI_ASSETSEDITOR_MEMBER_SPRITE_X:
			return (T*)&((CModAPI_Asset_Sprite*) pAsset)->m_X;
		case MODAPI_ASSETSEDITOR_MEMBER_SPRITE_Y:
			return (T*)&((CModAPI_Asset_Sprite*) pAsset)->m_Y;
		case MODAPI_ASSETSEDITOR_MEMBER_SPRITE_WIDTH:
			return (T*)&((CModAPI_Asset_Sprite*) pAsset)->m_Width;
		case MODAPI_ASSETSEDITOR_MEMBER_SPRITE_HEIGHT:
			return (T*)&((CModAPI_Asset_Sprite*) pAsset)->m_Height;
			
		case MODAPI_ASSETSEDITOR_MEMBER_ANIMATION_CYCLETYPE:
			return (T*)&((CModAPI_Asset_Animation*) pAsset)->m_CycleType;
			
		case MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_INDEX:
			return (T*)&((CModAPI_Asset_Animation*) pAsset)->m_lKeyFrames[SubId].m_ListId;
		case MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_ANGLE:
			return (T*)&((CModAPI_Asset_Animation*) pAsset)->m_lKeyFrames[SubId].m_Angle;
		case MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_POSX:
			return (T*)&((CModAPI_Asset_Animation*) pAsset)->m_lKeyFrames[SubId].m_Pos.x;
		case MODAPI_ASSETSEDITOR_MEMBER_ANIMATIONFRAME_POSY:
			return (T*)&((CModAPI_Asset_Animation*) pAsset)->m_lKeyFrames[SubId].m_Pos.y;
			
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BODYANIMPATH:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_BodyAnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKFOOTANIMPATH:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_BackFootAnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTFOOTANIMPATH:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_FrontFootAnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDANIMPATH:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_BackHandAnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDALIGNMENT:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_BackHandAlignment;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDOFFSETX:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_BackHandOffsetX;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_BACKHANDOFFSETY:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_BackHandOffsetY;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDANIMPATH:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_FrontHandAnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDALIGNMENT:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_FrontHandAlignment;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDOFFSETX:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_FrontHandOffsetX;
		case MODAPI_ASSETSEDITOR_MEMBER_TEEANIMATION_FRONTHANDOFFSETY:
			return (T*)&((CModAPI_Asset_TeeAnimation*) pAsset)->m_FrontHandOffsetY;
			
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACH_TEEANIMATIONPATH:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_TeeAnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACH_CURSORPATH:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_CursorPath;
			
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_SPRITEPATH:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_BackElements[SubId].m_SpritePath;
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_SIZE:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_BackElements[SubId].m_Size;
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_ANIMATIONPATH:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_BackElements[SubId].m_AnimationPath;
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_ALIGNMENT:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_BackElements[SubId].m_Alignment;
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_OFFSETX:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_BackElements[SubId].m_OffsetX;
		case MODAPI_ASSETSEDITOR_MEMBER_ATTACHELEM_OFFSETY:
			return (T*)&((CModAPI_Asset_Attach*) pAsset)->m_BackElements[SubId].m_OffsetY;
	}
	
	return 0;
}

#endif
