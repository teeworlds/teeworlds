/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_RENDER_H
#define GAME_CLIENT_RENDER_H

#include <engine/graphics.h>
#include <base/vmath.h>
#include <generated/protocol.h>
#include <game/mapitems.h>
#include "ui.h"


// sprite renderings
enum
{
	SPRITE_FLAG_FLIP_Y = 1,
	SPRITE_FLAG_FLIP_X = 2,

	LAYERRENDERFLAG_OPAQUE = 1,
	LAYERRENDERFLAG_TRANSPARENT = 2,

	TILERENDERFLAG_EXTEND = 4,
};

class CTeeRenderInfo
{
public:
	CTeeRenderInfo()
	{
		for(int i = 0; i < NUM_SKINPARTS; i++)
			m_aColors[i] = vec4(1,1,1,1);
		m_Size = 1.0f;
		m_GotAirJump = 1;
	};

	IGraphics::CTextureHandle m_aTextures[NUM_SKINPARTS];
	IGraphics::CTextureHandle m_HatTexture;
	IGraphics::CTextureHandle m_BotTexture;
	int m_HatSpriteIndex;
	vec4 m_BotColor;
	vec4 m_aColors[NUM_SKINPARTS];
	float m_Size;
	int m_GotAirJump;
};

typedef void (*ENVELOPE_EVAL)(float TimeOffset, int Env, float *pChannels, void *pUser);
class CTextCursor;

class CRenderTools
{
	void DrawRoundRectExt(float x, float y, float w, float h, float r, int Corners);
	void DrawRoundRectExt4(float x, float y, float w, float h, vec4 ColorTopLeft, vec4 ColorTopRight, vec4 ColorBottomLeft, vec4 ColorBottomRight, float r, int Corners);


	class CConfig *m_pConfig;
	class IGraphics *m_pGraphics;
	class CUI *m_pUI;
public:

	class IGraphics *Graphics() const { return m_pGraphics; }
	class CUI *UI() const { return m_pUI; }

	void Init(class CConfig *pConfig, class IGraphics *pGraphics, class CUI *pUI);

	void SelectSprite(struct CDataSprite *pSprite, int Flags=0, int sx=0, int sy=0);
	void SelectSprite(int id, int Flags=0, int sx=0, int sy=0);

	void DrawSprite(float x, float y, float size);

	// rects
	void DrawRoundRect(const CUIRect *r, vec4 Color, float Rounding);

	void DrawUIRect(const CUIRect *pRect, vec4 Color, int Corners, float Rounding);
	void DrawUIRect4(const CUIRect *pRect, vec4 ColorTopLeft, vec4 ColorTopRight, vec4 ColorBottomLeft, vec4 ColorBottomRight, int Corners, float Rounding);

	// object render methods (gc_render_obj.cpp)
	void RenderTee(class CAnimState *pAnim, const CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos);
	void RenderTeeHand(const CTeeRenderInfo *pInfo, vec2 CenterPos, vec2 Dir, float AngleOffset,
					   vec2 PostRotOffset);

	// map render methods (gc_render_map.cpp)
	static void RenderEvalEnvelope(CEnvPoint *pPoints, int NumPoints, int Channels, float Time, float *pResult);
	void RenderQuads(CQuad *pQuads, int NumQuads, int Flags, ENVELOPE_EVAL pfnEval, void *pUser);
	void RenderTilemap(CTile *pTiles, int w, int h, float Scale, vec4 Color, int RenderFlags, ENVELOPE_EVAL pfnEval, void *pUser, int ColorEnv, int ColorEnvOffset);

	// helpers
	void MapScreenToWorld(float CenterX, float CenterY, float ParallaxX, float ParallaxY,
		float OffsetX, float OffsetY, float Aspect, float Zoom, float aPoints[4]);
	void MapScreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup, float Zoom);

	void DrawClientID(ITextRender* pTextRender, CTextCursor* pCursor, int ID,
					  const vec4& BgColor = vec4(1, 1, 1, 0.5f), const vec4& TextColor = vec4(0.1f, 0.1f, 0.1f, 1.0f));
	float GetClientIdRectSize(float FontSize);
};

#endif
