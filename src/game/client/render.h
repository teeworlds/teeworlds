/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_RENDER_H
#define GAME_CLIENT_RENDER_H

#include <base/vmath.h>
#include <game/mapitems.h>
#include "ui.h"


class CTeeRenderInfo
{
public:
	CTeeRenderInfo()
	{
		m_Texture = -1;
		m_ColorBody = vec4(1,1,1,1);
		m_ColorFeet = vec4(1,1,1,1);
		m_Size = 1.0f;
		m_GotAirJump = 1;
	};

	int m_Texture;
	vec4 m_ColorBody;
	vec4 m_ColorFeet;
	float m_Size;
	int m_GotAirJump;
};

// sprite renderings
enum
{
	SPRITE_FLAG_FLIP_Y=1,
	SPRITE_FLAG_FLIP_X=2,

	LAYERRENDERFLAG_OPAQUE=1,
	LAYERRENDERFLAG_TRANSPARENT=2,

	TILERENDERFLAG_EXTEND=4,
};

typedef void (*ENVELOPE_EVAL)(float TimeOffset, int Env, float *pChannels, void *pUser);

class CRenderTools
{
public:
	class IGraphics *m_pGraphics;
	class CUI *m_pUI;

	class IGraphics *Graphics() const { return m_pGraphics; }
	class CUI *UI() const { return m_pUI; }

	//typedef struct SPRITE;

	void SelectSprite(struct CDataSprite *pSprite, int Flags=0, int sx=0, int sy=0);
	void SelectSprite(int id, int Flags=0, int sx=0, int sy=0);

	void DrawSprite(float x, float y, float size);

	// rects
	void DrawRoundRect(float x, float y, float w, float h, float r);
	void DrawRoundRectExt(float x, float y, float w, float h, float r, int Corners);

	void DrawUIRect(const CUIRect *pRect, vec4 Color, int Corners, float Rounding);

	// larger rendering methods
	void RenderTilemapGenerateSkip(class CLayers *pLayers);

	// object render methods (gc_render_obj.cpp)
	void RenderTee(class CAnimState *pAnim, CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos);

	// map render methods (gc_render_map.cpp)
	static void RenderEvalEnvelope(CEnvPoint *pPoints, int NumPoints, int Channels, float Time, float *pResult);
	void RenderQuads(CQuad *pQuads, int NumQuads, int Flags, ENVELOPE_EVAL pfnEval, void *pUser);
	void RenderTilemap(CTile *pTiles, int w, int h, float Scale, vec4 Color, int RenderFlags, ENVELOPE_EVAL pfnEval, void *pUser, int ColorEnv, int ColorEnvOffset);

	// helpers
	void MapscreenToWorld(float CenterX, float CenterY, float ParallaxX, float ParallaxY,
		float OffsetX, float OffsetY, float Aspect, float Zoom, float *pPoints);

};

#endif
