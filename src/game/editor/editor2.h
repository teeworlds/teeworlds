// LordSk
#ifndef GAME_EDITOR_EDITOR2_H
#define GAME_EDITOR_EDITOR2_H

#include <stdint.h>
#include <string.h> // memset
#include <base/system.h>
#include <base/tl/array.h>

#include <engine/editor.h>
#include <engine/config.h>
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

	void clear()
	{
		// don't free buffer on clear
		this->num_elements = 0;
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

// Sparse array
// - Provides indirection between ID and data pointer so IDs are not invalidated
// - Data is packed
template<typename T, u32 MAX_ELEMENTS>
struct CSparseArray
{
	enum
	{
		MAX_ELEMENTS = MAX_ELEMENTS,
		INVALID_ID = 0xFFFF
	};

	u16 m_ID[MAX_ELEMENTS];
	u16 m_ReverseID[MAX_ELEMENTS];
	T m_Data[MAX_ELEMENTS];
	int count;

	CSparseArray()
	{
		Clear();
	}

	u32 Push(const T& elt)
	{
		dbg_assert(count < MAX_ELEMENTS, "Array is full");

		for(int i = 0; i < MAX_ELEMENTS; i++)
		{
			if(m_ID[i] == INVALID_ID)
			{
				const u16 dataID = count++;
				m_ID[i] = dataID;
				m_ReverseID[dataID] = i;
				m_Data[dataID] = elt;
				return i;
			}
		}

		dbg_assert(0, "Failed to push element");
		return -1;
	}

	T& Set(u32 SlotID, const T& elt)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] == INVALID_ID) {
			dbg_assert(count < MAX_ELEMENTS, "Array is full"); // should never happen
			const u16 dataID = count++;
			m_ID[SlotID] = dataID;
			m_ReverseID[SlotID] = SlotID;
		}
		return m_Data[m_ID[SlotID]] = elt;
	}

	T& Get(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");
		dbg_assert(m_ID[SlotID] != INVALID_ID, "Slot is empty");
		return m_Data[m_ID[SlotID]];
	}

	bool IsValid(u32 SlotID) const
	{
		if(SlotID > MAX_ELEMENTS) return false;
		if(m_ID[SlotID] == INVALID_ID) return false;
#ifdef CONF_DEBUG
		const u16 dataID = m_ID[SlotID];
		return m_ReverseID[dataID] == SlotID;
#endif
		return true;
	}

	u32 GetIDFromData(const T& elt) const
	{
		int64 dataID = &elt - m_Data;
		dbg_assert(dataID >= 0 && dataID < MAX_ELEMENTS, "Element out of bounds");
		return m_ReverseID[dataID];
	}

	void RemoveByID(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] != INVALID_ID) {
			dbg_assert(count > 0, "Array is empty"); // should never happen
			const u16 dataID = m_ID[SlotID];

			const int lastDataID = count-1;
			const u16 lastID = m_ReverseID[lastDataID];
			m_ID[lastID] = dataID;
			tl_swap(m_ReverseID[dataID], m_ReverseID[lastDataID]);
			tl_swap(m_Data[dataID], m_Data[lastDataID]);
			count--;

			m_ID[SlotID] = INVALID_ID;
		}
	}

	void RemoveKeepOrderByID(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] != INVALID_ID) {
			dbg_assert(count > 0, "Array is empty"); // should never happen


			dbg_assert(0, "Implement me");
		}
	}

	void SwapTwoDataElements(const T& elt1, const T& elt2)
	{
		int64 dataID1 = &elt1 - m_Data;
		dbg_assert(dataID1 >= 0 && dataID1 < MAX_ELEMENTS, "Element out of bounds");
		int64 dataID2 = &elt2 - m_Data;
		dbg_assert(dataID2 >= 0 && dataID2 < MAX_ELEMENTS, "Element out of bounds");

		tl_swap(m_ReverseID[dataID1], m_ReverseID[dataID2]);
		tl_swap(m_Data[dataID1], m_Data[dataID2]);
	}

	void Clear()
	{
		memset(m_ID, 0xFF, sizeof(m_ID));
		count = 0;
	}

	int Count() const
	{
		return count;
	}

	/*
	inline T& operator [] (u32 ID) const
	{
		return Get(ID);
	}
	*/

	T* Data()
	{
		return m_Data;
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

	// TODO: split this into CTileLayer and CQuadLayer (make one array of each to allocate them)
	struct CLayer
	{
		char m_aName[12];
		int m_Type;
		int m_ImageID;
		bool m_HighDetail; // TODO: turn into m_Flags ?
		vec4 m_Color;

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

		CLayer()
		{
			m_Type = 0;
			m_ImageID = 0;
		}
	};

	struct CGroup
	{
		char m_aName[12];
		int m_apLayerIDs[MAX_GROUP_LAYERS];
		int m_LayerCount;
		int m_ParallaxX;
		int m_ParallaxY;
		int m_OffsetX;
		int m_OffsetY;
		int m_ClipX;
		int m_ClipY;
		int m_ClipWidth;
		int m_ClipHeight;
		bool m_UseClipping ;

		CGroup()
		{
			m_aName[0] = 0;
			m_LayerCount = 0;
			m_ParallaxX = 0;
			m_ParallaxY = 0;
			m_OffsetX = 0;
			m_OffsetY = 0;
			m_ClipX = 0;
			m_ClipY = 0;
			m_ClipWidth = 0;
			m_ClipHeight = 0;
			m_UseClipping = false;
		}
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
		int m_ImageCount;

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

	int m_MapMaxWidth;
	int m_MapMaxHeight;
	int m_GameLayerID;
	int m_GameGroupID;

	char m_aPath[256];

	array2<CEnvPoint> m_aEnvPoints;
	CSparseArray<CLayer,1024> m_aLayers;
	CSparseArray<CGroup,256> m_aGroups;
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

	CLayer& NewTileLayer(int Width, int Height, u32* pOutID = 0x0);
	CLayer& NewQuadLayer(u32* pOutID = 0x0);
};

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
	array<u8> m_UiGroupOpen;
	array<u8> m_UiGroupHidden;
	array<u8> m_UiGroupHovered;
	array<u8> m_UiLayerHovered;
	array<u8> m_UiLayerHidden;
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
