// LordSk
#ifndef GAME_EDITOR_EDITOR2_H
#define GAME_EDITOR_EDITOR2_H

#include <string.h> // memset
#include <base/system.h>

#include <engine/editor.h>
#include <engine/config.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include <generated/client_data.h>
#include <game/client/components/mapimages.h>

#include "ed_map2.h"
#include "ed_console2.h"

class IStorage;
class IGraphics;
class IInput;
class IClient;
class IConsole;
class ITextRender;
class IStorage;

struct CUIButton
{
	u8 m_Hovered;
	u8 m_Pressed;
	u8 m_Clicked;

	CUIButton()
	{
		m_Hovered = false;
		m_Pressed = false;
		m_Clicked = false;
	}
};

struct CUITextInput
{
	CUIButton m_Button;
	u8 m_Selected;
	CLineInput m_LineInput;
	int m_CursorPos;

	CUITextInput()
	{
		m_Selected = false;
	}
};

struct CUIIntegerInput
{
	CUITextInput m_TextInput;
	char m_aIntBuff[32];
	int m_Value;

	CUIIntegerInput()
	{
		m_aIntBuff[0] = 0;
		m_Value = 0;
	}
};

struct CUIMouseDrag
{
	vec2 m_StartDragPos;
	vec2 m_EndDragPos;
	bool m_IsDragging;

	CUIMouseDrag()
	{
		m_IsDragging = false;
	}
};

struct CUICheckboxYesNo
{
	CUIButton m_YesBut;
	CUIButton m_NoBut;
};

struct CUIGrabHandle: CUIMouseDrag
{
	bool m_IsGrabbed;
};

class CEditor2: public IEditor
{
	enum
	{
		MAX_HISTORY=1024,
	};

	IGraphics* m_pGraphics;
	IInput *m_pInput;
	IClient *m_pClient;
	IConsole *m_pConsole;
	ITextRender *m_pTextRender;
	IStorage *m_pStorage;
	IConfigManager *m_pConfigManager;
	CConfig *m_pConfig;
	CRenderTools m_RenderTools;
	CUI m_UI;

	vec2 m_RenderGrenadePickupSize;
	vec2 m_RenderShotgunPickupSize;
	vec2 m_RenderLaserPickupSize;

	vec2 m_MousePos;
	vec2 m_UiMousePos;
	vec2 m_UiMouseDelta;
	vec2 m_MapUiPosOffset;
	float m_Zoom;

	IGraphics::CTextureHandle m_CheckerTexture;
	IGraphics::CTextureHandle m_CursorTexture;
	IGraphics::CTextureHandle m_EntitiesTexture;
	IGraphics::CTextureHandle m_GameTexture;

	CEditorMap2 m_Map;
	CEditorConsoleUI m_InputConsole;

	bool m_ConfigShowGrid;
	bool m_ConfigShowGridMajor;
	bool m_ConfigShowGameEntities;
	bool m_ConfigShowExtendedTilemaps;

	float m_GfxScreenWidth;
	float m_GfxScreenHeight;
	float m_ZoomWorldViewWidth;
	float m_ZoomWorldViewHeight;
	float m_LocalTime;

	CUIRect m_UiScreenRect;
	CUIRect m_UiMainViewRect;
	array2<u8> m_UiGroupOpen;
	array2<u8> m_UiGroupHidden;
	array2<u8> m_UiGroupHovered;
	array2<u8> m_UiLayerHovered;
	array2<u8> m_UiLayerHidden;
	int m_UiSelectedLayerID;
	int m_UiSelectedGroupID;
	int m_UiSelectedImageID;

	enum
	{
		POPUP_NONE = -1,
		POPUP_BRUSH_PALETTE = 0,
		POPUP_MENU_FILE,
	};

	int m_UiCurrentPopupID;
	CUIRect m_UiCurrentPopupRect;

	struct CUIBrushPaletteState
	{
		u8 m_aTileSelected[256];
		int m_ImageID;

		CUIBrushPaletteState()
		{
			memset(m_aTileSelected, 0, sizeof(m_aTileSelected));
			m_ImageID = -1;
		}
	};
	CUIBrushPaletteState m_UiBrushPaletteState;
	CUIRect m_UiPopupBrushPaletteRect;
	CUIRect m_UiPopupBrushPaletteImageRect;

	bool m_UiTextInputConsumeKeyboardEvents;
	bool m_UiDetailPanelIsOpen;
	bool m_WasMouseOnUiElement;

	struct CBrush
	{
		array2<CTile> m_aTiles;
		int m_Width;
		int m_Height;

		inline bool IsEmpty() const { return m_Width <= 0;}

		CBrush()
		{
			m_Width = 0;
			m_Height = 0;
		}
	};

	CBrush m_Brush;
	int m_BrushAutomapRuleID;

	struct CUISnapshot
	{
		int m_SelectedLayerID;
		int m_SelectedGroupID;
		int m_ToolID;
	};

	struct CHistoryEntry
	{
		CHistoryEntry* m_pPrev;
		CHistoryEntry* m_pNext;
		CEditorMap2::CSnapshot* m_pSnap;
		CUISnapshot* m_pUiSnap;
		char m_aActionStr[64];
		char m_aDescStr[64];

		inline void SetAction(const char* pStr)
		{
			const int Len = min((int)(sizeof(m_aActionStr)-1), str_length(pStr));
			memmove(m_aActionStr, pStr, Len);
			m_aActionStr[Len] = 0;
		}

		inline void SetDescription(const char* pStr)
		{
			const int Len = min((int)(sizeof(m_aDescStr)-1), str_length(pStr));
			memmove(m_aDescStr, pStr, Len);
			m_aDescStr[Len] = 0;
		}
	};

	CHistoryEntry m_aHistoryEntries[MAX_HISTORY];
	u8 m_aHistoryEntryUsed[MAX_HISTORY];
	CHistoryEntry* m_pHistoryEntryCurrent;

	enum
	{
		PAGE_MAP_EDITOR=0,
		PAGE_ASSET_MANAGER,
		PAGE_COUNT_,

		TOOL_SELECT=0,
		TOOL_DIMENSION,
		TOOL_TILE_BRUSH,
		TOOL_COUNT_
	};

	int m_Page;
	int m_Tool;

	// TODO: here functions are member functions, but not for CBrush. Choose one or the other
	struct CTileSelection
	{
		int m_StartTX;
		int m_StartTY;
		int m_EndTX;
		int m_EndTY;

		CTileSelection()
		{
			m_StartTX = -1;
		}

		inline void Select(int StartTX, int StartTY, int EndTX, int EndTY)
		{
			m_StartTX = min(StartTX, EndTX);
			m_StartTY = min(StartTY, EndTY);
			m_EndTX = max(StartTX, EndTX);
			m_EndTY = max(StartTY, EndTY);
		}

		inline void Deselect() { m_StartTX = -1; }

		inline bool IsValid() const
		{
			return m_StartTX >= 0 && m_StartTY >= 0 && m_EndTX >= 0 && m_EndTY >= 0;
		}

		inline bool IsSelected() const
		{
			return IsValid() && m_EndTX - m_StartTX >= 0 && m_EndTY - m_StartTY >= 0;
		}

		void FitLayer(const CEditorMap2::CLayer& TileLayer);
	};

	CTileSelection m_TileSelection;

	void RenderLayerGameEntities(const CEditorMap2::CLayer& GameLayer);

	vec2 CalcGroupScreenOffset(float WorldWidth, float WorldHeight, float PosX, float PosY, float ParallaxX,
		float ParallaxY);
	vec2 CalcGroupWorldPosFromUiPos(int GroupID, float WorldWidth, float WorldHeight, vec2 UiPos);
	CUIRect CalcUiRectFromGroupWorldRect(int GroupID, float WorldWidth, float WorldHeight, CUIRect WorldRect);

	static void StaticEnvelopeEval(float TimeOffset, int EnvID, float *pChannels, void *pUser);
	void EnvelopeEval(float TimeOffset, int EnvID, float *pChannels);

	void RenderMap();
	void RenderMapOverlay();
	void RenderMapEditorUI();
	void RenderTopPanel(CUIRect TopPanel);
	void RenderMapEditorUiLayerGroups(CUIRect NavRect);
	void RenderMapEditorUiDetailPanel(CUIRect DetailRect);
	void RenderPopupMenuFile();
	void RenderPopupBrushPalette();
	void RenderBrush(vec2 Pos);

	void RenderAssetManager();

	void DrawRect(const CUIRect& Rect, const vec4& Color);
	void DrawRectBorder(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawRectBorderOutside(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawRectBorderMiddle(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawText(const CUIRect& Rect, const char* pText, float FontSize, vec4 Color = vec4(1,1,1,1));

	void UiDoButtonBehavior(const void* pID, const CUIRect& Rect, CUIButton* pButState);
	bool UiDoMouseDragging(const void* pID, const CUIRect& Rect, CUIMouseDrag* pDragState);

	bool UiButton(const CUIRect& Rect, const char* pText, CUIButton* pButState, float FontSize = 10);
	bool UiButtonEx(const CUIRect& Rect, const char* pText, CUIButton* pButState,
					vec4 ColNormal, vec4 ColHover, vec4 ColPress, vec4 ColBorder, float FontSize);
	bool UiTextInput(const CUIRect& Rect, char* pText, int TextMaxLength, CUITextInput* pInputState);
	bool UiIntegerInput(const CUIRect& Rect, int* pInteger, CUIIntegerInput* pInputState);
	bool UiSliderInt(const CUIRect& Rect, int* pInteger, int Min, int Max, CUIButton* pInputState);
	bool UiSliderFloat(const CUIRect& Rect, float* pVal, float Min, float Max, CUIButton* pInputState,
		const vec4* pColor = NULL);
	bool UiCheckboxYesNo(const CUIRect& Rect, bool* pVal, CUICheckboxYesNo* pCbyn);
	bool UiButtonSelect(const CUIRect& Rect, const char* pText, CUIButton* pButState, bool Selected,
		float FontSize = 10);
	bool UiGrabHandle(const CUIRect& Rect, CUIGrabHandle* pGrabHandle, const vec4& ColorNormal, const vec4& ColorActive); // Returns pGrabHandle->m_IsDragging

	struct CScrollRegionParams
	{
		float m_ScrollbarWidth;
		float m_ScrollbarMargin;
		float m_SliderMinHeight;
		float m_ScrollSpeed;
		vec4 m_ClipBgColor;
		vec4 m_ScrollbarBgColor;
		vec4 m_RailBgColor;
		vec4 m_SliderColor;
		vec4 m_SliderColorHover;
		vec4 m_SliderColorGrabbed;
		int m_Flags;

		enum {
			FLAG_CONTENT_STATIC_WIDTH = 0x1
		};

		CScrollRegionParams()
		{
			m_ScrollbarWidth = 8;
			m_ScrollbarMargin = 1;
			m_SliderMinHeight = 25;
			m_ScrollSpeed = 5;
			m_ClipBgColor = vec4(0.0f, 0.0f, 0.0f, 0.5f);
			m_ScrollbarBgColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			m_RailBgColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
			m_SliderColor = vec4(0.2f, 0.1f, 0.98f, 1.0f);
			m_SliderColorHover = vec4(0.4f, 0.41f, 1.0f, 1.0f);
			m_SliderColorGrabbed = vec4(0.2f, 0.1f, 0.98f, 1.0f);
			m_Flags = 0;
		}
	};

	struct CScrollRegion
	{
		float m_ScrollY;
		float m_ContentH;
		float m_RequestScrollY; // [0, ContentHeight]
		CUIRect m_ClipRect;
		CUIRect m_OldClipRect;
		CUIRect m_RailRect;
		CUIRect m_LastAddedRect; // saved for ScrollHere()
		vec2 m_MouseGrabStart;
		vec2 m_ContentScrollOff;
		bool m_WasClipped;
		CScrollRegionParams m_Params;

		enum {
			SCROLLHERE_KEEP_IN_VIEW=0,
			SCROLLHERE_TOP,
			SCROLLHERE_BOTTOM,
		};

		CScrollRegion()
		{
			m_ScrollY = 0;
			m_ContentH = 0;
			m_RequestScrollY = -1;
			m_ContentScrollOff = vec2(0,0);
			m_WasClipped = false;
			m_Params = CScrollRegionParams();
		}
	};

	void UiBeginScrollRegion(CScrollRegion* pSr, CUIRect* pClipRect, vec2* pOutOffset, const CScrollRegionParams* pParams = 0);
	void UiEndScrollRegion(CScrollRegion* pSr);
	void UiScrollRegionAddRect(CScrollRegion* pSr, CUIRect Rect);
	void UiScrollRegionScrollHere(CScrollRegion* pSr, int Option = CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
	bool UiScrollRegionIsRectClipped(CScrollRegion* pSr, const CUIRect& Rect);

	inline bool IsPopupBrushPalette() const { return m_UiCurrentPopupID == POPUP_BRUSH_PALETTE; }
	inline bool IsPopupMenuFile() const { return m_UiCurrentPopupID == POPUP_MENU_FILE; }

	void Reset();
	void ResetCamera();
	void ChangeZoom(float Zoom);
	void ChangePage(int Page);
	void ChangeTool(int Tool);
	void SelectLayerBelowCurrentOne();

	void SetNewBrush(CTile* aTiles, int Width, int Height);
	void BrushClear();
	void BrushFlipX();
	void BrushFlipY();
	void BrushRotate90Clockwise();
	void BrushRotate90CounterClockwise();
	void BrushPaintLayer(int PaintTX, int PaintTY, int LayerID);
	void BrushPaintLayerFillRectRepeat(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID);
	void TileLayerRegionToBrush(int LayerID, int StartTX, int StartTY, int EndTX, int EndTY);

	void CenterViewOnQuad(const CQuad& Quad);

	inline bool IsToolSelect() const { return m_Tool == TOOL_SELECT; }
	inline bool IsToolDimension() const { return m_Tool == TOOL_DIMENSION; }
	inline bool IsToolBrush() const { return m_Tool == TOOL_TILE_BRUSH; }

	int Save(const char* pFilename);
	bool LoadMap(const char *pFileName);
	bool SaveMap(const char *pFileName);
	void OnMapLoaded();

	void EditDeleteLayer(int LyID, int ParentGroupID);
	void EditDeleteGroup(u32 GroupID);
	void EditDeleteImage(int ImageID);
	void EditAddImage(const char* pFilename);
	void EditCreateAndAddGroup();
	int EditCreateAndAddTileLayerUnder(int UnderLyID, int GroupID);
	int EditCreateAndAddQuadLayerUnder(int UnderLyID, int GroupID);
	void EditLayerChangeImage(int LayerID, int NewImageID);
	void EditLayerHighDetail(int LayerID, bool NewHighDetail);
	void EditGroupUseClipping(int GroupID, bool NewUseClipping);
	int EditGroupOrderMove(int GroupListIndex, int RelativePos);
	int EditLayerOrderMove(int ParentGroupListIndex, int LayerListIndex, int RelativePos);
	void EditTileSelectionFlipX(int LayerID);
	void EditTileSelectionFlipY(int LayerID);
	void EditBrushPaintLayer(int PaintTX, int PaintTY, int LayerID);
	void EditBrushPaintLayerFillRectRepeat(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID);
	void EditBrushPaintLayerAutomap(int PaintTX, int PaintTY, int LayerID, int RulesetID);
	void EditBrushPaintLayerFillRectAutomap(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID, int RulesetID);
	void EditTileLayerResize(int LayerID, int NewWidth, int NewHeight);
	void EditTileLayerAutoMapWhole(int LayerID, int RulesetID);
	void EditTileLayerAutoMapSection(int LayerID, int RulesetID, int StartTx, int StartTy, int SectionWidth, int SectionHeight);

	void EditHistCondLayerChangeName(int LayerID, const char* pNewName, bool HistoryCondition);
	void EditHistCondLayerChangeColor(int LayerID, vec4 NewColor, bool HistoryCondition);
	void EditHistCondGroupChangeName(int GroupID, const char* pNewName, bool HistoryCondition);
	void EditHistCondGroupChangeParallax(int GroupID, int NewParallaxX, int NewParallaxY, bool HistoryCondition);
	void EditHistCondGroupChangeOffset(int GroupID, int NewOffsetX, int NewOffsetY, bool HistoryCondition);
	void EditHistCondGroupChangeClipX(int GroupID, int NewClipX, bool HistoryCondition);
	void EditHistCondGroupChangeClipY(int GroupID, int NewClipY, bool HistoryCondition);
	void EditHistCondGroupChangeClipRight(int GroupID, int NewClipRight, bool HistoryCondition);
	void EditHistCondGroupChangeClipBottom(int GroupID, int NewClipBottom, bool HistoryCondition);

	void HistoryClear();
	CHistoryEntry* HistoryAllocEntry();
	void HistoryDeallocEntry(CHistoryEntry* pEntry);
	void HistoryNewEntry(const char* pActionStr, const char* pDescStr);
	void HistoryRestoreToEntry(CHistoryEntry* pEntry);
	void HistoryUndo();
	void HistoryRedo();

	CUISnapshot* SaveUiSnapshot();
	void RestoreUiSnapshot(CUISnapshot* pUiSnap);

	const char* GetLayerName(int LayerID);
	const char* GetGroupName(int GroupID);

	static void ConLoad(IConsole::IResult *pResult, void *pUserData);
	static void ConSave(IConsole::IResult *pResult, void *pUserData);
	static void ConShowPalette(IConsole::IResult *pResult, void *pUserData);
	static void ConGameView(IConsole::IResult *pResult, void *pUserData);
	static void ConShowGrid(IConsole::IResult *pResult, void *pUserData);
	static void ConUndo(IConsole::IResult *pResult, void *pUserData);
	static void ConRedo(IConsole::IResult *pResult, void *pUserData);
	static void ConDeleteImage(IConsole::IResult *pResult, void *pUserData);

	inline IGraphics* Graphics() { return m_pGraphics; };
	inline IInput *Input() { return m_pInput; };
	inline IClient *Client() { return m_pClient; };
	inline IConsole *Console() { return m_pConsole; };
	inline ITextRender *TextRender() { return m_pTextRender; };
	inline IStorage *Storage() { return m_pStorage; };
	inline CUI *UI() { return &m_UI; }
	inline CRenderTools *RenderTools() { return &m_RenderTools; }
	inline CConfig *Config() { return m_pConfig; }

public:

	CEditor2();
	~CEditor2();

	void Init();
	void UpdateAndRender();
	void Update();
	void Render();
	bool HasUnsavedData() const;
	void OnInput(IInput::CEvent Event);
};

#endif
