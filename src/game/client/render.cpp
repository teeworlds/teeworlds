/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/math.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/textrender.h>
#include <generated/client_data.h>
#include <game/layers.h>
#include "animstate.h"
#include "render.h"

static float gs_SpriteWScale;
static float gs_SpriteHScale;

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
	float x2 = (x+w-1/32.0f)/(float)cx;
	float y1 = y/(float)cy;
	float y2 = (y+h-1/32.0f)/(float)cy;
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

		if(Corners&16) // ITL
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x, y,
			x+(1-Ca1)*r, y-r+Sa1*r,
			x+(1-Ca3)*r, y-r+Sa3*r,
			x+(1-Ca2)*r, y-r+Sa2*r);

		if(Corners&32) // ITR
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x+w, y,
			x+w-r+Ca1*r, y-r+Sa1*r,
			x+w-r+Ca3*r, y-r+Sa3*r,
			x+w-r+Ca2*r, y-r+Sa2*r);

		if(Corners&64) // IBL
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x, y+h,
			x+(1-Ca1)*r, y+h+(1-Sa1)*r,
			x+(1-Ca3)*r, y+h+(1-Sa3)*r,
			x+(1-Ca2)*r, y+h+(1-Sa2)*r);

		if(Corners&128) // IBR
		ArrayF[NumItems++] = IGraphics::CFreeformItem(
			x+w, y+h,
			x+w-r+Ca1*r, y+h+(1-Sa1)*r,
			x+w-r+Ca3*r, y+h+(1-Sa3)*r,
			x+w-r+Ca2*r, y+h+(1-Sa2)*r);
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

void CRenderTools::DrawRoundRectExt4(float x, float y, float w, float h, vec4 ColorTopLeft, vec4 ColorTopRight, vec4 ColorBottomLeft, vec4 ColorBottomRight, float r, int Corners)
{
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
		{
			Graphics()->SetColor(ColorTopLeft.r, ColorTopLeft.g, ColorTopLeft.b, ColorTopLeft.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x+r, y+r,
									x+(1-Ca1)*r, y+(1-Sa1)*r,
									x+(1-Ca3)*r, y+(1-Sa3)*r,
									x+(1-Ca2)*r, y+(1-Sa2)*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&2) // TR
		{
			Graphics()->SetColor(ColorTopRight.r, ColorTopRight.g, ColorTopRight.b, ColorTopRight.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x+w-r, y+r,
									x+w-r+Ca1*r, y+(1-Sa1)*r,
									x+w-r+Ca3*r, y+(1-Sa3)*r,
									x+w-r+Ca2*r, y+(1-Sa2)*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&4) // BL
		{
			Graphics()->SetColor(ColorBottomLeft.r, ColorBottomLeft.g, ColorBottomLeft.b, ColorBottomLeft.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x+r, y+h-r,
									x+(1-Ca1)*r, y+h-r+Sa1*r,
									x+(1-Ca3)*r, y+h-r+Sa3*r,
									x+(1-Ca2)*r, y+h-r+Sa2*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&8) // BR
		{
			Graphics()->SetColor(ColorBottomRight.r, ColorBottomRight.g, ColorBottomRight.b, ColorBottomRight.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x+w-r, y+h-r,
									x+w-r+Ca1*r, y+h-r+Sa1*r,
									x+w-r+Ca3*r, y+h-r+Sa3*r,
									x+w-r+Ca2*r, y+h-r+Sa2*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&16) // ITL
		{
			Graphics()->SetColor(ColorTopLeft.r, ColorTopLeft.g, ColorTopLeft.b, ColorTopLeft.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x, y,
									x+(1-Ca1)*r, y-r+Sa1*r,
									x+(1-Ca3)*r, y-r+Sa3*r,
									x+(1-Ca2)*r, y-r+Sa2*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&32) // ITR
		{
			Graphics()->SetColor(ColorTopRight.r, ColorTopRight.g, ColorTopRight.b, ColorTopRight.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x+w, y,
									x+w-r+Ca1*r, y-r+Sa1*r,
									x+w-r+Ca3*r, y-r+Sa3*r,
									x+w-r+Ca2*r, y-r+Sa2*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&64) // IBL
		{
			Graphics()->SetColor(ColorBottomLeft.r, ColorBottomLeft.g, ColorBottomLeft.b, ColorBottomLeft.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x, y+h,
									x+(1-Ca1)*r, y+h+(1-Sa1)*r,
									x+(1-Ca3)*r, y+h+(1-Sa3)*r,
									x+(1-Ca2)*r, y+h+(1-Sa2)*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&128) // IBR
		{
			Graphics()->SetColor(ColorBottomRight.r, ColorBottomRight.g, ColorBottomRight.b, ColorBottomRight.a);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
									x+w, y+h,
									x+w-r+Ca1*r, y+h+(1-Sa1)*r,
									x+w-r+Ca3*r, y+h+(1-Sa3)*r,
									x+w-r+Ca2*r, y+h+(1-Sa2)*r);
			Graphics()->QuadsDrawFreeform(&ItemF, 1);
		}
	}

	Graphics()->SetColor4(ColorTopLeft, ColorTopRight, ColorBottomLeft, ColorBottomRight);
	IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y+r, w-r*2, h-r*2); // center
	Graphics()->QuadsDrawTL(&ItemQ, 1);
	Graphics()->SetColor4(ColorTopLeft, ColorTopRight, ColorTopLeft, ColorTopRight);
	ItemQ = IGraphics::CQuadItem(x+r, y, w-r*2, r); // top
	Graphics()->QuadsDrawTL(&ItemQ, 1);
	Graphics()->SetColor4(ColorBottomLeft, ColorBottomRight, ColorBottomLeft, ColorBottomRight);
	ItemQ = IGraphics::CQuadItem(x+r, y+h-r, w-r*2, r); // bottom
	Graphics()->QuadsDrawTL(&ItemQ, 1);
	Graphics()->SetColor4(ColorTopLeft, ColorTopLeft, ColorBottomLeft, ColorBottomLeft);
	ItemQ = IGraphics::CQuadItem(x, y+r, r, h-r*2); // left
	Graphics()->QuadsDrawTL(&ItemQ, 1);
	Graphics()->SetColor4(ColorTopRight, ColorTopRight, ColorBottomRight, ColorBottomRight);
	ItemQ = IGraphics::CQuadItem(x+w-r, y+r, r, h-r*2); // right
	Graphics()->QuadsDrawTL(&ItemQ, 1);

	if(!(Corners&1))
	{
		Graphics()->SetColor(ColorTopLeft.r, ColorTopLeft.g, ColorTopLeft.b, ColorTopLeft.a);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y, r, r); // TL
		Graphics()->QuadsDrawTL(&ItemQ, 1);
	}
	if(!(Corners&2))
	{
		Graphics()->SetColor(ColorTopRight.r, ColorTopRight.g, ColorTopRight.b, ColorTopRight.a);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w, y, -r, r); // TR
		Graphics()->QuadsDrawTL(&ItemQ, 1);
	}
	if(!(Corners&4))
	{
		Graphics()->SetColor(ColorBottomLeft.r, ColorBottomLeft.g, ColorBottomLeft.b, ColorBottomLeft.a);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y+h, r, -r); // BL
		Graphics()->QuadsDrawTL(&ItemQ, 1);
	}
	if(!(Corners&8))
	{
		Graphics()->SetColor(ColorBottomRight.r, ColorBottomRight.g, ColorBottomRight.b, ColorBottomRight.a);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w, y+h, -r, -r); // BR
		Graphics()->QuadsDrawTL(&ItemQ, 1);
	}
}

void CRenderTools::DrawRoundRect(const CUIRect *r, vec4 Color, float Rounding)
{
	DrawUIRect(r, Color, CUI::CORNER_ALL, Rounding);
}

void CRenderTools::DrawUIRect(const CUIRect *r, vec4 Color, int Corners, float Rounding)
{
	Graphics()->TextureClear();

	// TODO: FIX US
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);
	DrawRoundRectExt(r->x,r->y,r->w,r->h,Rounding, Corners);
	Graphics()->QuadsEnd();
}

void CRenderTools::DrawUIRect4(const CUIRect *r, vec4 ColorTopLeft, vec4 ColorTopRight, vec4 ColorBottomLeft, vec4 ColorBottomRight, int Corners, float Rounding)
{
	Graphics()->TextureClear();

	Graphics()->QuadsBegin();
	DrawRoundRectExt4(r->x,r->y,r->w,r->h,ColorTopLeft,ColorTopRight,ColorBottomLeft,ColorBottomRight,Rounding, Corners);
	Graphics()->QuadsEnd();
}

void CRenderTools::RenderTee(CAnimState *pAnim, CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos)
{
	vec2 Direction = Dir;
	vec2 Position = Pos;

	// first pass we draw the outline
	// second pass we draw the filling
	for(int p = 0; p < 2; p++)
	{
		bool OutLine = p==0;

		for(int f = 0; f < 2; f++)
		{
			float AnimScale = pInfo->m_Size * 1.0f/64.0f;
			float BaseSize = pInfo->m_Size;
			if(f == 1)
			{
				vec2 BodyPos = Position + vec2(pAnim->GetBody()->m_X, pAnim->GetBody()->m_Y)*AnimScale;
				IGraphics::CQuadItem BodyItem(BodyPos.x, BodyPos.y, BaseSize, BaseSize);
				IGraphics::CQuadItem Item;

				// draw decoration
				if(pInfo->m_aTextures[SKINPART_DECORATION].IsValid())
				{
					Graphics()->TextureSet(pInfo->m_aTextures[2]);
					Graphics()->QuadsBegin();
					Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi*2);
					Graphics()->SetColor(pInfo->m_aColors[SKINPART_DECORATION].r, pInfo->m_aColors[SKINPART_DECORATION].g, pInfo->m_aColors[SKINPART_DECORATION].b, pInfo->m_aColors[SKINPART_DECORATION].a);
					SelectSprite(OutLine?SPRITE_TEE_DECORATION_OUTLINE:SPRITE_TEE_DECORATION, 0, 0, 0);
					Item = BodyItem;
					Graphics()->QuadsDraw(&Item, 1);
					Graphics()->QuadsEnd();
				}

				// draw body (behind marking)
				Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_BODY]);
				Graphics()->QuadsBegin();
				Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi*2);
				if(OutLine)
				{
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					SelectSprite(SPRITE_TEE_BODY_OUTLINE, 0, 0, 0);
				}
				else
				{
					Graphics()->SetColor(pInfo->m_aColors[SKINPART_BODY].r, pInfo->m_aColors[SKINPART_BODY].g, pInfo->m_aColors[SKINPART_BODY].b, pInfo->m_aColors[SKINPART_BODY].a);
					SelectSprite(SPRITE_TEE_BODY, 0, 0, 0);
				}
				Item = BodyItem;
				Graphics()->QuadsDraw(&Item, 1);
				Graphics()->QuadsEnd();

				// draw marking
				if(pInfo->m_aTextures[SKINPART_MARKING].IsValid() && !OutLine)
				{
					Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_MARKING]);
					Graphics()->QuadsBegin();
					Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi*2);
					Graphics()->SetColor(pInfo->m_aColors[SKINPART_MARKING].r*pInfo->m_aColors[SKINPART_MARKING].a, pInfo->m_aColors[SKINPART_MARKING].g*pInfo->m_aColors[SKINPART_MARKING].a,
						pInfo->m_aColors[SKINPART_MARKING].b*pInfo->m_aColors[SKINPART_MARKING].a, pInfo->m_aColors[SKINPART_MARKING].a);
					SelectSprite(SPRITE_TEE_MARKING, 0, 0, 0);
					Item = BodyItem;
					Graphics()->QuadsDraw(&Item, 1);
					Graphics()->QuadsEnd();
				}

				// draw body (in front of marking)
				if(!OutLine)
				{
					Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_BODY]);
					Graphics()->QuadsBegin();
					Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi*2);
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					for(int t = 0; t < 2; t++)
					{
						SelectSprite(t==0?SPRITE_TEE_BODY_SHADOW:SPRITE_TEE_BODY_UPPER_OUTLINE, 0, 0, 0);
						Item = BodyItem;
						Graphics()->QuadsDraw(&Item, 1);
					}
					Graphics()->QuadsEnd();
				}

				// draw eyes
				Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_EYES]);
				Graphics()->QuadsBegin();
				Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi*2);
				Graphics()->SetColor(pInfo->m_aColors[SKINPART_EYES].r, pInfo->m_aColors[SKINPART_EYES].g, pInfo->m_aColors[SKINPART_EYES].b, pInfo->m_aColors[SKINPART_EYES].a);
				if(p == 1)
				{
					switch (Emote)
					{
						case EMOTE_PAIN:
							SelectSprite(SPRITE_TEE_EYES_PAIN, 0, 0, 0);
							break;
						case EMOTE_HAPPY:
							SelectSprite(SPRITE_TEE_EYES_HAPPY, 0, 0, 0);
							break;
						case EMOTE_SURPRISE:
							SelectSprite(SPRITE_TEE_EYES_SURPRISE, 0, 0, 0);
							break;
						case EMOTE_ANGRY:
							SelectSprite(SPRITE_TEE_EYES_ANGRY, 0, 0, 0);
							break;
						default:
							SelectSprite(SPRITE_TEE_EYES_NORMAL, 0, 0, 0);
							break;
					}

					float EyeScale = BaseSize*0.60f;
					float h = Emote == EMOTE_BLINK ? BaseSize*0.15f/2.0f : EyeScale/2.0f;
					vec2 Offset = vec2(Direction.x*0.125f, -0.05f+Direction.y*0.10f)*BaseSize;
					IGraphics::CQuadItem QuadItem(BodyPos.x+Offset.x, BodyPos.y+Offset.y, EyeScale, h);
					Graphics()->QuadsDraw(&QuadItem, 1);
				}
				Graphics()->QuadsEnd();

				// draw xmas hat
				if(!OutLine && pInfo->m_HatTexture.IsValid())
				{
					Graphics()->TextureSet(pInfo->m_HatTexture);
					Graphics()->QuadsBegin();
					Graphics()->QuadsSetRotation(pAnim->GetBody()->m_Angle*pi * 2);
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					int Flag = Direction.x < 0.0f ? SPRITE_FLAG_FLIP_X : 0;
					switch(pInfo->m_HatSpriteIndex)
					{
					case 0:
						SelectSprite(SPRITE_TEE_HATS_TOP1, Flag, 0, 0);
						break;
					case 1:
						SelectSprite(SPRITE_TEE_HATS_TOP2, Flag, 0, 0);
						break;
					case 2:
						SelectSprite(SPRITE_TEE_HATS_SIDE1, Flag, 0, 0);
						break;
					case 3:
						SelectSprite(SPRITE_TEE_HATS_SIDE2, Flag, 0, 0);
					}
					Item = BodyItem;
					Graphics()->QuadsDraw(&Item, 1);
					Graphics()->QuadsEnd();
				}
			}

			// draw feet
			Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_FEET]);
			Graphics()->QuadsBegin();
			CAnimKeyframe *pFoot = f ? pAnim->GetFrontFoot() : pAnim->GetBackFoot();

			float w = BaseSize/2.1f;
			float h = w;

			Graphics()->QuadsSetRotation(pFoot->m_Angle*pi*2);

			if(OutLine)
			{
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
			}
			else
			{
				bool Indicate = !pInfo->m_GotAirJump && g_Config.m_ClAirjumpindicator;
				float cs = 1.0f; // color scale
				if(Indicate)
					cs = 0.5f;
				Graphics()->SetColor(pInfo->m_aColors[SKINPART_FEET].r*cs, pInfo->m_aColors[SKINPART_FEET].g*cs, pInfo->m_aColors[SKINPART_FEET].b*cs, pInfo->m_aColors[SKINPART_FEET].a);
				SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
			}

			IGraphics::CQuadItem QuadItem(Position.x+pFoot->m_X*AnimScale, Position.y+pFoot->m_Y*AnimScale, w, h);
			Graphics()->QuadsDraw(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
	}
}

void CRenderTools::RenderTeeHand(CTeeRenderInfo *pInfo, vec2 CenterPos, vec2 Dir, float AngleOffset,
								 vec2 PostRotOffset)
{
	// in-game hand size is 15 when tee size is 64
	float BaseSize = 15.0f * (pInfo->m_Size / 64.0f);

	vec2 HandPos = CenterPos + Dir;
	float Angle = angle(Dir);
	if(Dir.x < 0)
		Angle -= AngleOffset;
	else
		Angle += AngleOffset;

	vec2 DirX = Dir;
	vec2 DirY(-Dir.y,Dir.x);

	if(Dir.x < 0)
		DirY = -DirY;

	HandPos += DirX * PostRotOffset.x;
	HandPos += DirY * PostRotOffset.y;

	const vec4 Color = pInfo->m_aColors[SKINPART_HANDS];
	IGraphics::CQuadItem QuadOutline(HandPos.x, HandPos.y, 2*BaseSize, 2*BaseSize);
	IGraphics::CQuadItem QuadHand = QuadOutline;

	Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_HANDS]);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
	Graphics()->QuadsSetRotation(Angle);

	SelectSprite(SPRITE_TEE_HAND_OUTLINE, 0, 0, 0);
	Graphics()->QuadsDraw(&QuadOutline, 1);
	SelectSprite(SPRITE_TEE_HAND, 0, 0, 0);
	Graphics()->QuadsDraw(&QuadHand, 1);

	Graphics()->QuadsSetRotation(0);
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

void CRenderTools::MapScreenToWorld(float CenterX, float CenterY, float ParallaxX, float ParallaxY,
	float OffsetX, float OffsetY, float Aspect, float Zoom, float aPoints[4])
{
	float Width, Height;
	CalcScreenParams(1150*1000, 1500, 1050, Aspect, &Width, &Height);
	CenterX *= ParallaxX;
	CenterY *= ParallaxY;
	Width *= Zoom;
	Height *= Zoom;
	aPoints[0] = OffsetX+CenterX-Width/2;
	aPoints[1] = OffsetY+CenterY-Height/2;
	aPoints[2] = aPoints[0]+Width;
	aPoints[3] = aPoints[1]+Height;
}

void CRenderTools::MapScreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup, float Zoom)
{
	float aPoints[4];
	MapScreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), Zoom, aPoints);
	Graphics()->MapScreen(aPoints[0], aPoints[1], aPoints[2], aPoints[3]);
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

void CRenderTools::DrawClientID(ITextRender* pTextRender, CTextCursor* pCursor, int ID,
								const vec4& BgColor, const vec4& TextColor)
{
	if(!g_Config.m_ClShowUserId) return;

	char aBuff[4];
	str_format(aBuff, sizeof(aBuff), "%2d ", ID);

	const float LinebaseY = pTextRender->TextGetLineBaseY(pCursor);

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	float FakeToScreenY = (Graphics()->ScreenHeight()/(ScreenY1-ScreenY0));
	float FontSize = (int)(pCursor->m_FontSize * FakeToScreenY)/FakeToScreenY;

	CUIRect Rect;
	Rect.x = pCursor->m_X;
	Rect.y = LinebaseY - FontSize + 0.025f * FontSize;
	Rect.w = 1.4f * FontSize;
	Rect.h = FontSize;
	DrawRoundRect(&Rect, BgColor, 0.25f * FontSize);

	const float PrevX = pCursor->m_X;
	pCursor->m_X += (ID < 10 ? 0.04f: 0.0f) * FontSize;

	// TODO: make a simple text one (no shadow)
	pTextRender->TextShadowed(pCursor, aBuff, -1, vec2(0,0), vec4(0,0,0,0), TextColor);

	pCursor->m_X = PrevX + Rect.w + 0.2f * FontSize;
}

float CRenderTools::GetClientIdRectSize(float FontSize)
{
	if(!g_Config.m_ClShowUserId) return 0;
	return 1.4f * FontSize + 0.2f * FontSize;
}
