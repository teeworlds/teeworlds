// LordSk
#ifndef GAME_EDITOR_EDITOR2_H
#define GAME_EDITOR_EDITOR2_H

#include <stdint.h>
#include <string.h> // memset
#include <base/system.h>
#include <base/tl/array.h>

#include <engine/editor.h>
#include <engine/shared/datafile.h>

#include <game/mapitems.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <generated/client_data.h>
#include <game/client/components/mapimages.h>

#include "ed_console.h"
#include "auto_map2.h"

class IStorage;
class IGraphics;
class IInput;
class IClient;
class IConsole;
class ITextRender;
class IStorage;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

template<typename T>
struct array2: public array<T>
{
	typedef array<T> ParentT;

	inline T& add(const T& Elt)
	{
		return this->base_ptr()[ParentT::add(Elt)];
	}

	T& add(const T* aElements, int EltCount)
	{
		for(int i = 0; i < EltCount; i++)
			ParentT::add(aElements[i]);
		return *(this->base_ptr()+this->size()-EltCount);
	}

	T& add_empty(int EltCount)
	{
		dbg_assert(EltCount > 0, "Add 0 or more");
		for(int i = 0; i < EltCount; i++)
			ParentT::add(T());
		return *(this->base_ptr()+this->size()-EltCount);
	}

	inline void remove_index_fast(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index_fast(Index);
	}

	// keeps order, way slower
	inline void remove_index(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index(Index);
	}

	inline T& operator[] (int Index)
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}

	inline const T& operator[] (int Index) const
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}
};

struct CEditorMap2
{
	enum
	{
		MAX_IMAGES=128,
		MAX_GROUP_LAYERS=64,
		MAX_IMAGE_NAME_LEN=64,
		MAX_EMBEDDED_FILES=64,
	};

	struct CLayer
	{
		char m_aName[12];
		int m_Type = 0;
		int m_ImageID = 0;
		bool m_HighDetail; // TODO: turn into m_Flags ?
		vec4 m_Color;

		// NOTE: we have to split the union because gcc doesn't like non-POD anonymous structs...
		// TODO: enable c++11, please?

		//union
		//{
			array2<CTile> m_aTiles;
			array2<CQuad> m_aQuads;
		//};

		union
		{
			// tile
			struct {
				int m_Width;
				int m_Height;
				int m_ColorEnvelopeID;
				int m_ColorEnvOffset;
			};

			// quad
			struct {

			};
		};

		inline bool IsTileLayer() const { return m_Type == LAYERTYPE_TILES; }
		inline bool IsQuadLayer() const { return m_Type == LAYERTYPE_QUADS; }
	};

	struct CGroup
	{
		char m_aName[12] = {0};
		int m_apLayerIDs[MAX_GROUP_LAYERS];
		int m_LayerCount = 0;
		int m_ParallaxX = 0;
		int m_ParallaxY = 0;
		int m_OffsetX = 0;
		int m_OffsetY = 0;
		int m_ClipX = 0;
		int m_ClipY = 0;
		int m_ClipWidth = 0;
		int m_ClipHeight = 0;
		bool m_UseClipping = false;
	};

	struct CEnvelope
	{
		int m_Version;
		int m_Channels;
		CEnvPoint* m_aPoints;
		int m_NumPoints;
		//int m_aName[8];
		bool m_Synchronized;
	};

	struct CImageName
	{
		char m_Buff[MAX_IMAGE_NAME_LEN];
	};

	struct CEmbeddedFile
	{
		u32 m_Crc;
		int m_Type; // unused for now (only images)
		void* m_pData;
	};

	struct CAssets
	{
		// TODO: pack some of these together
		CImageName m_aImageNames[MAX_IMAGES];
		u32 m_aImageNameHash[MAX_IMAGES];
		u32 m_aImageEmbeddedCrc[MAX_IMAGES];
		IGraphics::CTextureHandle m_aTextureHandle[MAX_IMAGES];
		CImageInfo m_aTextureInfos[MAX_IMAGES];
		int m_ImageCount = 0;

		CEmbeddedFile m_aEmbeddedFile[MAX_EMBEDDED_FILES];
		int m_EmbeddedFileCount;

		array<u32> m_aAutomapTileHashID;
		array<CTilesetMapper2> m_aAutomapTile;
	};

	// used for undo/redo
	struct CSnapshot
	{
		int m_GroupCount;
		int m_LayerCount;
		int m_EnvelopeCount;
		int m_ImageCount;
		int m_GameLayerID;
		int m_GameGroupID;
		CImageName* m_aImageNames;
		u32* m_aImageNameHash;
		u32* m_aImageEmbeddedCrc;
		CImageInfo* m_aImageInfos;
		CGroup* m_aGroups;
		CMapItemLayer** m_apLayers;
		CMapItemEnvelope* m_aEnvelopes;
		CTile* m_aTiles;
		CQuad* m_aQuads;
		CEnvPoint* m_aEnvPoints;
		u8 m_Data[1];
	};

	int m_MapMaxWidth = 0;
	int m_MapMaxHeight = 0;
	int m_GameLayerID = -1;
	int m_GameGroupID = -1;

	char m_aPath[256];

	array2<CEnvPoint> m_aEnvPoints;
	array2<CLayer> m_aLayers;
	array2<CGroup> m_aGroups;
	array2<CMapItemEnvelope> m_aEnvelopes;

	CAssets m_Assets;

	IGraphics* m_pGraphics;
	IConsole *m_pConsole;
	IStorage *m_pStorage;

	inline IGraphics* Graphics() { return m_pGraphics; };
	inline IConsole *Console() { return m_pConsole; };
	inline IStorage *Storage() { return m_pStorage; };

	void Init(IStorage *pStorage, IGraphics* pGraphics, IConsole* pConsole);
	bool Save(const char *pFileName);
	bool Load(const char *pFileName);
	void LoadDefault();
	void Clear();

	// loads not-loaded images, clears the rest
	void AssetsClearAndSetImages(CImageName* aName, CImageInfo* aInfo, u32* aImageEmbeddedCrc, int ImageCount);
	u32 AssetsAddEmbeddedData(void* pData, u64 DataSize);
	void AssetsClearEmbeddedFiles();
	bool AssetsAddAndLoadImage(const char* pFilename); // also loads automap rules if there are any
	void AssetsDeleteImage(int ImgID);
	void AssetsLoadAutomapFileForImage(int ImgID);
	void AssetsLoadMissingAutomapFiles();
	CTilesetMapper2* AssetsFindTilesetMapper(int ImgID);

	CSnapshot* SaveSnapshot();
	void RestoreSnapshot(const CSnapshot* pSnapshot);
#ifdef CONF_DEBUG
	void CompareSnapshot(const CSnapshot* pSnapshot);
#endif

	CLayer& NewTileLayer(int Width, int Height);
	CLayer& NewQuadLayer();
};

struct CUIButton
{
	u8 m_Hovered = false;
	u8 m_Pressed = false;
	u8 m_Clicked = false;
};

struct CUITextInput
{
	CUIButton m_Button;
	u8 m_Selected = false;
	CLineInput m_LineInput;
	int m_CursorPos;
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
	bool m_IsDragging = false;
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
	CRenderTools m_RenderTools;
	CUI m_UI;

	vec2 m_RenderGrenadePickupSize;
	vec2 m_RenderShotgunPickupSize;
	vec2 m_RenderLaserPickupSize;

	vec2 m_MousePos;
	vec2 m_UiMousePos;
	vec2 m_UiMouseDelta;
	vec2 m_MapUiPosOffset;
	float m_Zoom = 1.0f;

	IGraphics::CTextureHandle m_CheckerTexture;
	IGraphics::CTextureHandle m_CursorTexture;
	IGraphics::CTextureHandle m_EntitiesTexture;
	IGraphics::CTextureHandle m_GameTexture;

	CEditorMap2 m_Map;
	CEditorConsoleUI m_InputConsole;

	bool m_ConfigShowGrid = true;
	bool m_ConfigShowGridMajor = false;
	bool m_ConfigShowGameEntities = false;
	bool m_ConfigShowExtendedTilemaps = false;

	float m_GfxScreenWidth;
	float m_GfxScreenHeight;
	float m_ZoomWorldViewWidth;
	float m_ZoomWorldViewHeight;
	float m_LocalTime;

	CUIRect m_UiScreenRect;
	CUIRect m_UiMainViewRect;
	array<u8> m_UiGroupOpen;
	array<u8> m_UiGroupHidden;
	array<u8> m_UiGroupHovered;
	array<u8> m_UiLayerHovered;
	array<u8> m_UiLayerHidden;
	int m_UiSelectedLayerID = -1;
	int m_UiSelectedGroupID = -1;
	int m_UiSelectedImageID = -1;

	enum
	{
		POPUP_NONE = -1,
		POPUP_BRUSH_PALETTE = 0
	};

	int m_UiCurrentPopupID = POPUP_NONE;

	struct CUIBrushPaletteState
	{
		u8 m_aTileSelected[256] = {0};
		int m_ImageID = -1;
	};
	CUIBrushPaletteState m_UiBrushPaletteState;
	CUIRect m_UiPopupBrushPaletteRect = {};
	CUIRect m_UiPopupBrushPaletteImageRect = {};

	bool m_UiTextInputConsumeKeyboardEvents = false; // TODO: remork/remove
	bool m_UiDetailPanelIsOpen = false;
	bool m_WasMouseOnUiElement = false;

	struct CBrush
	{
		array2<CTile> m_aTiles;
		int m_Width = 0;
		int m_Height = 0;

		inline bool IsEmpty() const { return m_Width <= 0;}
	};

	CBrush m_Brush;
	int m_BrushAutomapRuleID = -1;

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
	CHistoryEntry* m_pHistoryEntryCurrent = 0x0;

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

	int m_Page = PAGE_MAP_EDITOR;
	int m_Tool = TOOL_SELECT;

	// TODO: here functions are member functions, but not for CBrush. Choose one or the other
	struct CTileSelection
	{
		int m_StartTX = -1;
		int m_StartTY;
		int m_EndTX;
		int m_EndTY;

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
	void RenderMapEditorUiLayerGroups(CUIRect NavRect);
	void RenderMapEditorUiDetailPanel(CUIRect DetailRect);
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

	inline bool IsToolSelect() const { return m_Tool == TOOL_SELECT; }
	inline bool IsToolDimension() const { return m_Tool == TOOL_DIMENSION; }
	inline bool IsToolBrush() const { return m_Tool == TOOL_TILE_BRUSH; }

	int Save(const char* pFilename);
	bool LoadMap(const char *pFileName);
	bool SaveMap(const char *pFileName);
	void OnMapLoaded();

	void EditDeleteLayer(int LyID, int ParentGroupID);
	void EditDeleteGroup(int GroupID);
	void EditDeleteImage(int ImageID);
	void EditAddImage(const char* pFilename);
	void EditCreateAndAddGroup();
	int EditCreateAndAddTileLayerUnder(int UnderLyID, int GroupID);
	int EditCreateAndAddQuadLayerUnder(int UnderLyID, int GroupID);
	void EditLayerChangeImage(int LayerID, int NewImageID);
	void EditLayerHighDetail(int LayerID, bool NewHighDetail);
	void EditGroupUseClipping(int GroupID, bool NewUseClipping);
	int EditGroupOrderMove(int GroupID, int RelativePos);
	int EditLayerOrderMove(int LayerID, int RelativePos);
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
