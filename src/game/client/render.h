/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_RENDER_H
#define GAME_CLIENT_RENDER_H

#include <engine/graphics.h>
#include <base/vmath.h>
#include <generated/protocol.h>
#include <game/mapitems.h>

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
	class CConfig *m_pConfig;
	class IGraphics *m_pGraphics;

	class IGraphics *Graphics() const { return m_pGraphics; }

public:
	void Init(class CConfig *pConfig, class IGraphics *pGraphics);

	void SelectSprite(const struct CDataSprite *pSprite, int Flags = 0, int sx = 0, int sy = 0);
	void SelectSprite(int Id, int Flags = 0, int sx = 0, int sy = 0);

	void DrawSprite(float x, float y, float Size);
	void RenderCursor(float CenterX, float CenterY, float Size);
	void RenderCursor(vec2 Center, float Size) { RenderCursor(Center.x, Center.y, Size); }

	// object render methods
	void RenderTee(class CAnimState *pAnim, const CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos);
	void RenderTeeHand(const CTeeRenderInfo *pInfo, vec2 CenterPos, vec2 Dir, float AngleOffset, vec2 PostRotOffset);

	// map render methods (render_map.cpp)
	static void RenderEvalEnvelope(const CEnvPoint *pPoints, int NumPoints, int Channels, float Time, float *pResult);
	void RenderQuads(const CQuad *pQuads, int NumQuads, int Flags, ENVELOPE_EVAL pfnEval, void *pUser);
	void RenderTilemap(const CTile *pTiles, int w, int h, float Scale, vec4 Color, int RenderFlags, ENVELOPE_EVAL pfnEval, void *pUser, int ColorEnv, int ColorEnvOffset);

	// helpers
	void MapScreenToWorld(float CenterX, float CenterY, float ParallaxX, float ParallaxY,
		float OffsetX, float OffsetY, float Aspect, float Zoom, float aPoints[4]);
	void MapScreenToGroup(float CenterX, float CenterY, const CMapItemGroup *pGroup, float Zoom);
};

#endif
