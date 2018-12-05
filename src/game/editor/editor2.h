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

template<typename T, u32 ELT_COUNT_MAX, u32 RING_ELT_COUNT=1>
class CChainAllocator
{
	int64 m_AllocatedSize;
	u8* m_aRingUsed;
	T* m_pElementBuffer;

public:
	typedef CMemBlock<T> BlockT;

	CChainAllocator()
	{
		m_AllocatedSize = 0;
		m_aRingUsed = 0;
		m_pElementBuffer = 0;
	}

	~CChainAllocator()
	{
		Deinit();
	}

	void Init()
	{
		const int TOTAL_RING_COUNT = (ELT_COUNT_MAX/RING_ELT_COUNT);
		dbg_assert(m_pElementBuffer == 0, "Don't init this twice");
		m_AllocatedSize = sizeof(u8)*TOTAL_RING_COUNT+sizeof(T)*ELT_COUNT_MAX;
		m_aRingUsed = (u8*)mem_alloc(m_AllocatedSize, 0); // TODO: align
		m_pElementBuffer = (T*)(m_aRingUsed+ELT_COUNT_MAX);
		mem_zero(m_aRingUsed, sizeof(u8)*TOTAL_RING_COUNT);
	}

	void Deinit()
	{
		mem_free(m_aRingUsed);
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
				i += 7;
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

		if(pBlock->m_pStart >= m_pElementBuffer &&
			pBlock->m_pStart+pBlock->m_Count <= m_pElementBuffer+ELT_COUNT_MAX)
		{
			const int Start = pBlock->m_pStart-m_pElementBuffer;
			const int Count = pBlock->m_Count;
			mem_zero(m_aRingUsed+(Start/RING_ELT_COUNT), sizeof(u8)*(Count/RING_ELT_COUNT));
		}
		else
			mem_free(pBlock->m_pStart);

		pBlock->m_Count = 0;
	}

	inline int64 AllocatedSize() const { return m_AllocatedSize; }
};

struct CEditorMap
{
	enum
	{
		MAX_TEXTURES=64,
		MAX_GROUP_LAYERS=64,
	};

	struct CLayer
	{
		int m_Type;
		int m_ImageID;
		vec4 m_Color;

		union
		{
			struct {
				int m_TileStartID;
				u16 m_Width;
				u16 m_Height;
			};

			struct {
				int m_QuadStartID;
				int m_QuadCount;
			};
		};
	};

	struct CGroup
	{
		int m_apLayerIDs[MAX_GROUP_LAYERS];
		int m_LayerCount = 0;
		int m_ParallaxX;
		int m_ParallaxY;
		int m_OffsetX;
		int m_OffsetY;
	};

	int m_MapMaxWidth = 0;
	int m_MapMaxHeight = 0;
	int m_GameLayerID = -1;
	int m_GameGroupID = -1;

	// TODO: use a different allocator
	array<CTile> m_aTiles;
	array<CQuad> m_aQuads;
	array<CEnvPoint> m_aEnvPoints;
	array<CLayer> m_aLayers;
	array<CGroup> m_aGroups;
	array<CMapItemEnvelope> m_aEnvelopes;

	IGraphics::CTextureHandle m_aTextures[MAX_TEXTURES];
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
	void Clear();
};

struct CUIButtonState
{
	u8 m_Hovered = false;
	u8 m_Pressed = false;
	u8 m_Clicked = false;
};

class CEditor: public IEditor
{
	enum
	{
		MAX_GROUPS=64,
		MAX_LAYERS=128,
	};

	IGraphics* m_pGraphics;
	IInput *m_pInput;
	IClient *m_pClient;
	IConsole *m_pConsole;
	ITextRender *m_pTextRender;
	IStorage *m_pStorage;
	CRenderTools m_RenderTools;
	CUI m_UI;

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
	bool m_ConfigShowGameEntities = true;

	float m_GfxScreenWidth;
	float m_GfxScreenHeight;
	float m_ZoomWorldViewWidth;
	float m_ZoomWorldViewHeight;

	CUIRect m_UiScreenRect;
	u8 m_UiGroupOpen[MAX_GROUPS] = {0};
	u8 m_UiGroupHidden[MAX_GROUPS] = {0};
	u8 m_UiLayerHovered[MAX_LAYERS] = {0};
	u8 m_UiLayerHidden[MAX_LAYERS] = {0};
	int m_UiSelectedLayerID = -1;
	int m_UiSelectedGroupID = -1;

	vec2 m_RenderGrenadePickupSize;
	vec2 m_RenderShotgunPickupSize;
	vec2 m_RenderLaserPickupSize;

	CChainAllocator<CTile, 2000000, 100> m_TileDispenser;
	CChainAllocator<CQuad, 2000, 4> m_QuadDispenser;
	CChainAllocator<CEnvPoint, 100000, 10> m_EnvPointDispenser;
	CChainAllocator<CEditorMap::CLayer, 1000> m_LayerDispenser;
	CChainAllocator<CEditorMap::CGroup, 200> m_GroupDispenser;
	CChainAllocator<CMapItemEnvelope, 1000> m_EnvelopeDispenser;

	void RenderLayerGameEntities(const CEditorMap::CLayer& GameLayer);

	vec2 CalcGroupScreenOffset(float WorldWidth, float WorldHeight, float PosX, float PosY, float ParallaxX,
					  float ParallaxY);
	static void StaticEnvelopeEval(float TimeOffset, int EnvID, float *pChannels, void *pUser);
	void EnvelopeEval(float TimeOffset, int EnvID, float *pChannels);

	void RenderUI();

	void DrawRect(const CUIRect& Rect, const vec4& Color);
	void DrawRectBorder(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawText(const CUIRect& Rect, const char* pText, float FontSize, vec4 Color = vec4(1,1,1,1));

	void UiDoButtonBehavior(const void* pID, const CUIRect& Rect, CUIButtonState* pButState);

	void Reset();
	void ResetCamera();
	void ChangeZoom(float Zoom);

	int Save(const char* pFilename);
	bool LoadMap(const char *pFileName);

	static void ConLoad(IConsole::IResult *pResult, void *pUserData);

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
