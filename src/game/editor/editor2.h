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

#include "input_console.h"


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

/* Chain Allocator
 *
 * - Allocate/Deallocate a continous list of T elements
 * - Pointers are never invalidated
 *
 * Technicality:
 * - Slightly faster than malloc with RING_ELT_COUNT==1, faster otherwise depending on RING_ELT_COUNT
 * - A Ring can be X element long
 * - Allocation will be rounded out to match RING_ELT_COUNT
 * - A Chain is made of rings, hence the name
 * - Returned memory is zeroed out
 * - Falls back on malloc when out of space
 *
 * - Usage:
 *	CChainAllocator<T, MaxElementCount, RingElementCount> ChainAllocator;
 *	ChainAllocator.Init();
 *	CMemBlock<T> Block = ChainAllocator.Alloc(ElementCount);
 *	T* pData = Block.Get();
 *	ChainAllocator.Dealloc(&Block);
*/

template<typename T>
struct CMemBlock
{
	T* m_pStart;
	int m_Count;

	inline T* Get() { return m_pStart; }
};

template<typename T>
class CChainAllocator
{
	int64 m_AllocatedSize;
	u8* m_aRingUsed;
	T* m_pElementBuffer;
	u32 ELT_COUNT_MAX;
	u32 RING_ELT_COUNT;

public:
	typedef CMemBlock<T> BlockT;

	CChainAllocator()
	{
		m_AllocatedSize = 0;
		m_aRingUsed = 0;
		m_pElementBuffer = 0;
		ELT_COUNT_MAX = 0;
		RING_ELT_COUNT = 1;
	}

	~CChainAllocator()
	{
		Deinit();
	}

	void Init(u32 ElementCountMax, u32 RingElementCount = 1)
	{
		ELT_COUNT_MAX = ElementCountMax;
		RING_ELT_COUNT = RingElementCount;
		const int TOTAL_RING_COUNT = (ELT_COUNT_MAX/RING_ELT_COUNT);
		dbg_assert(m_pElementBuffer == 0, "Don't init this twice");
		m_AllocatedSize = sizeof(u8)*TOTAL_RING_COUNT+sizeof(T)*ELT_COUNT_MAX;
		m_aRingUsed = (u8*)mem_alloc(m_AllocatedSize, 0); // TODO: align
		m_pElementBuffer = (T*)(m_aRingUsed+ELT_COUNT_MAX);
		mem_zero(m_aRingUsed, sizeof(u8)*TOTAL_RING_COUNT);
	}

	void Clear()
	{
		const int TOTAL_RING_COUNT = (ELT_COUNT_MAX/RING_ELT_COUNT);
		mem_zero(m_aRingUsed, sizeof(u8)*TOTAL_RING_COUNT);
	}

	void Deinit()
	{
		mem_free(m_aRingUsed);
	}

	inline T* AllocOne()
	{
		return Alloc(1).Get();
	}

	BlockT Alloc(const int Count)
	{
		dbg_assert(m_pElementBuffer != 0, "Forgot to call Init()");
		int ChainRingStart = 0;
		int ChainRingCount = 0;
		const int TOTAL_RING_COUNT = (ELT_COUNT_MAX/RING_ELT_COUNT);
		for(int i = 0; i < TOTAL_RING_COUNT; i++)
		{
			uint64_t Ring8 = *(uint64_t*)(m_aRingUsed+i);
			if(Ring8 == 0)
			{
				ChainRingCount += 8;
				i += 7;
			}
			else if(Ring8 == (uint64_t)0xFFFFFFFFFFFFFFFF)
			{
				ChainRingStart = i+8;
				ChainRingCount = 0;
				i += 7;
				continue;
			}
			else
			{
				if(m_aRingUsed[i])
				{
					ChainRingStart = i+1;
					ChainRingCount = 0;
					continue;
				}
				else
					ChainRingCount++;
			}

			if(ChainRingCount*RING_ELT_COUNT >= Count)
			{
				memset(m_aRingUsed+ChainRingStart, 0xFF, sizeof(u8)*ChainRingCount);

				BlockT Block;
				Block.m_pStart = m_pElementBuffer + ChainRingStart*RING_ELT_COUNT;
				Block.m_Count = ChainRingCount*RING_ELT_COUNT;
				mem_zero(Block.m_pStart, sizeof(T)*Block.m_Count);
				dbg_assert(ChainRingStart*RING_ELT_COUNT + Block.m_Count <= ELT_COUNT_MAX,
						   "Something went wrong");
				return Block;
			}
		}

#ifdef CONF_DEBUG
		dbg_break();
#endif
		// fallback to malloc in release mode
		T* pFallBack = (T*)mem_alloc(sizeof(T)*Count, 0);
		BlockT Block;
		Block.m_pStart = pFallBack;
		Block.m_Count = Count;
		mem_zero(Block.m_pStart, sizeof(T)*Block.m_Count);
		return Block;
	}

	void Dealloc(BlockT* pBlock)
	{
		if(pBlock->m_Count <= 0)
			return;

		const int Start = pBlock->m_pStart-m_pElementBuffer;
		const int Count = pBlock->m_Count;

		if(Start >= 0 && Start+Count <= ELT_COUNT_MAX)
			mem_zero(m_aRingUsed+(Start/RING_ELT_COUNT), sizeof(u8)*(Count/RING_ELT_COUNT));
		else
			mem_free(pBlock->m_pStart);

		pBlock->m_Count = 0;
	}

	inline void DeallocOne(T* pPtr)
	{
		BlockT Block;
		Block.m_pStart = pPtr;
		Block.m_Count = 1;
		Dealloc(&Block);
	}

	inline int64 AllocatedSize() const { return m_AllocatedSize; }
};

template<typename T>
struct CDynArray
{
	CChainAllocator<T>* m_pAllocator;
	CMemBlock<T> m_MemBlock;
	int m_EltCount;

	CDynArray()
	{
		m_pAllocator = 0;
	}

	~CDynArray()
	{
		Clear();
	}

	void Init(CChainAllocator<T>* pAllocator)
	{
		dbg_assert(!m_pAllocator, "Init already called");
		m_pAllocator = pAllocator;
		m_MemBlock.m_pStart = 0;
		m_MemBlock.m_Count = 0;
		m_EltCount = 0;
	}

	void Reserve(int NewCapacity)
	{
		dbg_assert(m_pAllocator != 0, "Forgot to call Init");
		if(NewCapacity <= Capacity())
			return;
		CMemBlock<T> NewBlock = m_pAllocator->Alloc(NewCapacity);
		mem_copy(NewBlock.m_pStart, Data(), Capacity()*sizeof(T));
		m_pAllocator->Dealloc(&m_MemBlock);
		m_MemBlock = NewBlock;
	}

	inline T& Add(const T& Elt) { return Add(&Elt, 1); }

	T& Add(const T* aElements, int Count)
	{
		if(m_EltCount+Count >= Capacity())
			Reserve(max(Capacity() * 2, m_EltCount+Count));
		mem_copy(Data()+m_EltCount, aElements, Count*sizeof(T));
		m_EltCount += Count;
		return *(Data()-Count);
	}

	T& AddEmpty(int Count)
	{
		dbg_assert(Count > 0, "Add 0 or more");
		if(m_EltCount+Count >= Capacity())
			Reserve(max(Capacity() * 2, m_EltCount+Count));
		mem_zero(Data()+m_EltCount, Count*sizeof(T));
		m_EltCount += Count;
		return *(Data()-Count);
	}

	inline void Clear()
	{
		for(int i = 0; i < m_EltCount; i++)
			Data()[i].~T();
		m_pAllocator->Dealloc(&m_MemBlock);
		m_MemBlock.m_pStart = 0;
		m_MemBlock.m_Count = 0;
		m_EltCount = 0;
	}

	inline void RemoveByIndex(int Index)
	{
		dbg_assert(Index >= 0 && Index < m_EltCount, "Index out of bounds");
		Data()[Index].~T();
		Data()[Index] = Data()[m_EltCount-1];
		m_EltCount--;
	}

	inline int Count() const { return m_EltCount; }
	inline int Capacity() const { return m_MemBlock.m_Count; }
	inline T* Data() { return m_MemBlock.m_pStart; }
	inline const T* Data() const { return m_MemBlock.m_pStart; }

	inline T& operator[] (int Index)
	{
		dbg_assert(Index >= 0 && Index < m_EltCount, "Index out of bounds");
		return m_MemBlock.m_pStart[Index];
	}

	inline const T& operator[] (int Index) const
	{
		dbg_assert(Index >= 0 && Index < m_EltCount, "Index out of bounds");
		return m_MemBlock.m_pStart[Index];
	}
};

struct CEditorMap
{
	enum
	{
		MAX_TEXTURES=128,
		MAX_GROUP_LAYERS=64,
		MAX_IMAGE_NAME_LEN=64,
	};

	struct CLayer
	{
		int m_Type = 0;
		int m_ImageID = 0;
		vec4 m_Color;

		union
		{
			struct {
				CDynArray<CTile> m_aTiles;
				int m_Width;
				int m_Height;
				int m_ColorEnvelopeID;
			};

			struct {
				CDynArray<CQuad> m_aQuads;
			};
		};

		CLayer(){}
		~CLayer(){}
		inline bool IsTileLayer() const { return m_Type == LAYERTYPE_TILES; }
		inline bool IsQuadLayer() const { return m_Type == LAYERTYPE_QUADS; }
	};

	struct CGroup
	{
		int m_apLayerIDs[MAX_GROUP_LAYERS];
		int m_LayerCount = 0;
		int m_ParallaxX = 0;
		int m_ParallaxY = 0;
		int m_OffsetX = 0;
		int m_OffsetY = 0;
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

	struct CTextureInfo
	{
		int m_Flags;
		int m_Format;
		int m_Width;
		int m_Height;
		u8* m_Data;
	};

	struct CImageName
	{
		char m_Buff[MAX_IMAGE_NAME_LEN];
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
		CTextureInfo* m_aImageInfos;
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

	CDynArray<CEnvPoint> m_aEnvPoints;
	CDynArray<CLayer> m_aLayers;
	CDynArray<CGroup> m_aGroups;
	CDynArray<CMapItemEnvelope> m_aEnvelopes;

	CChainAllocator<CTile> m_TileDispenser;
	CChainAllocator<CQuad> m_QuadDispenser;
	CChainAllocator<CEnvPoint> m_EnvPointDispenser;
	CChainAllocator<CLayer> m_LayerDispenser;
	CChainAllocator<CGroup> m_GroupDispenser;
	CChainAllocator<CMapItemEnvelope> m_EnvelopeDispenser;

	CImageName m_aImageNames[MAX_TEXTURES];
	IGraphics::CTextureHandle m_aTextureHandle[MAX_TEXTURES];
	IGraphics::CTextureHandle m_aTexture2DHandle[MAX_TEXTURES];
	CTextureInfo m_aTextureInfos[MAX_TEXTURES];
	int m_TextureCount = 0;

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

	CSnapshot* SaveSnapshot();
	void RestoreSnapshot(const CSnapshot* pSnapshot);
#ifdef CONF_DEBUG
	void CompareSnapshot(const CSnapshot* pSnapshot);
#endif

	inline CDynArray<CTile> NewTileArray()
	{
		CDynArray<CTile> Array;
		Array.Init(&m_TileDispenser);
		return Array;
	}

	inline CDynArray<CQuad> NewQuadArray()
	{
		CDynArray<CQuad> Array;
		Array.Init(&m_QuadDispenser);
		return Array;
	}

	CLayer& NewTileLayer(int Width, int Height);
	CLayer& NewQuadLayer();
};

struct CUIButtonState
{
	u8 m_Hovered = false;
	u8 m_Pressed = false;
	u8 m_Clicked = false;
};

struct CHistoryEntry
{
	CHistoryEntry* m_pPrev;
	CHistoryEntry* m_pNext;
	CEditorMap::CSnapshot* m_pSnap;
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

class CEditor: public IEditor
{
	enum
	{
		MAX_GROUPS=64,
		MAX_LAYERS=128,
		MAX_HISTORY=128,
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

	CEditorMap m_Map;
	CEditorInputConsole m_InputConsole;

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
	u8 m_UiGroupOpen[MAX_GROUPS] = {0};
	u8 m_UiGroupHidden[MAX_GROUPS] = {0};
	u8 m_UiLayerHovered[MAX_LAYERS] = {0};
	u8 m_UiLayerHidden[MAX_LAYERS] = {0};
	int m_UiSelectedLayerID = -1;
	int m_UiSelectedGroupID = -1;

	enum
	{
		POPUP_NONE = -1,
		POPUP_BRUSH_PALETTE = 0
	};

	int m_UiCurrentPopupID = POPUP_BRUSH_PALETTE;

	struct CUIBrushPaletteState
	{
		vec2 m_MouseStartDragPos;
		vec2 m_MouseEndDragPos;
		bool m_MouseClicked = false;
		bool m_MouseIsDraggingRect = false;
		u8 m_aTileSelected[256] = {0};
	};
	CUIBrushPaletteState m_UiBrushPaletteState;
	CUIRect m_UiPopupBrushPaletteRect = {};
	CUIRect m_UiPopupBrushPaletteImageRect = {};

	vec2 m_UiMouseStartDragPos;
	vec2 m_UiMouseEndDragPos;
	bool m_UiMouseIsDragging = false;
	bool m_UiMouseLeftPressed = false;

	struct CBrush
	{
		CDynArray<CTile> m_aTiles;
		int m_Width = 0;
		int m_Height = 0;

		inline bool IsEmpty() const { return m_Width <= 0;}
	};

	CBrush m_Brush;
	ivec2 m_TileStartDrag;

	CChainAllocator<CHistoryEntry> m_HistoryEntryDispenser;
	CHistoryEntry* m_pHistoryEntryCurrent = nullptr;

	void RenderLayerGameEntities(const CEditorMap::CLayer& GameLayer);

	vec2 CalcGroupScreenOffset(float WorldWidth, float WorldHeight, float PosX, float PosY, float ParallaxX,
		float ParallaxY);
	vec2 CalcGroupWorldPosFromUiPos(int GroupID, float WorldWidth, float WorldHeight, vec2 UiPos);

	static void StaticEnvelopeEval(float TimeOffset, int EnvID, float *pChannels, void *pUser);
	void EnvelopeEval(float TimeOffset, int EnvID, float *pChannels);

	void RenderHud();
	void RenderUI();
	void RenderUiLayerGroups(CUIRect NavRect);
	void RenderPopupBrushPalette();
	void RenderBrush(vec2 Pos);

	void DrawRect(const CUIRect& Rect, const vec4& Color);
	void DrawRectBorder(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawRectBorderOutside(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawRectBorderMiddle(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawText(const CUIRect& Rect, const char* pText, float FontSize, vec4 Color = vec4(1,1,1,1));

	void UiDoButtonBehavior(const void* pID, const CUIRect& Rect, CUIButtonState* pButState);
	bool UiButton(const CUIRect& Rect, const char* pText, CUIButtonState* pButState);
	bool UiButtonEx(const CUIRect& Rect, const char* pText, CUIButtonState* pButState,
					vec4 ColNormal, vec4 ColHover, vec4 ColPress, vec4 ColBorder);
	inline bool IsPopupBrushPalette() const { return m_UiCurrentPopupID == POPUP_BRUSH_PALETTE; }

	void PopupBrushPaletteProcessInput(IInput::CEvent Event);

	void Reset();
	void ResetCamera();
	void ChangeZoom(float Zoom);

	void SetNewBrush(CTile* aTiles, int Width, int Height);
	void ClearBrush();
	void BrushFlipX();
	void BrushFlipY();
	void BrushRotate90Clockwise();
	void BrushRotate90CounterClockwise();

	int Save(const char* pFilename);
	bool LoadMap(const char *pFileName);
	void OnMapLoaded();

	void OnStartDragging();
	void OnFinishDragging();

	void EditDeleteLayer(int LyID, int ParentGroupID);

	void HistoryNewEntry(const char* pActionStr, const char* pDescStr);
	void HistoryRestoreToEntry(CHistoryEntry* pEntry);

	static void ConLoad(IConsole::IResult *pResult, void *pUserData);
	static void ConShowPalette(IConsole::IResult *pResult, void *pUserData);
	static void ConGameView(IConsole::IResult *pResult, void *pUserData);
	static void ConShowGrid(IConsole::IResult *pResult, void *pUserData);

	inline IGraphics* Graphics() { return m_pGraphics; };
	inline IInput *Input() { return m_pInput; };
	inline IClient *Client() { return m_pClient; };
	inline IConsole *Console() { return m_pConsole; };
	inline ITextRender *TextRender() { return m_pTextRender; };
	inline IStorage *Storage() { return m_pStorage; };
	inline CUI *UI() { return &m_UI; }
	inline CRenderTools *RenderTools() { return &m_RenderTools; }

public:

	CEditor();
	~CEditor();

	void Init();
	void Update();
	void Render();
	bool HasUnsavedData() const;
	void OnInput(IInput::CEvent Event);
};

#endif
