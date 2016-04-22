#ifndef MODAPI_ASSETSEDITOR_H
#define MODAPI_ASSETSEDITOR_H

#include <base/vmath.h>
#include <engine/kernel.h>
#include <game/client/render.h>
#include <modapi/client/gui/popup.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/tabs.h>
#include <modapi/client/gui/label.h>

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
	
	MODAPI_ASSETSEDITOR_ICON_ASSET = 48,
	MODAPI_ASSETSEDITOR_ICON_INTERNAL_ASSET,
	MODAPI_ASSETSEDITOR_ICON_EXTERNAL_ASSET,
	MODAPI_ASSETSEDITOR_ICON_MAP_ASSET,
	MODAPI_ASSETSEDITOR_ICON_SKIN_ASSET,
	MODAPI_ASSETSEDITOR_ICON_LAYERS,
	MODAPI_ASSETSEDITOR_ICON_BONE,
	MODAPI_ASSETSEDITOR_ICON_SPRITE,
	MODAPI_ASSETSEDITOR_ICON_IMAGE,
	MODAPI_ASSETSEDITOR_ICON_SKELETON,
	MODAPI_ASSETSEDITOR_ICON_SKELETONSKIN,
	MODAPI_ASSETSEDITOR_ICON_SKELETONANIMATION,
	MODAPI_ASSETSEDITOR_ICON_LAYERANIMATION,
	
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_MOVE = 64,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_TRANSLATE,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_TRANSLATE_X,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_TRANSLATE_Y,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_ROTATE,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_SCALE,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_SCALE_X,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_SCALE_Y,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_BONE_LENGTH,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_BONE_ADD,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_BONE_DELETE,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_BONE_ATTACH,
	
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_FRAME_MOVE = 80,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_FRAME_ADD,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_FRAME_DELETE,
	MODAPI_ASSETSEDITOR_ICON_FRAMES,
	MODAPI_ASSETSEDITOR_ICON_CURSORTOOL_FRAME_COLOR,
	MODAPI_ASSETSEDITOR_ICON_KEYFRAME_BONE,
	MODAPI_ASSETSEDITOR_ICON_KEYFRAME_LAYER,
	
	MODAPI_ASSETSEDITOR_ICON_COLORPICKER_RGB = 96,
	MODAPI_ASSETSEDITOR_ICON_COLORPICKER_HSV,
	MODAPI_ASSETSEDITOR_ICON_COLORPICKER_SQUARE,
	MODAPI_ASSETSEDITOR_ICON_COLORPICKER_WHEEL,
};

class IModAPI_AssetsEditor : public IInterface
{
	MACRO_INTERFACE("assetseditor", 0)
public:

	virtual ~IModAPI_AssetsEditor() {}
	virtual void Init(class CModAPI_AssetManager* pAssetManager, class CModAPI_Client_Graphics* pModAPIGraphics) = 0;
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
	class CModAPI_AssetManager *m_pAssetManager;
	class ITextRender *m_pTextRender;
	class IStorage *m_pStorage;
	class IModAPI_AssetsFile *m_pAssetsFile;
	
	CRenderTools m_RenderTools;
	
	bool m_ShowCursor;
	vec2 m_MouseDelta;
	vec2 m_MousePos;
	int m_MouseButton[3];
	
	float m_Time;
	float m_LastTime;
	bool m_Paused;
	
	IGraphics::CTextureHandle m_CursorTexture;
	
	bool m_RefreshAssetEditor;

public:
	class CModAPI_ClientGui_Config *m_pGuiConfig;
	IGraphics::CTextureHandle m_ModEditorTexture;
	IGraphics::CTextureHandle m_SkinTexture[6];
	
	bool m_Hint;
	class CModAPI_ClientGui_Label* m_pHintLabel;
	class CModAPI_ClientGui_HListLayout* m_pGuiToolbar;
	class CModAPI_ClientGui_VListLayout* m_pGuiAssetList[CModAPI_AssetPath::NUM_SOURCES];
	class CModAPI_ClientGui_Tabs* m_pGuiAssetListTabs;
	class CModAPI_AssetsEditorGui_Editor* m_pGuiAssetEditor;
	class CModAPI_AssetsEditorGui_View* m_pGuiView;
	class CModAPI_AssetsEditorGui_Timeline* m_pGuiTimeline;
	array<class CModAPI_ClientGui_Popup*> m_GuiPopups;
	
	int m_AssetsListSource;
	CModAPI_AssetPath m_EditedAssetPath;
	int m_EditedAssetSubPath;
	int m_EditorTab;
	
	CModAPI_AssetPath m_ViewedAssetPath;
	
private:
	void Render();
	void TimeWrap();

public:
	CModAPI_AssetsEditor();
	virtual ~CModAPI_AssetsEditor();
	
	virtual void Init(class CModAPI_AssetManager* pAssetManager, class CModAPI_Client_Graphics* pModAPIGraphics);
	virtual void UpdateAndRender();
	virtual bool HasUnsavedData() const;
	
	class IClient *Client() { return m_pClient; };
	class IInput *Input() { return m_pInput; };
	class IGraphics *Graphics() { return m_pGraphics; };
	class CModAPI_Client_Graphics *ModAPIGraphics() { return m_pModAPIGraphics; };
	class CModAPI_AssetManager *AssetManager() { return m_pAssetManager; };
	class ITextRender *TextRender() { return m_pTextRender; };
	class CRenderTools *RenderTools() { return &m_RenderTools; }
	class IStorage *Storage() { return m_pStorage; };
	class IModAPI_AssetsFile *AssetsFile() { return m_pAssetsFile; };
	
	void SetPause(bool Pause);
	bool IsPaused();
	void SetTime(float Time);
	float GetTime();
	
	void LoadAssetsFile(const char* pFilename);
	void CloseEditor();
	
	void RefreshAssetList(int Source);
	void RefreshAssetList();
	void RefreshAssetEditor(int Tab=-1);
	void DisplayPopup();
	void ShowCursor();
	void HideCursor();
	void DisplayPopup(CModAPI_ClientGui_Popup* pWidget);
	void EditAsset(CModAPI_AssetPath AssetPath);
	void EditAssetSubItem(CModAPI_AssetPath AssetPath, int ItemPath, int Tab=-1);
	void EditAssetFirstFrame();
	void EditAssetLastFrame();
	void EditAssetPrevFrame();
	void EditAssetNextFrame();
	void DisplayAsset(CModAPI_AssetPath AssetPath);
	void DeleteAsset(CModAPI_AssetPath AssetPath);
	void NewAsset(CModAPI_AssetPath AssetPath);
	
	bool IsEditedAsset(CModAPI_AssetPath AssetPath);
	bool IsDisplayedAsset(CModAPI_AssetPath AssetPath);
	
	void ShowHint(const char* pText);
};

#endif
