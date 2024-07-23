/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>
#include <algorithm>

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

void CRenderTools::Init(CConfig *pConfig, IGraphics *pGraphics)
{
	m_pConfig = pConfig;
	m_pGraphics = pGraphics;
}

void CRenderTools::SelectSprite(const CDataSprite *pSpr, int Flags, int sx, int sy)
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

	float x1 = x/(float)cx + 0.5f/(float)(cx*32);
	float x2 = (x+w)/(float)cx - 0.5f/(float)(cx*32);
	float y1 = y/(float)cy + 0.5f/(float)(cy*32);
	float y2 = (y+h)/(float)cy - 0.5f/(float)(cy*32);

	if(Flags&SPRITE_FLAG_FLIP_Y)
		std::swap(y1, y2);

	if(Flags&SPRITE_FLAG_FLIP_X)
		std::swap(x1, x2);

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

void CRenderTools::RenderCursor(float CenterX, float CenterY, float Size)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->WrapClamp();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(CenterX, CenterY, Size, Size);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	Graphics()->WrapNormal();
}

void CRenderTools::RenderTee(CAnimState *pAnim, const CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos)
{
	vec2 Direction = Dir;
	vec2 Position = Pos;
	bool IsBot = pInfo->m_BotTexture.IsValid();

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
				IGraphics::CQuadItem BotItem(BodyPos.x+(2.f/3.f)*AnimScale, BodyPos.y+(-16+2.f/3.f)*AnimScale, BaseSize, BaseSize); // x+0.66, y+0.66 to correct some rendering bug
				IGraphics::CQuadItem Item;

				// draw bot visuals (background)
				if(IsBot && !OutLine)
				{
					Graphics()->TextureSet(pInfo->m_BotTexture);
					Graphics()->QuadsBegin();
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					SelectSprite(SPRITE_TEE_BOT_BACKGROUND, 0, 0, 0);
					Item = BotItem;
					Graphics()->QuadsDraw(&Item, 1);
					Graphics()->QuadsEnd();
				}

				// draw bot visuals (foreground)
				if(IsBot && !OutLine)
				{
					Graphics()->TextureSet(pInfo->m_BotTexture);
					Graphics()->QuadsBegin();
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					SelectSprite(SPRITE_TEE_BOT_FOREGROUND, 0, 0, 0);
					Item = BotItem;
					Graphics()->QuadsDraw(&Item, 1);
					Graphics()->SetColor(pInfo->m_BotColor.r, pInfo->m_BotColor.g, pInfo->m_BotColor.b, pInfo->m_BotColor.a);
					SelectSprite(SPRITE_TEE_BOT_GLOW, 0, 0, 0);
					Item = BotItem;
					Graphics()->QuadsDraw(&Item, 1);
					Graphics()->QuadsEnd();
				}

				// draw decoration
				if(pInfo->m_aTextures[SKINPART_DECORATION].IsValid())
				{
					Graphics()->TextureSet(pInfo->m_aTextures[SKINPART_DECORATION]);
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
				if(IsBot)
				{
					Graphics()->SetColor(pInfo->m_BotColor.r, pInfo->m_BotColor.g, pInfo->m_BotColor.b, pInfo->m_BotColor.a);
					Emote = EMOTE_SURPRISE;
				}
				else
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
				bool Indicate = !pInfo->m_GotAirJump && m_pConfig->m_ClAirjumpindicator;
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

void CRenderTools::RenderTeeHand(const CTeeRenderInfo *pInfo, vec2 CenterPos, vec2 Dir, float AngleOffset,
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

void CRenderTools::MapScreenToGroup(float CenterX, float CenterY, const CMapItemGroup *pGroup, float Zoom)
{
	float aPoints[4];
	MapScreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), Zoom, aPoints);
	Graphics()->MapScreen(aPoints[0], aPoints[1], aPoints[2], aPoints[3]);
}
