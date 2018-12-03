// LordSk
#include <stdint.h>
#include <base/system.h>
#include <base/tl/array.h>

#include <engine/editor.h>
#include <engine/shared/datafile.h>

#include <game/mapitems.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/components/mapimages.h>


class IStorage;
class IGraphics;
class IInput;
class IClient;
class IConsole;
class ITextRender;
class IStorage;

typedef uint8_t u8;
typedef uint16_t u16;

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

	bool Save(IStorage *pStorage, const char *pFileName);
	bool Load(IStorage *pStorage, IGraphics* pGraphics, const char *pFileName);
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

	CEditorMap m_Map;

	bool m_ConfigShowGrid = true;

	float m_GfxScreenWidth;
	float m_GfxScreenHeight;

	CUIRect m_UiScreenRect;
	u8 m_UiGroupOpen[MAX_GROUPS] = {0};
	u8 m_UiGroupHidden[MAX_GROUPS] = {0};
	u8 m_UiLayerHovered[MAX_LAYERS] = {0};
	u8 m_UiLayerHidden[MAX_LAYERS] = {0};
	int m_UiSelectedLayerID = -1;
	int m_UiSelectedGroupID = -1;

	void Update();
	void Render();

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

	int Save(const char* pFilename);
	int Load(const char *pFileName, int StorageType);

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
	void UpdateAndRender();
	bool HasUnsavedData() const;
};
