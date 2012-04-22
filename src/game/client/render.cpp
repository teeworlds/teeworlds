/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/math.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/map.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>
#include <game/layers.h>
#include "animstate.h"
#include "render.h"

static float gs_SpriteWScale;
static float gs_SpriteHScale;


/*
static void layershot_begin()
{
	if(!config.cl_layershot)
		return;

	Graphics()->Clear(0,0,0);
}

static void layershot_end()
{
	if(!config.cl_layershot)
		return;

	char buf[256];
	str_format(buf, sizeof(buf), "screenshots/layers_%04d.png", config.cl_layershot);
	gfx_screenshot_direct(buf);
	config.cl_layershot++;
}*/

void CRenderTools::SelectSprite(CDataSprite *pSpr, int Flags, int sx, int sy)
{
	int x = pSpr->m_X+sx;
	int y = pSpr->m_Y+sy;
	int w = pSpr->m_W;
	int h = pSpr->m_H;
	int cx = pSpr->m_pSet->m_Gridx;
	int cy = pSpr->m_pSet->m_Gridy;

	float f = sqrtf(h*h + w*w);
	gs_SpriteWScale = w/f;
	gs_SpriteHScale = h/f;

	float x1 = x/(float)cx;
	float x2 = (x+w)/(float)cx;
	float y1 = y/(float)cy;
	float y2 = (y+h)/(float)cy;
	float Temp = 0;

	if(Flags&SPRITE_FLAG_FLIP_Y)
	{
		Temp = y1;
		y1 = y2;
		y2 = Temp;
	}

	if(Flags&SPRITE_FLAG_FLIP_X)
	{
		Temp = x1;
		x1 = x2;
		x2 = Temp;
	}

	Graphics()->QuadsSetSubset(x1, y1, x2, y2);
}

void CRenderTools::SelectSprite(int Id, int Flags, int sx, int sy)
{
	if(Id < 0 || Id >= g_pData->m_NumSprites)
		return;
	SelectSprite(&g_pData->m_aSprites[Id], Flags, sx, sy);
}

void CRenderTools::DrawSprite(float x, float y, float Size)
{
	IGraphics::CQuadItem QuadItem(x, y, Size*gs_SpriteWScale, Size*gs_SpriteHScale);
	Graphics()->QuadsDraw(&QuadItem, 1);
}

void CRenderTools::DrawRoundRectExt(float x, float y, float w, float h, float r, int Corners)
{
	IGraphics::CFreeformItem ArrayF[32];
	int NumItems = 0;
	int Num = 8;
	for(int i = 0; i < Num; i+=2)
	{
		float a1 = i/(float)Num * pi/2;
		float a2 = (i+1)/(float)Num * pi/2;
		float a3 = (i+2)/(float)Num * pi/2;
		float Ca1 = cosf(a1);
		float Ca2 = cosf(a2);
		float Ca3 = cosf(a3);
		float Sa1 = sinf(a1);
		float Sa2 = sinf(a2);
		float Sa3 = sinf(a3);

		if(Corners&1) // TL
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x+r, y+r,
			x+(1-Ca1)*r, y+(1-Sa1)*r,
			x+(1-Ca3)*r, y+(1-Sa3)*r,
			x+(1-Ca2)*r, y+(1-Sa2)*r);

		if(Corners&2) // TR
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x+w-r, y+r,
			x+w-r+Ca1*r, y+(1-Sa1)*r,
			x+w-r+Ca3*r, y+(1-Sa3)*r,
			x+w-r+Ca2*r, y+(1-Sa2)*r);

		if(Corners&4) // BL
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x+r, y+h-r,
			x+(1-Ca1)*r, y+h-r+Sa1*r,
			x+(1-Ca3)*r, y+h-r+Sa3*r,
			x+(1-Ca2)*r, y+h-r+Sa2*r);

		if(Corners&8) // BR
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x+w-r, y+h-r,
			x+w-r+Ca1*r, y+h-r+Sa1*r,
			x+w-r+Ca3*r, y+h-r+Sa3*r,
			x+w-r+Ca2*r, y+h-r+Sa2*r);
	}
	Graphics()->QuadsDrawFreeform(ArrayF, NumItems);

	IGraphics::CQuadItem ArrayQ[9];
	NumItems = 0;
	ArrayQ[NumItems++] = IGraphics::CQuadItem(x+r, y+r, w-r*2, h-r*2); // center
	ArrayQ[NumItems++] = IGraphics::CQuadItem(x+r, y, w-r*2, r); // top
	ArrayQ[NumItems++] = IGraphics::CQuadItem(x+r, y+h-r, w-r*2, r); // bottom
	ArrayQ[NumItems++] = IGraphics::CQuadItem(x, y+r, r, h-r*2); // left
	ArrayQ[NumItems++] = IGraphics::CQuadItem(x+w-r, y+r, r, h-r*2); // right

	if(!(Corners&1)) ArrayQ[NumItems++] = IGraphics::CQuadItem(x, y, r, r); // TL
	if(!(Corners&2)) ArrayQ[NumItems++] = IGraphics::CQuadItem(x+w, y, -r, r); // TR
	if(!(Corners&4)) ArrayQ[NumItems++] = IGraphics::CQuadItem(x, y+h, r, -r); // BL
	if(!(Corners&8)) ArrayQ[NumItems++] = IGraphics::CQuadItem(x+w, y+h, -r, -r); // BR

	Graphics()->QuadsDrawTL(ArrayQ, NumItems);
}

void CRenderTools::DrawRoundRect(float x, float y, float w, float h, float r)
{
	DrawRoundRectExt(x,y,w,h,r,0xf);
}

void CRenderTools::DrawUIRect(const CUIRect *r, vec4 Color, int Corners, float Rounding)
{
	Graphics()->TextureSet(-1);

	// TODO: FIX US
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
	DrawRoundRectExt(r->x,r->y,r->w,r->h,Rounding*UI()->Scale(), Corners);
	Graphics()->QuadsEnd();
}

void CRenderTools::RenderTee(CAnimState *pAnim, CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos)
{
	vec2 Direction = Dir;
	vec2 Position = Pos;

	//Graphics()->TextureSet(data->images[IMAGE_CHAR_DEFAULT].id);
	Graphics()->TextureSet(pInfo->m_Texture);

	// TODO: FIX ME
	Graphics()->QuadsBegin();
	//Graphics()->QuadsDraw(pos.x, pos.y-128, 128, 128);

	// first pass we draw the outline
	// second pass we draw the filling
	for(int p = 0; p < 2; p++)
	{
		int OutLine = p==0 ? 1 : 0;

		for(int f = 0; f < 2; f++)
		{
			float AnimScale = pInfo->m_Size * 1.0f/64.0f;
			float BaseSize = pInfo->m_Size;
			if(f == 1)
			{
				Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi*2);

				// draw body
				Graphics()->SetColor(pInfo->m_ColorBody.r, pInfo->m_ColorBody.g, pInfo->m_ColorBody.b, pInfo->m_ColorBody.a);
				vec2 BodyPos = Position + vec2(pAnim->GetBody()->m_X, pAnim->GetBody()->m_Y)*AnimScale;
				SelectSprite(OutLine?SPRITE_TEE_BODY_OUTLINE:SPRITE_TEE_BODY, 0, 0, 0);
				IGraphics::CQuadItem QuadItem(BodyPos.x, BodyPos.y, BaseSize, BaseSize);
				Graphics()->QuadsDraw(&QuadItem, 1);

				// draw eyes
				if(p == 1)
				{
					switch (Emote)
					{
						case EMOTE_PAIN:
							SelectSprite(SPRITE_TEE_EYE_PAIN, 0, 0, 0);
							break;
						case EMOTE_HAPPY:
							SelectSprite(SPRITE_TEE_EYE_HAPPY, 0, 0, 0);
							break;
						case EMOTE_SURPRISE:
							SelectSprite(SPRITE_TEE_EYE_SURPRISE, 0, 0, 0);
							break;
						case EMOTE_ANGRY:
							SelectSprite(SPRITE_TEE_EYE_ANGRY, 0, 0, 0);
							break;
						default:
							SelectSprite(SPRITE_TEE_EYE_NORMAL, 0, 0, 0);
							break;
					}

					float EyeScale = BaseSize*0.40f;
					float h = Emote == EMOTE_BLINK ? BaseSize*0.15f : EyeScale;
					float EyeSeparation = (0.075f - 0.010f*absolute(Direction.x))*BaseSize;
					vec2 Offset = vec2(Direction.x*0.125f, -0.05f+Direction.y*0.10f)*BaseSize;
					IGraphics::CQuadItem Array[2] = {
						IGraphics::CQuadItem(BodyPos.x-EyeSeparation+Offset.x, BodyPos.y+Offset.y, EyeScale, h),
						IGraphics::CQuadItem(BodyPos.x+EyeSeparation+Offset.x, BodyPos.y+Offset.y, -EyeScale, h)};
					Graphics()->QuadsDraw(Array, 2);
				}
			}

			// draw feet
			CAnimKeyframe *pFoot = f ? pAnim->GetFrontFoot() : pAnim->GetBackFoot();

			float w = BaseSize;
			float h = BaseSize/2;

			Graphics()->QuadsSetRotation(pFoot->m_Angle*pi*2);

			bool Indicate = !pInfo->m_GotAirJump && g_Config.m_ClAirjumpindicator;
			float cs = 1.0f; // color scale

			if(OutLine)
				SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
			else
			{
				SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
				if(Indicate)
					cs = 0.5f;
			}

			Graphics()->SetColor(pInfo->m_ColorFeet.r*cs, pInfo->m_ColorFeet.g*cs, pInfo->m_ColorFeet.b*cs, pInfo->m_ColorFeet.a);
			IGraphics::CQuadItem QuadItem(Position.x+pFoot->m_X*AnimScale, Position.y+pFoot->m_Y*AnimScale, w, h);
			Graphics()->QuadsDraw(&QuadItem, 1);
		}
	}

	Graphics()->QuadsEnd();


}

static void CalcScreenParams(float Amount, float WMax, float HMax, float Aspect, float *w, float *h)
{
	float f = sqrtf(Amount) / sqrtf(Aspect);
	*w = f*Aspect;
	*h = f;

	// limit the view
	if(*w > WMax)
	{
		*w = WMax;
		*h = *w/Aspect;
	}

	if(*h > HMax)
	{
		*h = HMax;
		*w = *h*Aspect;
	}
}

void CRenderTools::MapscreenToWorld(float CenterX, float CenterY, float ParallaxX, float ParallaxY,
	float OffsetX, float OffsetY, float Aspect, float Zoom, float *pPoints)
{
	float Width, Height;
	CalcScreenParams(1150*1000, 1500, 1050, Aspect, &Width, &Height);
	CenterX *= ParallaxX;
	CenterY *= ParallaxY;
	Width *= Zoom;
	Height *= Zoom;
	pPoints[0] = OffsetX+CenterX-Width/2;
	pPoints[1] = OffsetY+CenterY-Height/2;
	pPoints[2] = pPoints[0]+Width;
	pPoints[3] = pPoints[1]+Height;
}

void CRenderTools::RenderTilemapGenerateSkip(class CLayers *pLayers)
{

	for(int g = 0; g < pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = pLayers->GetGroup(g);

		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = pLayers->GetLayer(pGroup->m_StartLayer+l);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pLayer;
				CTile *pTiles = (CTile *)pLayers->Map()->GetData(pTmap->m_Data);
				for(int y = 0; y < pTmap->m_Height; y++)
				{
					for(int x = 1; x < pTmap->m_Width;)
					{
						int sx;
						for(sx = 1; x+sx < pTmap->m_Width && sx < 255; sx++)
						{
							if(pTiles[y*pTmap->m_Width+x+sx].m_Index)
								break;
						}

						pTiles[y*pTmap->m_Width+x].m_Skip = sx-1;
						x += sx;
					}
				}
			}
		}
	}
}
